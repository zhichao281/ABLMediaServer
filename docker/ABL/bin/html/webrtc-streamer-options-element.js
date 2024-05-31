
class WebRTCStreamerOptionsElement extends HTMLElement {	
	constructor() {
		super(); 
		this.shadowDOM = this.attachShadow({mode: 'open'});
		this.shadowDOM.innerHTML = `
					<style>@import "styles.css"</style>
					<h2>Options</h2>
					<div id="options"></div>
					<button id="apply">Apply</button>
					`;
		this.button = this.shadowDOM.getElementById("apply");
		this.button.onclick = () => this.notifyOptions();
		this.params = new URLSearchParams();
	}

	static get observedAttributes() {
		return ['options'];
	}  

	attributeChangedCallback(attrName, oldVal, newVal) {
		if (attrName === "options") {
			this.fillOptions();
		}
	}

	connectedCallback() {
		this.fillOptions();
	}

	fillOptions() {
		const options = this.getAttribute("options") || "";
		this.params = new URLSearchParams(options);

		let optElement = this.shadowDOM.getElementById("options");
		optElement.innerHTML = "";

		for (let [k,v] of this.params.entries()) {
			let label = document.createTextNode(k+":");
			optElement.appendChild(label);			
			let input = document.createElement("input");
			input.type = "text";
			input.value = v;
			input.onchange = () => this.params.set(k, input.value);
			optElement.appendChild(input);		
			optElement.appendChild(document.createElement("br"));		
		}	
	}

	notifyOptions()
	{
		this.setAttribute("options", this.params.toString());
		this.dispatchEvent(new CustomEvent('change', {
			detail: {
		  		options: this.params.toString(),
			}
	  	}));
	}
}

customElements.define('webrtc-streamer-options', WebRTCStreamerOptionsElement);
