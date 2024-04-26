

class WebRTCStreamerAlgoElement extends HTMLElement {	
	static get observedAttributes() {
		return ['selected'];
	}  	
	constructor() {
		super(); 
		this.shadowDOM = this.attachShadow({mode: 'open'});
		this.shadowDOM.innerHTML = `
					<style>@import "styles.css"</style>
                    <h2>
                        <select id="algoList">
                            <option value="none">none</option>
                            <option value="cocossd">cocossd</option>
                            <option value="posenet">posenet</option>
							<option value="deeplab">deeplab</option>
							<option value="bodyPix">bodyPix</option>
							<option value="blazeface">blazeface</option>
                        </select>
                    </h2>
					`;
	}
	connectedCallback() {
		this.fillList();
	}
	
	attributeChangedCallback(attrName, oldVal, newVal) {
		if (attrName === "selected") {
			this.selected = newVal;
			let mediaList = this.shadowDOM.getElementById("algoList");
			for (const option of mediaList.getElementsByTagName('option')) {
				if (option.value === newVal) {
					option.selected = true;
				}
			}
		}
	}	

	fillList() {
		let algoList = this.shadowDOM.getElementById("algoList");
		algoList.onchange = (event) => {
				this.dispatchEvent(new CustomEvent('change', {
					detail: {
                        algo: algoList.selectedOptions[0].value,
					}
				  }));		
			}
	}
}

customElements.define('webrtc-streamer-algo', WebRTCStreamerAlgoElement);
