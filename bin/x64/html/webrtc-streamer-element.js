import "./libs/adapter.min.js";
import "./tensorflow.js";
import "./webrtcstreamer.js";

class WebRTCStreamerElement extends HTMLElement {
    static get observedAttributes() {
        return ['url', 'options', 'webrtcurl', 'notitle', 'width', 'height', 'algo'];
    }


    /**
     * Dom容器
     * @type {null}
     */

    shadowDOM = null

    /**
     * video标签
     * @type {null}
     */
    videoElement = null

    /**
     * 播放按钮
     * @type {null}
     */
    playBtnElement = null

    /**
     *
     * @type {null}
     */
    emptyElement = null

    /**
     * loading Dom
     * @type {null}
     */
    loadingElement = null

    /**
     * 设置按钮
     * @type {null}
     */
    settingElement = null


    /**
     * 设置面板
     * @type {null}
     */
    settingPlane = null

    /**
     * 播放器状态
     * @type {boolean}
     */
    initialized = false


    /**
     * 视频流地址
     * @type {undefined}
     */
    videoUrl = undefined

    /**
     * 音频流地址
     * @type {undefined}
     */
    audioUrl = undefined


    /**
     * 是否重连
     * @type {boolean}
     */
    reconnect = false

    constructor() {
        super();
        this.render()
        this.initialized = false;
        this.modelLoaded = {};
    }


    /**
     * 在元素被添加到文档之后，浏览器会调用这个方法
     */
    connectedCallback() {
        this.connectStream(true);
        this.initialized = true;
    }

    /**
     * 在元素从文档移除的时候，浏览器会调用这个方法
     */
    disconnectedCallback() {
        this.disconnectStream();
        this.initialized = false;
    }

    /**
     * 当属性发生变化的时候，这个方法会被调用
     * @param attrName
     * @param oldVal
     * @param newVal
     */
    attributeChangedCallback(attrName, oldVal, newVal) {
        if (attrName === "notitle") {
            //this.titleElement.style.visibility = "hidden";
        } else if (attrName === "width") {
            this.videoElement.style.width = newVal;
        } else if (attrName === "height") {
            this.videoElement.style.height = newVal;
        }
        if (this.initialized) {
            this.connectStream((attrName !== "algo"));
        }
    }

    disconnectStream() {
        if (this.webRtcServer) {
            this.webRtcServer.disconnect();
            this.webRtcServer = null;
        }
    }

    /**
     * 初始化webrtc
     * @param reconnect
     */
    connectStream(reconnect) {

        this.webRtcServer = new WebRtcStreamer(this.videoElement, null);

        // if (url) {
        //
        //     // stop running algo
        //     Object.values(this.modelLoaded).forEach(promise => {
        //         if (promise.model) {
        //             promise.model.run = null;
        //         }
        //     });
        //
        //     let imgLoaded;
        //     if (reconnect) {
        //         this.disconnectStream();
        //         imgLoaded = new Promise((resolve, rejet) => {
        //             this.videoElement.addEventListener('loadedmetadata', () => resolve(), {once: true});
        //         });
        //         // this.webRtcServer = new WebRtcStreamer(this.videoElement, webrtcurl);
        //         // this.webRtcServer.connect(videostream, audiostream, this.getAttribute("options"));
        //
        //     } else {
        //         imgLoaded = new Promise((resolve) => resolve());
        //     }
        //     const algo = this.getAttribute("algo")
        //     let modelLoaded = this.getModelPromise(algo);
        //
        //     Promise.all([imgLoaded, modelLoaded]).then(([event, model]) => {
        //         this.setVideoSize(this.videoElement.videoWidth, this.videoElement.videoHeight)
        //
        //         model.run = this.getModelRunFunction(algo);
        //         if (model.run) {
        //             model.run(model, this.videoElement, this.canvasElement)
        //             modelLoaded.model = model;
        //         }
        //     });
        // }
    }

    /**
     * 播放视频
     * @param videoUrl
     * @param audioUrl
     */
    play(videoUrl, audioUrl) {
        if (this.webRtcServer) {
            this.emptyElement.style.display = "none"
            this.loadingElement.style.display = 'block'
            this.webRtcServer.connect(videoUrl, audioUrl, this.getAttribute("options"));
            setTimeout(() => {
                this.videoElement.style.display = 'block'
                this.loadingElement.style.display = 'none'
                this.playBtnElement.src="./image/stop.png";
                console.log(                this.playBtnElement)
            }, 1000)
        }

    }

    /**
     * 暂停
     */
    stop() {
        if (this.webRtcServer) {
            this.webRtcServer.disconnect()
            this.videoElement.style.display = 'none'
            this.emptyElement.style.display = "block"
        }
    }

