var WebRtcStreamer = (function() {

/** 
 * Interface with WebRTC-streamer API
 * @constructor
 * @param {string} videoElement - id of the video element tag
 * @param {string} srvurl -  url of webrtc-streamer (default is current location)
*/
var WebRtcStreamer = function WebRtcStreamer (videoElement, srvurl) {
	if (typeof videoElement === "string") {
		this.videoElement = document.getElementById(videoElement);
	} else {
		this.videoElement = videoElement;
	}
	this.srvurl           = srvurl || location.protocol+"//"+window.location.hostname+":"+window.location.port;
	this.pc               = null;    

	this.mediaConstraints = { offerToReceiveAudio: true, offerToReceiveVideo: true };

	this.iceServers = null;
	this.earlyCandidates = [];
}

WebRtcStreamer.prototype._handleHttpErrors = function (response) {
    if (!response.ok) {
        throw Error(response.statusText);
    }
    return response;
}

/** 
 * Connect a WebRTC Stream to videoElement 
 * @param {string} videourl - id of WebRTC video stream
 * @param {string} audiourl - id of WebRTC audio stream
 * @param {string} options -  options of WebRTC call
 * @param {string} stream  -  local stream to send
*/
WebRtcStreamer.prototype.connect = function(videourl, audiourl, options, localstream) {
	this.disconnect();
	
	// getIceServers is not already received
	if (!this.iceServers) {
		console.log("Get IceServers");
		
		fetch(this.srvurl + "/api/getIceServers")
			.then(this._handleHttpErrors)
			.then( (response) => (response.json()) )
			.then( (response) =>  this.onReceiveGetIceServers(response, videourl, audiourl, options, localstream))
			.catch( (error) => this.onError("getIceServers " + error ))
				
	} else {
		this.onReceiveGetIceServers(this.iceServers, videourl, audiourl, options, localstream);
	}
}

/** 
 * Disconnect a WebRTC Stream and clear videoElement source
*/
WebRtcStreamer.prototype.disconnect = function() {		
	if (this.videoElement?.srcObject) {
		this.videoElement.srcObject.getTracks().forEach(track => {
			track.stop()
			this.videoElement.srcObject.removeTrack(track);
		});
	}
	if (this.pc) {
		fetch(this.srvurl + "/api/hangup?peerid=" + this.pc.peerid)
			.then(this._handleHttpErrors)
			.catch( (error) => this.onError("hangup " + error ))

		
		try {
			this.pc.close();
		}
		catch (e) {
			console.log ("Failure close peer connection:" + e);
		}
		this.pc = null;
	}
}    

/*
* GetIceServers callback
*/
WebRtcStreamer.prototype.onReceiveGetIceServers = function(iceServers, videourl, audiourl, options, stream) {
	this.iceServers       = iceServers;
	this.pcConfig         = iceServers || {"iceServers": [] };
	try {            
		this.createPeerConnection();

		var callurl = this.srvurl + "/api/call?peerid=" + this.pc.peerid + "&url=" + encodeURIComponent(videourl);
		if (audiourl) {
			callurl += "&audiourl="+encodeURIComponent(audiourl);
		}
		if (options) {
			callurl += "&options="+encodeURIComponent(options);
		}
		
		if (stream) {
			this.pc.addStream(stream);
		}

                // clear early candidates
		this.earlyCandidates.length = 0;
		
		// create Offer
		this.pc.createOffer(this.mediaConstraints).then((sessionDescription) => {
			console.log("Create offer:" + JSON.stringify(sessionDescription));
			
			this.pc.setLocalDescription(sessionDescription)
				.then(() => {
					fetch(callurl, { method: "POST", body: JSON.stringify(sessionDescription) })
						.then(this._handleHttpErrors)
						.then( (response) => (response.json()) )
						.catch( (error) => this.onError("call " + error ))
						.then( (response) =>  this.onReceiveCall(response) )
						.catch( (error) => this.onError("call " + error ))
				
				}, (error) => {
					console.log ("setLocalDescription error:" + JSON.stringify(error)); 
				});
			
		}, (error) => { 
			alert("Create offer error:" + JSON.stringify(error));
		});

	} catch (e) {
		this.disconnect();
		alert("connect error: " + e);
	}	    
}


WebRtcStreamer.prototype.getIceCandidate = function() {
	fetch(this.srvurl + "/api/getIceCandidate?peerid=" + this.pc.peerid)
		.then(this._handleHttpErrors)
		.then( (response) => (response.json()) )
		.then( (response) =>  this.onReceiveCandidate(response))
		.catch( (error) => this.onError("getIceCandidate " + error ))
}
					
/*
* create RTCPeerConnection 
*/
WebRtcStreamer.prototype.createPeerConnection = function() {
	console.log("createPeerConnection  config: " + JSON.stringify(this.pcConfig));
	this.pc = new RTCPeerConnection(this.pcConfig);
	var pc = this.pc;
	pc.peerid = Math.random();		
	
	pc.onicecandidate = (evt) => this.onIceCandidate(evt);
	pc.onaddstream    = (evt) => this.onAddStream(evt);
	pc.oniceconnectionstatechange = (evt) => {  
		console.log("oniceconnectionstatechange  state: " + pc.iceConnectionState);
		if (this.videoElement) {
			if (pc.iceConnectionState === "connected") {
				this.videoElement.style.opacity = "1.0";
			}			
			else if (pc.iceConnectionState === "disconnected") {
				this.videoElement.style.opacity = "0.25";
			}			
			else if ( (pc.iceConnectionState === "failed") || (pc.iceConnectionState === "closed") )  {
				this.videoElement.style.opacity = "0.5";
			} else if (pc.iceConnectionState === "new") {
				this.getIceCandidate();
			}
		}
	}
	pc.ondatachannel = function(evt) {  
		console.log("remote datachannel created:"+JSON.stringify(evt));
		
		evt.channel.onopen = function () {
			console.log("remote datachannel open");
			this.send("remote channel openned");
		}
		evt.channel.onmessage = function (event) {
			console.log("remote datachannel recv:"+JSON.stringify(event.data));
		}
	}
	pc.onicegatheringstatechange = function() {
		if (pc.iceGatheringState === "complete") {
			const recvs = pc.getReceivers();
		
			recvs.forEach((recv) => {
			  if (recv.track && recv.track.kind === "video") {
				console.log("codecs:" + JSON.stringify(recv.getParameters().codecs))
			  }
			});
		  }
	}

	try {
		var dataChannel = pc.createDataChannel("ClientDataChannel");
		dataChannel.onopen = function() {
			console.log("local datachannel open");
			this.send("local channel openned");
		}
		dataChannel.onmessage = function(evt) {
			console.log("local datachannel recv:"+JSON.stringify(evt.data));
		}
	} catch (e) {
		console.log("Cannor create datachannel error: " + e);
	}	
	
	console.log("Created RTCPeerConnnection with config: " + JSON.stringify(this.pcConfig) );
	return pc;
}


/*
* RTCPeerConnection IceCandidate callback
*/
WebRtcStreamer.prototype.onIceCandidate = function (event) {
	if (event.candidate) {
		if (this.pc.currentRemoteDescription)  {
			this.addIceCandidate(this.pc.peerid, event.candidate);					
		} else {
			this.earlyCandidates.push(event.candidate);
		}
	} 
	else {
		console.log("End of candidates.");
	}
}


WebRtcStreamer.prototype.addIceCandidate = function(peerid, candidate) {
	fetch(this.srvurl + "/api/addIceCandidate?peerid="+peerid, { method: "POST", body: JSON.stringify(candidate) })
		.then(this._handleHttpErrors)
		.then( (response) => (response.json()) )
		.then( (response) =>  {console.log("addIceCandidate ok:" + response)})
		.catch( (error) => this.onError("addIceCandidate " + error ))
}
				
/*
* RTCPeerConnection AddTrack callback
*/
WebRtcStreamer.prototype.onAddStream = function(event) {
	console.log("Remote track added:" +  JSON.stringify(event));
	
	this.videoElement.srcObject = event.stream;
	var promise = this.videoElement.play();
	if (promise !== undefined) {
	  promise.catch((error) => {
		console.warn("error:"+error);
		this.videoElement.setAttribute("controls", true);
	  });
	}
}
		
/*
* AJAX /call callback
*/
WebRtcStreamer.prototype.onReceiveCall = function(dataJson) {

	console.log("offer: " + JSON.stringify(dataJson));
	var descr = new RTCSessionDescription(dataJson);
	this.pc.setRemoteDescription(descr).then(() =>  { 
			console.log ("setRemoteDescription ok");
			while (this.earlyCandidates.length) {
				var candidate = this.earlyCandidates.shift();
				this.addIceCandidate(this.pc.peerid, candidate);				
			}
		
			this.getIceCandidate()
		}
		, (error) => { 
			console.log ("setRemoteDescription error:" + JSON.stringify(error)); 
		});
}	

/*
* AJAX /getIceCandidate callback
*/
WebRtcStreamer.prototype.onReceiveCandidate = function(dataJson) {
	console.log("candidate: " + JSON.stringify(dataJson));
	if (dataJson) {
		for (var i=0; i<dataJson.length; i++) {
			var candidate = new RTCIceCandidate(dataJson[i]);
			
			console.log("Adding ICE candidate :" + JSON.stringify(candidate) );
			this.pc.addIceCandidate(candidate).then( () =>      { console.log ("addIceCandidate OK"); }
				, (error) => { console.log ("addIceCandidate error:" + JSON.stringify(error)); } );
		}
		this.pc.addIceCandidate();
	}
}


/*
* AJAX callback for Error
*/
WebRtcStreamer.prototype.onError = function(status) {
	console.log("onError:" + status);
}

return WebRtcStreamer;
})();

if (typeof window !== 'undefined' && typeof window.document !== 'undefined') {
	window.WebRtcStreamer = WebRtcStreamer;
}
if (typeof module !== 'undefined' && typeof module.exports !== 'undefined') {
	module.exports = WebRtcStreamer;
}
