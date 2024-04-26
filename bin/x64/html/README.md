# WebRtc介绍

## WebRtc三大核心API
```
WebRTC将音视频数据从设备采集到网络传输过程中涉及的功能封装到以下3个主要API：MediaStream、PeerConnection和DataChanel。
```

### MediaStream
```
MediaStream（即getUserMedia）用于表示被获取的媒体数据，例如来自麦克风和摄像头音视频流。
```

### RTCPeerConnection
```
WebRTC使用RTCPeerConnection在浏览器之间传输流数据.需要一个机制来进行传输的协调和控制消息的发送,这个过程叫做信号处理.信号处理的方法和协议未包含在WebRTC中.

WebRTC应用的开发者可以选择自己喜欢的消息协议,比如 SIP 或者XMPP,任何适合的双向通行信道.appr.tc的示例使用了XHR和Channel API 作为信令机制.Codelab使用 Node运行的Socket.io 库来做.

信号用于交换以下三类信息:

会话控制消息:用来初始化或者关闭通讯和报告错误

网络配置:我面向外部世界的IP地址合端口

媒体能力:什么样的解码器合分辨率可以被我的浏览器支持和浏览器想要什么样的数据.
```
### DataChannel
```
DataChannel表示一个在两个节点之间的双向的数据通道。
```

## WebRtcPlayer
```
WebrtcPlayer播放器采用web自定义组件实现
```
### 调用示例
```
<webrtc-video  id="webrtc-video" url=""  options="rtptransport=tcp&timeout=60&width=0&height=0&bitrate=0&rotation=0"></webrtc-video>
```
### 参数介绍
```
url:视频播放地址
auto：是否开启自动播放
snap：是否开启抓拍控件
full：是否开启全屏控件
options:配置项，暂时填写固定值

```

### 方法
####播放 play(url:string) 

```
const player = document.getElementById("webrtc-video")
player.play("填写url地址")
```
####停止 stop()

```
const player = document.getElementById("webrtc-video")
player.stop()
```
####抓拍 snapPicture(cb)

```
const player = document.getElementById("webrtc-video")
player.snapPicture((canvas)=>{
    //抓拍成功后的回调
})
```
####全屏 fullscreen()

```
const player = document.getElementById("webrtc-video")
player.fullscreen()
```
####退出全屏 exitScreen()

```
const player = document.getElementById("webrtc-video")
player.exitScreen()
```
