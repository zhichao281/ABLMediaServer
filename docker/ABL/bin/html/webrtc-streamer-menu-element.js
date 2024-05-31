
class WebRTCStreamerMenuElement extends HTMLElement {
	static get observedAttributes() {
		return ['selected', 'webrtcurl'];
	}
	constructor() {
		super();
		this.shadowDOM = this.attachShadow({ mode: 'open' });
		this.shadowDOM.innerHTML = `
					<style>@import "styles.css"</style>
					<nav id="mediaList"></nav>
					`;
	}
	connectedCallback() {
		this.fillList();
	}

	attributeChangedCallback(attrName, oldVal, newVal) {
		if (attrName === "selected") {
			this.selected = newVal;
			let mediaList = this.shadowDOM.getElementById("mediaList");
			let newValjson = JSON.parse(newVal);
			for (const option of mediaList.getElementsByTagName('a')) {
				let optionjson = JSON.parse(option.value);
				if (optionjson.video === newValjson.video) {
					option.selected = true;
					option.className = "active";
				}
			}

		} else if (attrName === "webrtcurl") {
			this.fillList();
		}
	}

	fillList() {
		let mediaList = this.shadowDOM.getElementById("mediaList");
		const webrtcurl = this.getAttribute("webrtcurl") || "";
		fetch(webrtcurl + "/api/getMediaList").then(r => r.json()).then((response) => {
			response.forEach((media) => {
				var option = document.createElement("a");
				option.text = media.video;
				option.value = JSON.stringify(media);
				if (this.selected && (this.selected === option.text)) {
					option.className = "active";
				}
				option.onclick = () => {
					if (option.className === "active") {
						option.className = "";
						this.dispatchEvent(new CustomEvent('change', {
							detail: { url: "" }
						}));
					} else {
						for (const opt of mediaList.getElementsByTagName('a')) {
							opt.className = "";
						}
						this.dispatchEvent(new CustomEvent('change', {
							detail: { url: option.value }
						}));
						option.className = "active";
					}
				}
				mediaList.appendChild(option);
			});
			this.dispatchEvent(new CustomEvent('init', {
				detail: response
			}));


			var settings = document.createElement("a");
			settings.onclick = () => {
				if (settings.className === "active") {
					settings.className = "";
					this.dispatchEvent(new CustomEvent('settings', {
						detail: "off"
					}));
				} else {
					settings.className = "active";
					this.dispatchEvent(new CustomEvent('settings', {
						detail: "on"
					}));
				}
			}
			var img = document.createElement("img");
			img.src = "webrtc.png"
			settings.appendChild(img);
			mediaList.appendChild(settings);


			for (const option of mediaList.getElementsByTagName('a')) {
				if (option.className === "active") {
					this.dispatchEvent(new CustomEvent('change', {
						detail: { url: option.value }
					}));
				}
			}
		});
	}
}

customElements.define('webrtc-streamer-menu', WebRTCStreamerMenuElement);
