class WebRTCStreamerSelectorElement extends HTMLElement {	
	static get observedAttributes() {
		return ['selected', 'webrtcurl'];
	}  	
	constructor() {
		super(); 
		this.shadowDOM = this.attachShadow({mode: 'open'});
		this.shadowDOM.innerHTML = `
					<style>@import "styles.css"</style>
					<h2><select id="mediaList"></select></h2>
					`;
	}
	connectedCallback() {
		this.fillList();
	}
	
	attributeChangedCallback(attrName, oldVal, newVal) {
		if (attrName === "selected") {
			this.selected = newVal;
			let mediaList = this.shadowDOM.getElementById("mediaList");
			for (const option of mediaList.getElementsByTagName('option')) {
				if (option === newVal) {
					option.selected = true;
				}
			}

		} else if (attrName === "webrtcurl") {
			this.fillList();
		}
	}	

	fillList() {
		let mediaList = this.shadowDOM.getElementById("mediaList");
		const webrtcurl = this.getAttribute("webrtcurl") || "";
		fetch(webrtcurl + "/api/getMediaList").then(r => r.json()).then( (response) => { 
			response.forEach( (media) => {
				var newOption = document.createElement("option");
				newOption.text = media.video;
				newOption.value = JSON.stringify(media);
				if (this.selected && (this.selected === newOption.text) ) {
					newOption.selected = true;
				}
				mediaList.appendChild(newOption);
			});

			if (mediaList.selectedOptions.length > 0) {
				this.dispatchEvent(new CustomEvent('change', {
					detail: {
					  url: mediaList.selectedOptions[0].value,
					}
				  }));				
			}	
		});	

		mediaList.onchange = (event) => {
				this.dispatchEvent(new CustomEvent('change', {
					detail: {
					  url: mediaList.selectedOptions[0].value,
					}
				  }));		
			}
	}
}

customElements.define('webrtc-streamer-selector', WebRTCStreamerSelectorElement);