    /**
     * 设置video大小
     * @param width
     * @param height
     */
    setVideoSize(width, height) {
        this.videoElement.width = width;
        this.videoElement.height = height;
    }

    getModelPromise(algo) {
        let modelLoaded;
        if (this.modelLoaded[algo]) {
            modelLoaded = this.modelLoaded[algo];
        } else {
            if (algo === "posenet") {
                modelLoaded = posenet.load();
            } else if (algo === "deeplab") {
                modelLoaded = deeplab.load()
            } else if (algo === "cocossd") {
                modelLoaded = cocoSsd.load();
            } else if (algo === "bodyPix") {
                modelLoaded = bodyPix.load();
            } else if (algo === "blazeface") {
                modelLoaded = blazeface.load();
            } else {
                modelLoaded = new Promise((resolve) => resolve());
            }
            this.modelLoaded[algo] = modelLoaded;
        }
        return modelLoaded;
    }

    getModelRunFunction(algo) {
        let modelFunction;
        if (algo === "posenet") {
            modelFunction = runPosenet;
        } else if (algo === "deeplab") {
            modelFunction = runDeeplab;
        } else if (algo === "cocossd") {
            modelFunction = runDetect;
        } else if (algo === "bodyPix") {
            modelFunction = runbodyPix;
        } else if (algo === "blazeface") {
            modelFunction = runblazeface;
        }
        return modelFunction;
    }


    /**
     * 播放器模板
     * @returns {string}
     */
    htmlTemplate() {
        const style = `<style>@import "styles.css"</style>`
        const element = `<div class="rtsp-player">
                      <div  class="rtsp-player-detail">
                        <video class="rtsp-player-video" muted playsinline id="video" style="display: none;"></video>
                        <canvas></canvas>
                        <div class="loadBox" style="display: none;">
                          <div class="loaderContantBox"></div>
                        </div>
                          <div class="no-select-video">
                            <div class="rtsp-player-logo"></div>
                            <div class="no-select-video-title">
                              <span>暂无输入画面</span>
                            </div>
                          </div>
                          <div class="rtsp-player-toolbar">  
                            <input class="play_btn" type="image" src="./image/playing.png" >
                            <div class="progress-bar">
                                <div class="buffer-bar"></div>
                                <div class="buffer-bar__read"></div>
                                <div class="buffer-bar__button"></div>
                             </div>
                            <input class="setting_btn" type="image" src="./image/setting.png" >
                          </div>
                        <div class="setting-plane">
                            <div class="setting-plane-title">
                                <span>设置</span>   
                            </div>
                            <div class="setting-plane-content">
                             <div class="plane-item">
                                <label>视频流地址</label>
                                <input type="text" id="videoUrl" placeholder="请输入视频流地址" />
                            </div>
                             <div class="plane-item">
                                <label>音频流地址</label>
                                <input type="text" id="audioUrl"  placeholder="请输入音频流地址" />
                            </div>                    
                            </div>
                            <div class="setting-plane-footer">
                                <button class="plane-btn-save">开始拉流</button>   
                                <button class="plane-btn-close">关闭</button>                            
                            </div>
                        </div>
                      </div>
                 </div>`
        return style + element
    }

    /**
     * 渲染播放器
     */
    render() {
        this.shadowDOM = this.attachShadow({mode: 'open'});
        this.shadowDOM.innerHTML = this.htmlTemplate()
        this.videoElement = this.shadowDOM.getElementById("video");
        this.loadingElement = this.shadowDOM.querySelector('.loadBox')
        this.emptyElement = this.shadowDOM.querySelector('.no-select-video')

        this.playBtnElement = this.shadowDOM.querySelector('.play_btn')

        this.settingElement = this.shadowDOM.querySelector('.setting_btn')
        this.settingPlane = this.shadowDOM.querySelector('.setting-plane')

        this.settingElement.addEventListener('click', (e) => {
            e.preventDefault()
            this.settingPlane.style.display = 'block'
        })

        /**
         * 开始拉流按钮点滴事件
         */
        this.shadowDOM.querySelector('.plane-btn-save').addEventListener('click',()=>{
            this.videoUrl = this.shadowDOM.getElementById("videoUrl").value;
            this.audioUrl = this.shadowDOM.getElementById("audioUrl").value;
            this.play(this.videoUrl,this.audioUrl)
            this.settingPlane.style.display = 'none'

        })

        /**
         * 关闭plane弹框
         */
        this.shadowDOM.querySelector('.plane-btn-close').addEventListener('click',()=>{
            this.settingPlane.style.display = 'none'

        })

    }


}

customElements.define('webrtc-streamer', WebRTCStreamerElement);
