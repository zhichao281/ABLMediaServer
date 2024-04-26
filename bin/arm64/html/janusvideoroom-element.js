import "./libs/request.min.js";
import "./janusvideoroom.js";
import "./libs/EventEmitter.min.js";

class WebRTCStreamerJanusElement extends HTMLElement {
	static get observedAttributes() {
		return ['videostream','audiostream','options', 'webrtcurl'];
	}  
	
	constructor() {
		super(); 
		this.shadowDOM = this.attachShadow({mode: 'open'});
		this.shadowDOM.innerHTML = `
					<style>@import "styles.css"</style>
					<body></body>
					`;
	}
	connectedCallback() {
		this.connectStream();
	}
	disconnectedCallback() {
		this.disconnectStream();
	}
	attributeChangedCallback(attrName, oldVal, newVal) {
		this.connectStream();
	}
	
	writeStatus(name, status) {
		var textNode = this.shadowDOM.createElement("h3");
		textNode.innerHTML = name + " " + status;
		this.shadowDOM.body.appendChild(textNode);
	}
		
	disconnectStream() {
		if (this.janus) {
			this.janus.leave();
			this.janus = null;
		}
	}
	connectStream() {
		this.disconnectStream();
		
		let webrtcurl = this.getAttribute("webrtcurl") || webrtcConfig.url;
		let videostream = this.getAttribute("videostream") || webrtcConfig.defaultvideostream;
		let audiostream = this.getAttribute("audiostream") || webrtcConfig.defaultaudiostream;
		let options = this.getAttribute("options") || webrtcConfig.options;
		
		
		var bus = new EventEmitter();
		bus.addListener('state', writeStatus);		
	
		this.janus = new JanusVideoRoom(janusRoomConfig.url, webrtcurl, bus);
		this.janus.join(janusRoomConfig.roomId, {video:videostream}, videostream);
	}
	
	
}

customElements.define('webrtc-streamer-janus', WebRTCStreamerJanusElement);
