var JanusVideoRoom = (function() {

	
/** 
 * Interface with Janus Gateway Video Room and WebRTC-streamer API
 * @constructor
 * @param {string} janusUrl - url of Janus Gateway
 * @param {string} srvurl - url of WebRTC-streamer
*/
var JanusVideoRoom = function JanusVideoRoom (janusUrl, srvurl, bus) {	
	this.janusUrl    = janusUrl;
	this.handlers    = [];
	this.srvurl      = srvurl || location.protocol+"//"+window.location.hostname+":"+window.location.port;
	this.connection  = [];
	this.bus = bus;
};
	
JanusVideoRoom.prototype._handleHttpErrors = function (response) {
    if (!response.ok) {
        throw Error(response.statusText);
    }
    return response;
}

/** 
* Ask to publish a stream from WebRTC-streamer in a Janus Video Room user
 * @param {string} janusroomid - id of the Janus Video Room to join
 * @param {string} url - WebRTC stream to publish
 * @param {string} name - name in  Janus Video Room
*/
JanusVideoRoom.prototype.join = function(janusroomid, url, name) {
	// create a session
	var createReq = {janus: "create", transaction: Math.random().toString() };

	fetch(this.janusUrl, { method: "POST", body: JSON.stringify(createReq) })
	.then(this._handleHttpErrors)
	.then( (response) => (response.json()) )
	.then( (response) => this.onCreateSession(response, janusroomid, url, name))
	.catch( (error) => this.onError("create " + error ))

}

/**
* Ask to unpublish a stream from WebRTC-streamer in a Janus Video Room user
 * @param {string} janusroomid - id of the Janus Video Room to join
 * @param {string} url - WebRTC stream to publish
 * @param {string} name - name in  Janus Video Room
*/
JanusVideoRoom.prototype.leave = function(janusroomid, url, name) {
	var connection = this.connection[janusroomid + "_" + url + "_" + name];
	if (connection) {
		var sessionId = connection.sessionId;
		var pluginid  = connection.pluginId;
		
		var leaveReq = { "janus": "message", "body": {"request": "unpublish"}, "transaction": Math.random().toString() };
		
		
		fetch(this.janusUrl + "/" + sessionId + "/" + pluginid, { method: "POST", body: JSON.stringify(leaveReq) })
			.then(this._handleHttpErrors)
			.then( (response) => (response.json()) )
			.then( (response) => console.log("leave janus room answer:" + response) )
			.catch( (error) => this.onError("leave " + error ))
	}
}


JanusVideoRoom.prototype.emit = function(name, state) {
	if (this.bus) {
		this.bus.emit('state', name, state);
	}
}

// ------------------------------------------
// Janus callback for Session Creation
// ------------------------------------------
JanusVideoRoom.prototype.onCreateSession = function(dataJson, janusroomid, url, name) {
	var sessionId = dataJson.data.id;
	console.log("onCreateSession sessionId:" + sessionId);
	
	// attach to video room plugin
	var attachReq = { "janus": "attach", "plugin": "janus.plugin.videoroom", "transaction": Math.random().toString() };			
	
	fetch(this.janusUrl + "/" + sessionId, { method: "POST", body: JSON.stringify(attachReq) })
		.then(this._handleHttpErrors)
		.then( (response) => (response.json()) )
		.then( (response) => this.onPluginsAttached(response, janusroomid, url, name, sessionId) )
		.catch( (error) => this.onError("attach " + error ))

}
	
// ------------------------------------------
// Janus callback for Video Room Plugins Connection
// ------------------------------------------
JanusVideoRoom.prototype.onPluginsAttached = function(dataJson, janusroomid, url, name, sessionId) {
	var pluginid = dataJson.data.id;
	console.log("onPluginsAttached pluginid:" + pluginid);
	
	this.emit(name, "joining");

	var joinReq = {"janus":"message","body":{"request":"join","room":janusroomid,"ptype":"publisher","display":name},"transaction":Math.random().toString()};
	
	fetch(this.janusUrl + "/" + sessionId + "/" + pluginid, { method: "POST", body: JSON.stringify(joinReq) })
		.then(this._handleHttpErrors)
		.then( (response) => (response.json()) )
		.then( (response) => this.onJoinRoom(response, janusroomid, url, name, sessionId, pluginid) )
		.catch( (error) => this.onError("join " + error ))

}

// ------------------------------------------
// Janus callback for Video Room Joined
// ------------------------------------------
JanusVideoRoom.prototype.onJoinRoom = function(dataJson,janusroomid,url,name,sessionId,pluginid) {
	console.log("onJoinRoom:" + JSON.stringify(dataJson));

	fetch(this.janusUrl + "/" + sessionId + "?rid=" + new Date().getTime() + "&maxev=1")
		.then(this._handleHttpErrors)
		.then( (response) => (response.json()) )
		.then( (response) => this.onJoinRoomResult(response, janusroomid, url, name, sessionId, pluginid) )
		.catch( (error) => this.onError("join anwser " + error ))
}

// ------------------------------------------
// Janus callback for Video Room Joined
// ------------------------------------------
JanusVideoRoom.prototype.onJoinRoomResult = function(dataJson,janusroomid,url,name,sessionId,pluginid) {
	console.log("onJoinRoomResult:" + JSON.stringify(dataJson));

	if (dataJson.plugindata.data.videoroom === "joined") {	
		// register connection
		this.connection[janusroomid + "_" + url + "_" + name] = {"sessionId":sessionId, "pluginId": pluginid };

		// member of the room
		var publishers = dataJson.plugindata.data.publishers;
		for (var i=0; i<publishers.length; i++) {
			var publisher = publishers[i];
			this.emit(publisher.display, "up");
		}
		
		if (name) {
			// notify new state
			this.emit(name, "joined");
			
			var peerid = Math.random().toString();
			
			var videourl = url.video || url;
			var createOfferUrl = this.srvurl + "/api/createOffer?peerid="+ peerid+"&url="+encodeURIComponent(videourl);
			if (url.audio) {
				createOfferUrl += "&audiourl="+encodeURIComponent(url.audio);
			}
			if (url.options) {
				createOfferUrl += "&options="+encodeURIComponent(url.options);
			}	
			
			fetch(createOfferUrl)
				.then(this._handleHttpErrors)
				.then( (response) => (response.json()) )
				.then( (response) => this.onCreateOffer(response,  name, sessionId, pluginid, peerid) )
				.catch( (error) => this.onError("createOffer " + error ))
	
		} else {
			// start long polling
			this.longpoll(null, name, sessionId);	
		}
	} else {
		this.emit(name, "joining room failed");
	}
}

// ------------------------------------------
// WebRTC streamer callback for Offer 
// ------------------------------------------
JanusVideoRoom.prototype.onCreateOffer = function(dataJson,name,sessionId,pluginid,peerid) {
	console.log("onCreateOffer:" + JSON.stringify(dataJson));
	
	this.emit(name, "publishing");
	
	var publishReq = { "janus": "message", "body": {"request": "publish", "video": true, "audio": true, "data": true}, "jsep": dataJson, "transaction": Math.random().toString() };		

	fetch(this.janusUrl + "/" + sessionId + "/" + pluginid, { method: "POST", body: JSON.stringify(publishReq) })
		.then(this._handleHttpErrors)
		.then( (response) => (response.json()) )
		.then( (response) => this.onPublishStream(response, name, sessionId, pluginid, peerid) )
		.catch( (error) => this.onError("publish " + error ))
	
}

// ------------------------------------------
// Janus callback for WebRTC stream is published
// ------------------------------------------
JanusVideoRoom.prototype.onPublishStream = function(dataJson,name,sessionId,pluginid,peerid) {
	console.log("onPublishStream:" + JSON.stringify(dataJson));

	fetch(this.janusUrl + "/" + sessionId + "?rid=" + new Date().getTime() + "&maxev=1")
		.then(this._handleHttpErrors)
		.then( (response) => (response.json()) )
		.then( (response) => this.onPublishStreamResult(response, name, sessionId, pluginid, peerid) )
		.catch( (error) => this.onError("publish anwser " + error ))
		
}

// ------------------------------------------
// Janus callback for WebRTC stream is published
// ------------------------------------------
JanusVideoRoom.prototype.onPublishStreamResult = function(dataJson,name,sessionId,pluginid,peerid) {
	console.log("onPublishStreamResult:" + JSON.stringify(dataJson));

	if (dataJson.jsep) {
		fetch(this.srvurl + "/api/setAnswer?peerid="+ peerid, { method: "POST", body: JSON.stringify(dataJson.jsep) })
			.then(this._handleHttpErrors)
			.then( (response) => (response.json()) )
			.then( (response) => this.onSetAnswer(response, name, sessionId, pluginid, peerid) )
			.catch( (error) => this.onError("setAnswer " + error ))
		
	} else {
		this.emit(name, "publishing failed (no SDP)");
	}
}

// ------------------------------------------
// WebRTC streamer callback for Answer 
// ------------------------------------------
JanusVideoRoom.prototype.onSetAnswer = function(dataJson,name,sessionId,pluginid,peerid) {
	console.log("onSetAnswer:" + JSON.stringify(dataJson));
	
	fetch(this.srvurl + "/api/getIceCandidate?peerid="+peerid)
		.then(this._handleHttpErrors)
		.then( (response) => (response.json()) )
		.then( (response) => this.onReceiveCandidate(response, name, sessionId, pluginid) )
		.catch( (error) => this.onError("getIceCandidate " + error ))
	
}

// ------------------------------------------
// WebRTC streamer callback for ICE candidate 
// ------------------------------------------
JanusVideoRoom.prototype.onReceiveCandidate = function(dataJson,name,sessionId,pluginid) {
	console.log("onReceiveCandidate answer:" + JSON.stringify(dataJson));
	
	for (var i=0; i<dataJson.length; i++) {
		// send ICE candidate to Janus
		var candidateReq = { "janus": "trickle", "candidate": dataJson[i], "transaction": Math.random().toString()  };
		
		fetch(this.janusUrl + "/" + sessionId + "/" + pluginid, { method: "POST", body: JSON.stringify(candidateReq) })
			.then(this._handleHttpErrors)
			.then( (response) => (response.json()) )
			.then( (response) => console.log("onReceiveCandidate janus answer:" + JSON.stringify(response)) )
			.catch( (error) => this.onError("setAnswer " + error ))
	
	}
	
	// start long polling
	this.longpoll(null, name, sessionId);	
}

// ------------------------------------------
// Janus callback for keepAlive Session
// ------------------------------------------
JanusVideoRoom.prototype.keepAlive = function(sessionId) {
	var keepAliveReq = { "janus": "keepalive", "session_id": sessionId, "transaction": Math.random().toString()  };
	
	fetch(this.janusUrl + "/" + sessionId, { method: "POST", body: JSON.stringify(keepAliveReq) })
		.then(this._handleHttpErrors)
		.then( (response) => (response.json()) )
		.then( (response) => console.log("keepAlive answer:" + JSON.stringify(response)) )
		.catch( (error) => this.onError("keepAlive " + error ))
	
}

// ------------------------------------------
// Janus callback for Long Polling
// ------------------------------------------
JanusVideoRoom.prototype.longpoll = function(dataJson, name, sessionId) {
	if (dataJson) {
		console.log("poll evt:" + JSON.stringify(dataJson));
	
		if (dataJson.janus === "webrtcup") {
			// notify connection
			this.emit(name, "up");
			
			// start keep alive
			var bind = this;
			setInterval( function() { bind.keepAlive(sessionId); }, 10000);	
		}
		else if (dataJson.janus === "hangup") {
			// notify connection
			this.emit(name, "down");
		}
		else if (dataJson.janus === "event") {
			// member of the room
			var publishers = dataJson.plugindata.data.publishers;
			if (publishers) {
				for (var i=0; i<publishers.length; i++) {
					var publisher = publishers[i];
					this.emit(publisher.display, "up");
				}	
			}
		}
	}
	
	fetch(this.janusUrl + "/" + sessionId + "?rid=" + new Date().getTime() + "&maxev=1")
		.then(this._handleHttpErrors)
		.then( (response) => (response.json()) )
		.then( (response) => this.longpoll(response, name, sessionId) )
		.catch( (error) => this.onError("longpoll anwser " + error ))
	
}

// ------------------------------------------
// Janus callback for Error
// ------------------------------------------
JanusVideoRoom.prototype.onError = function(status) {
	console.log("onError:" + status);
}


return JanusVideoRoom;
})();

if (typeof window !== 'undefined' && typeof window.document !== 'undefined') {
	window.JanusVideoRoom = JanusVideoRoom;
}
if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = JanusVideoRoom;
}	
	