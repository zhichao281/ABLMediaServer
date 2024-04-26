var XMPPVideoRoom = (function() {

	
	/** 
	 * Interface with Jitsi Video Room and WebRTC-streamer API
	 * @constructor
	 * @param {string} xmppUrl - url of XMPP server
	 * @param {string} srvurl - url of WebRTC-streamer
	*/
	var XMPPVideoRoom = function XMPPVideoRoom (xmppUrl, srvurl, bus) {	
		this.xmppUrl     = xmppUrl;
		this.srvurl      = srvurl || "//"+window.location.hostname+":"+window.location.port;
		this.sessionList = {};
		this.bus = bus;
		this.connection = {};
	};
		
	XMPPVideoRoom.prototype.getConnection = function(roomId, username) {
		var key = roomId + "/" + username
		return new Promise( (resolve, reject) => {
			if (this.connection.hasOwnProperty(key)) {
				resolve(this.connection[key]);
			} else {
				var connection = new Strophe.Connection("https://" + this.xmppUrl + "/http-bind?room="+roomId);
				connection.connect(this.xmppUrl, null, (status) => {
					if (status === Strophe.Status.CONNECTING) {
						console.log('Strophe is connecting.');
					} else if (status === Strophe.Status.CONNFAIL) {
						console.log('Strophe failed to connect.');
						reject('Strophe failed to connect.')
					} else if (status === Strophe.Status.CONNECTED) {
						console.log('Strophe is connected.');
						if (connection.disco) {
							connection.disco.addIdentity('client', 'http://jitsi.org/jitsimeet');
						}
						this.connection[key] = connection;
						resolve(connection);
					}
				});
			}
		});
	}

	/** 
	* Ask to publish a stream from WebRTC-streamer in a XMPP Video Room user
	* @param {string} roomId - id of the XMPP Video Room to join
	* @param {string} url - WebRTC stream to publish
	* @param {string} username - name in Video Room
	*/
	XMPPVideoRoom.prototype.join = function(roomId, url, username, password) {

		this.getConnection(roomId, username).then( connection => {
			if (connection.disco) {
				connection.disco.addFeature("urn:xmpp:jingle:1");	
				connection.disco.addFeature("urn:xmpp:jingle:errors:1");	
				connection.disco.addFeature("urn:xmpp:jingle:apps:rtp:1");	
				connection.disco.addFeature("urn:xmpp:jingle:apps:rtp:info:1")
				connection.disco.addFeature("urn:xmpp:jingle:apps:rtp:errors:1")				
				connection.disco.addFeature("urn:xmpp:jingle:apps:rtp:audio");	
				connection.disco.addFeature("urn:xmpp:jingle:apps:rtp:video");	
				connection.disco.addFeature("urn:xmpp:jingle:apps:dtls:0");	
				connection.disco.addFeature("urn:xmpp:jingle:transports:ice-udp:1");
				connection.disco.addFeature("urn:xmpp:ping");	
				connection.disco.addFeature("urn:ietf:rfc:5761")
				connection.disco.addFeature("urn:ietf:rfc:5888"); // a=group, e.g. bundle
			}		
			connection.addHandler(this.OnJingle.bind(this, connection, roomId, username, url), 'urn:xmpp:jingle:1', 'iq', 'set', null, null);
	
			var roomUrl = this.getRoomUrl(roomId);			
			this.emitState(roomId + '/' + username, "joining");

			var extPresence = Strophe.xmlElement('nick', {xmlns:'http://jabber.org/protocol/nick'}, username);
			connection.muc.join(roomUrl, username, null, null, null, password, null, extPresence);

			var extPresence = Strophe.xmlElement('videomuted', {xmlns:'http://jitsi.org/jitmeet/video'}, false);									
			connection.muc.join(roomUrl, username, null, this.OnPresence.bind(this,connection,roomId), null, password, null, extPresence);				
			
			this.emitState(roomId + '/' + username, "joined");
		});
	}

	/** 
	* Ask to leave a XMPP Video Room user
	* @param {string} roomId - id of the XMPP Video Room to leave
	* @param {string} name - name in Video Room
	*/
	XMPPVideoRoom.prototype.leave = function (roomId, username) {
		var found = false;
		Object.entries(this.sessionList).forEach( ([sid, session]) => {
			if ( (session.roomid === roomId) && (session.name === username) ) {
				this.leaveSession(sid);
				found = true;
			}
		});
		if (!found) {
			var roomUrl = this.getRoomUrl(roomId);
			this.getConnection(roomId, username).then( connection => {
				var sessionname = "kicker" + Math.floor(Math.random()*1000000).toString();
		
				connection.muc.join(roomUrl, sessionname, null, null, null, null, null, null);						
				connection.muc.kick(roomUrl, username, "Unknown session", () => this.emitPresence(roomId + '/' + username, "out") )
				connection.muc.leave(roomUrl, sessionname);

				connection.flush()
			})
		}
	}

	/** 
	* Ask to leave all XMPP Video Room
	*/
	XMPPVideoRoom.prototype.leaveAll = function () {
		Object.keys(this.sessionList).forEach( (sid) => {
			this.leaveSession(sid);
		});
		this.sessionList = {};
	}

	/** 
	* Query a XMPP Video Room 
	* @param {string} roomid - id of the XMPP Video Room to join
	*/
	XMPPVideoRoom.prototype.query = function(roomId, password) {		
		var sessionname = "query" + Math.floor(Math.random()*1000000).toString();

		this.getConnection(roomId, sessionname).then( connection => {

			var roomUrl = this.getRoomUrl(roomId);						

			connection.muc.join(roomUrl, sessionname, null, this.OnPresence.bind(this,connection,roomId), null, password, null, null);					

			connection.muc.queryOccupants(roomUrl, (query) => {
				var occupants = $(query).find(">query>item");
				occupants.toArray().forEach( (item) => {
					this.emitPresence(roomId + '/'  + item.getAttribute("name"), "in");
				});
			}, () => connection.muc.leave(roomUrl, sessionname), () => connection.muc.leave(roomUrl, sessionname));
		})
	}
	
	
	/*
	/* HTTP callback for /getIceCandidate 
	*/
	XMPPVideoRoom.prototype.onReceiveCandidate = function(connection, roomid, name, answer, candidateList) {
		console.log("============candidateList:" +  JSON.stringify(candidateList));
		var jingle = answer.querySelector("jingle");
		var sid = jingle.getAttribute("sid");
		var from = answer.getAttribute("to");
		var to = answer.getAttribute("from");

		candidateList.forEach(function (candidate) {
			var jsoncandidate = SDPUtil.parse_icecandidate(candidate.candidate);
			console.log("<=== webrtc candidate:" +  JSON.stringify(jsoncandidate));
			// get ufrag
			var ufrag = "";
			var elems = candidate.candidate.split(' ');
			for (var i = 8; i < elems.length; i += 2) {
				switch (elems[i]) {
					case 'ufrag':
						ufrag = elems[i + 1];
						break;
				}
			}
			// get pwd
			var pwd = "";
			var transports = $(jingle).find('>content>transport');
			transports.each( (idx,transport) => {
				if (ufrag == transport.getAttribute('ufrag')) {
					pwd = transport.getAttribute('pwd');
				}
			});
			
			// convert candidate from webrtc to jingle 
			var iq = $iq({ type: "set",  from, to })
					.c('jingle', {xmlns: 'urn:xmpp:jingle:1'})
						.attrs({ action: "transport-info",  sid, ufrag, pwd })
						.c('candidate', jsoncandidate)
						.up()
					.up();

			var id = connection.sendIQ(iq, () => {
				console.log("===> xmpp transport-info ok sid:" + sid);		
			},(error) => {
				console.warn("############transport-info error sid:" + sid + " error:" + error.textContent);
			});
		});	

		var id = connection.sendIQ(answer, () => {
			console.log("===> xmpp session-accept ok sid:" + sid);
			this.emitState(roomid + '/' + name, "published");			
		},(error) => {
			console.warn("############session-accept error sid:" + sid + " error:" + error.textContent);
		});	
	}

	/*
	/* HTTP callback for  /call 
	*/	
	XMPPVideoRoom.prototype.onCall = function(connection, roomid, name, iq, data) {		
		var jingle = iq.querySelector("jingle");
		var sid = jingle.getAttribute("sid");
		console.log("<=== webrtc answer sid:" + sid);		
		
		var earlyCandidates = this.sessionList[sid].earlyCandidates;
		if (earlyCandidates) {
			while (earlyCandidates.length) {
					var candidate = earlyCandidates.shift();
					console.log("===> webrtc candidate :" + JSON.stringify(candidate));
					var method = this.srvurl + "/api/addIceCandidate?peerid="+ sid;
					fetch(method, { method: "POST", body: JSON.stringify(candidate) })
						.then( (response) => (response.json()) )
						.then( (response) => console.log("method:"+method+ " answer:" +response) )
						.catch( (error) => this.onError("addIceCandidate error:" + error ))	
			}
		}
		delete this.sessionList[sid].earlyCandidates;
				
		var jingle = $iq({ type: "set",  from: iq.getAttribute("to"), to: iq.getAttribute("from") })
						.c('jingle', {xmlns: 'urn:xmpp:jingle:1'})
							.attrs({ action: "session-accept",  sid, responder:iq.getAttribute("to") });

		var sdp = new SDP(data.sdp);
		var answer = sdp.toJingle(jingle, 'initiator'); 
		fetch(this.srvurl + "/api/getIceCandidate?peerid="+ sid)
			.then(r => r.json())
			.then( (response) => this.onReceiveCandidate(connection, roomid, name, answer.node, response) )
			.catch( error =>  this.onError("getIceCandidate error:" + error) )
	}
	
	XMPPVideoRoom.prototype.emitState = function(name, state) {
		if (this.bus) {
			this.bus.emit('state', name, state);
		}
	}

	XMPPVideoRoom.prototype.emitPresence = function(name, state) {
		if (this.bus) {
			this.bus.emit('presence', name, state);
		}
	}	
	
	XMPPVideoRoom.prototype.onError = function (error) {
		console.warn("############onError:" + error)
	}
	
	XMPPVideoRoom.prototype.getRoomUrl = function (roomId) {
		return roomId + "@" + "conference." + this.xmppUrl
	}

	/*
	/* XMPP callback for  jingle 
	*/		
	XMPPVideoRoom.prototype.OnJingle = function(connection, roomid, name, url, iq) {
		var jingle = iq.querySelector("jingle");
		var sid = jingle.getAttribute("sid");
		var action = jingle.getAttribute("action");
		const fromJid = iq.getAttribute('from');
		const toJid = iq.getAttribute('to');
		console.log("OnJingle from:" + fromJid + " to:" + toJid + " sid:" + sid + " action:" + action)
		console.log(iq.innerHTML);

		var id = iq.getAttribute("id");
		var ack = $iq({ type: "result", to: fromJid, id })

		if (action === "session-initiate") 	{
			connection.sendIQ(ack);	

			const resource = Strophe.getResourceFromJid(fromJid);
			const isP2P = (resource !== 'focus');
			console.log("<=== xmpp offer sid:" + sid + " resource:" + resource + " initiator:" + jingle.getAttribute("initiator"));

			if (!isP2P) {
				this.emitState(roomid + '/' + name, "publishing");

				var sdp = new SDP('');
				sdp.fromJingle($(jingle));
				
				this.sessionList[sid] = { roomid, name, earlyCandidates:[] } ;

				var videourl = url.video || url;
				var method = this.srvurl + "/api/call?peerid="+ sid +"&url="+encodeURIComponent(videourl);
				if (url.audio) {
					method += "&audio="+encodeURIComponent(url.audio);
				}
				method += "&options="+encodeURIComponent("rtptransport=tcp&timeout=60");	
				fetch(method, { method: "POST", body: JSON.stringify({type:"offer",sdp:sdp.raw}) })
					.then( (response) => response.json() )
					.then( (response) => this.onCall(connection, roomid, name, iq, response ) )
					.catch( (error) => this.onError("call " + error ))

			}
				
		} else if (action === "transport-info") {
			connection.sendIQ(ack);		

			console.log("<=== xmpp candidate sid:" + sid);
			
			if (this.sessionList[sid]) {
				var contents = $(jingle).find('>content');
				contents.each( (contentIdx,content) => {
					var transports = $(content).find('>transport');
					transports.each( (idx,transport) => {
						var ufrag = transport.getAttribute('ufrag');
						var candidates = $(transport).find('>candidate');
						candidates.each ( (idx,candidate) => {
							var sdp = SDPUtil.candidateFromJingle(candidate);
							sdp = sdp.replace("a=candidate","candidate");
							sdp = sdp.replace("\r\n"," ufrag " + ufrag);
							var candidate = { candidate:sdp, sdpMid:"", sdpMLineIndex:contentIdx }
				
							if (this.sessionList[sid].earlyCandidates) {
								console.log("queue candidate waiting for call answer");
								this.sessionList[sid].earlyCandidates.push(candidate);
							} else {
								console.log("===> webrtc candidate :" + JSON.stringify(candidate));
								var method = this.srvurl + "/api/addIceCandidate?peerid="+ sid;

								fetch(method, { method: "POST", body: JSON.stringify(candidate) })
									.then( (response) => (response.json()) )
									.then( (response) => console.log("method:"+method+ " answer:" +response) )
									.catch( (error) => this.onError("call " + error ))		
							}							
						});
					});
				});	
			}
		} else if (action === "session-terminate") {			
			connection.sendIQ(ack);		
			console.log("<=== xmpp session-terminate sid:" + sid + " reason:" + jingle.querySelector("reason").textContent);
			this.leaveSession(sid);
		} else {			
			connection.sendIQ(ack);		
			console.log("<=== unknown action");
		}		
					
		return true;		
	}
	
	/*
	/* XMPP callback for  presence 
	*/		
	XMPPVideoRoom.prototype.OnPresence = function(connection,roomid,pres)
	{
		const resource = Strophe.getResourceFromJid(pres.getAttribute('from'));
		var msg = "resource:" + resource;
		
		const type = pres.getAttribute('type');
		if (type) {
			msg += " type:" + type;
		}
		const statusEl = pres.getElementsByTagName('status')[0];
		if (statusEl) {
			msg += " status:" + statusEl.getAttribute('code');
		}

		if (type == "unavailable") {
			this.emitPresence(roomid + '/' + resource, "out");
		} else if (statusEl) {
			var code = statusEl.getAttribute('code');
			msg += " status:" + code; 
			if (code === "100" || code === "110" || code === "201" || code === "210") {
					this.emitPresence(roomid + '/'  + resource, "in");
			}
			else if (code === "301" || code === "307") {
					this.emitPresence(roomid + '/' + resource, "out");
			}
		} else {
			this.emitPresence(roomid + '/'  + resource, "in");
		}

        const xElement = pres.getElementsByTagNameNS('http://jabber.org/protocol/muc#user', 'x')[0];
		const mucUserItem = xElement && xElement.getElementsByTagName('item')[0];
		if (mucUserItem) {
			msg += " jid:" + mucUserItem.getAttribute('jid') 
						+ " role:" + mucUserItem.getAttribute('role')
						+ " affiliation:" + mucUserItem.getAttribute('affiliation');
		}

		const nickEl = pres.getElementsByTagName('nick')[0];
		if (nickEl) {
			msg += " nick:" + nickEl.textContent; 
		}
		console.log ( "OnPresence " + msg); 							

		return true;		
	}

	/*
	/* leave a session
	*/	
	XMPPVideoRoom.prototype.leaveSession = function (sid) {
		var session = this.sessionList[sid];
		if (session) {
			var roomId = session.roomid
			this.emitState(roomId + '/' + session.name, "leaving");

			this.getConnection(roomId, session.name).then( connection => {
				var roomUrl = this.getRoomUrl(roomId);

				// close jingle session
				var iq = $iq({ type: "set",  from: roomUrl +"/" + session.name, to: roomUrl })
								.c('jingle', {xmlns: 'urn:xmpp:jingle:1'})
									.attrs({ action: "session-terminate",  sid})
								.up();	
				connection.sendIQ(iq);

				// publish on muc
				connection.muc.leave(roomUrl, session.name, this.OnPresence.bind(this,connection,roomId));
				connection.flush();

				// close WebRTC session
				fetch(this.srvurl + "/api/hangup?peerid="+ sid)
					.then(r => r.json())
					.then( (response) => { 
						console.log("hangup answer:" +response);
					})
					.catch(error => {
						this.onError("hangup error:" + error);
					})
			
				this.emitState(roomId + '/' + session.name, "leaved");

				// unregister session
				delete this.sessionList[sid];
			})				

		}
	}


	return XMPPVideoRoom;
})();

if (typeof window !== 'undefined' && typeof window.document !== 'undefined') {
	window.XMPPVideoRoom = XMPPVideoRoom;
}
if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = XMPPVideoRoom;
}	


