我们的官网 [https://www.yunshihome.com/](http://)

我们的QQ群：873666268

### 一、功能说明

         

- ABLMediaServer是一款高性能流媒体服务器，其在Windows平台采用高性能的完成端口网络模型，在Linux平台则采用epoll，同时配备线程池用于媒体数据的接收、转换和发送。

- 该服务器具有强劲的性能和稳定的运行，经过压力测试表明，在转发性能、CPU占用率和运行稳定性方面具有明显优势。ABLMediaServer能够接收通过ffmpeg命令或其他标准的rtsp、rtmp推流函数推送的rtsp流、rtmp流，并能够代理拉流，接收国标GB28181流。

- 服务器经过转换后，能够输出标准的rtsp码流、rtmp码流、http-flv、ws-flv码流（支持H265视频输出）、http-mp4（视频支持H264、H265，音频支持AAC）、hls码流（视频支持H264、H265，音频支持AAC）、GB28181码流（国标PS流）。

- 此外，流媒体服务器支持录像功能，包括智能录像删除、录像查询、录像文件点播和录像文件极速下载。对于http-flv、ws-flv、http-mp4协议点播时，支持暂停继续和拖动播放；对于rtsp点播录像文件时，支持慢放（1/16、1/8、1/4、1/2）、快放（2、4、8、16）、正常速度以及拖动播放。

- 该服务器还支持秒级（基本上在1秒以内）图片抓拍，并能够对抓拍的图片进行查找和以http协议下载。同时，服务器支持H265转码为H264，转码输出视频支持指定分辨率、宽高、码率大小等参数。在Windows平台上，支持英伟达显卡硬件加速转码。

- 经过测试，Linux平台最大并发转码能力为40路H265（在至强E5 2650 V3的硬件环境下），而Windows平台为35路H265（在i9 + 英伟达RTX 2080的硬件环境下）。

- 最后，服务器支持对转码后的视频打入自定义水印，水印的字符内容、字体大小、字体颜色和字体位置均可在配置文件中配置。


                                                                           【欢迎加入高性能流媒体服务QQ群 873666268 】
  
二、ABLMediaServer主要功能
	 
    网络协议媒体输入 
	   rtsp、rtmp外部主动推流输入 
         1、rtsp外部主动推流    (支持 视频：H264、H265 ，音频：AAC、G711A、G711U)	
         2、rtmp外部主动推流    (支持 视频：H264、H265 ，音频：AAC)	
		 3、国标GB28181输入     (支持 视频：H264、H265 ，音频：AAC、G711A、G711U) 
		 
	   rtsp、rtmp、http-flv 拉流输入：
         1、rtsp     拉流       (支持 视频：H264、H265 ，音频：AAC、G711A、G711U)	
         2、rtmp     拉流       (支持 视频：H264、H265 ，音频：AAC)	
		 3、http-flv 拉流       (支持 视频：H264、H265 ，音频：AAC)
		 
    网络协议媒体 输出：
	   被动拉流输出 
         1、rtsp                (支持 视频：H264、H265 ，音频：AAC、G711A、G711U)	
         2、rtmp                (支持 视频：H264、H265 ，音频：AAC)
		 3、GB28181码流         (支持 视频：H264、H265 ，音频：AAC、G711A、G711U)
         4、http-flv            (支持 视频：H264、H265 ，音频：AAC)
         5、http-hls            (支持 视频：H264、H265 ，音频：AAC) 
		 6、http-mp4            (支持 视频：H264、H265 ，音频：AAC)  
 		 7、websocket-flv       (支持 视频：H264、H265 ，音频：AAC)
	  
	   rtsp、rtmp、gb28181 主动推流输出：
         1、rtsp推流            (支持 视频：H264、H265 ，音频：AAC、G711A、G711U)	
         2、rtmp推流            (支持 视频：H264、H265 ，音频：AAC)		  
         3、GB28181推流         (支持 视频：H264、H265 ，音频：AAC、G711A、G711U)	
	  
	
三、简明使用例子
     1） 首先要配置 ABLMediaServer.ini 里面的 本机的IP地址 localipAddress 、recordPath 项。 
	    
 		 1  本机的IP地址，最好需要配置准确（如果不配置程序会自动获取一个地址代替，如果本机有多个地址可能会不准确，如果配置则使用配置的IP地址，这样就准确），
		   因为调用 getMediaList 获取可用媒体源的json中，会使用到本机的IP地址来拼接 rtsp、rtmp、http-flv、ws-flv、hls、http-mp4 的播放url 。
		   调用 getMediaList 返回的json串中有如下url子项：
			"url": {
				"rtsp": "rtsp://10.0.0.239:554/Media/Camera_00001",
				"rtmp": "rtmp://10.0.0.239:1935/Media/Camera_00001",
				"http-flv": "http://10.0.0.239:8088/Media/Camera_00001.flv",
				"ws-flv": "ws://10.0.0.239:6088/Media/Camera_00001.flv",
				"http-mp4": "http://10.0.0.239:5088/Media/Camera_00001.mp4",
				"http-hls": "http://10.0.0.239:9088/Media/Camera_00001.m3u8"
			}		    
		  
		    其中的 10.0.0.239 就是可以从 localipAddress 配置项 精确获取 。
		  
		 2、录像路径配置 recordPath，如果不需要录像，可以忽略录像路径配置
			# 录像文件保存路径,如果不配置录像文件保存在应用程序所在的路径下的record子路径，如果配置路径则保存在配置的路径的record里面 
			# 注意：如果需要录像存储，存储的硬盘千万不要分区，整个硬盘作为一个区，因为服务器没有执行两个以上的盘符操作。
			# 录像保存路径配置 windows平台的路径配置 比如 D:\video ,Linux 平台配置 /home/video
			# 录像路径使用了默认路径，就一直使用默认路径，如果使用了配置路径就一直使用配置路径，确保使用的路径的硬盘空间为最大的，如果需要更换路径，要把原来的录像路径的视频全部删除。
            # 1路高清5M的摄像头，如果录像的话，每小时产生2G大小左右的录像文件。可以根据这个来计算需要购买多大的硬盘，接入多少路摄像头，需要设置录像文件最大的保存时间 	

	 2）、  媒体输出规则： [network protocol]://[ip]:[port]/[app]/[stream][.extend]
	 
	          【注：如果自己不想拼接播放url ，可以调用http函数 /index/api/getMediaList，返回可播放媒
			   体源中有各种播放协议的url, 详见下面的函数 /index/api/getMediaList 】
		
		        说明： 
				      [network protocol]  有 rtsp、rtmp、http、ws 
    				  [ip]                就是服务器所在的IP地址 
      				  [port]              各个网络协议分享时设置的端口号，详见 ABLMediaServer.ini 的配置文件，里面有相应的网络协议配置端口
					  [app]               各种网络协议发送过来设置的一级名字
					  [stream]            各种网络协议发送过来设置的二级名字
					  [.extend]           扩展名字，主要为为了访问服务器时，服务器需要识别网络协议需要客户端发送过来的扩展名。
					                        rtsp、rtmp        不需要扩展名，
										    http-flv 、ws-flv 扩展名为 .flv 
					                        hls 方式访问时，  扩展名为 .m3u8 
										    http-mp4访问时    扩展名为 .mp4 
										   
				      比如服务器IP为 190.15.240.11 ，app 为 Media ,stream 为 Camera_00001 ,假定端口都是默认 ，那么各种网络访问url如下：
						 rtsp:  
							rtsp://190.15.240.11:554/Media/Camera_00001
							
						 rtmp:  
							rtmp://190.15.240.11:1935/Media/Camera_00001

						 http-flv: 
							http://190.15.240.11:8088/Media/Camera_00001.flv
							
						 http-mp4: 
							http://190.15.240.11:5088/Media/Camera_00001.mp4
						
						 websocket-flv:  
							ws://190.15.240.11:6088/Media/Camera_00001.flv
							
						 http-hls:  
							http://190.15.240.11:9088/Media/Camera_00001.m3u8

    3）、使用ffmpeg往 ABLMediaServer 推送rtsp 码流 【假定 源摄像机rtsp RUL为 rtsp://admin:abldyjh2020@192.168.1.120:554 , ABLMediaServer 所在服务器地址为 190.15.240.11 】
	    【推送rtsp方式说明：视频支持 H264、H265 ,音频支持AAC、G711A、G711U 】
		
		ffmpeg -rtsp_transport tcp -i rtsp://admin:abldyjh2020@192.168.1.120:554 -vcodec copy -acodec copy -f rtsp -rtsp_transport tcp rtsp://190.15.240.11:554/Media/Camera_00001
		
	   媒体输出： 
	     rtsp: 【rtsp输出格式说明：视频支持 H264、H265 ,音频支持AAC、G711A、G711U 】
		    rtsp://190.15.240.11:554/Media/Camera_00001
			
		 rtmp: 【rtmp输出格式说明：视频支持 H264、H265 ,音频支持AAC 】
            rtmp://190.15.240.11:1935/Media/Camera_00001	 

		 http-flv: 【http-flv输出格式说明：视频支持 H264、H265 ,音频支持AAC 】
            http://190.15.240.11:8088/Media/Camera_00001.flv
			
		 ws-flv: 【http-flv输出格式说明：视频支持 H264、H265 ,音频支持AAC 】
            ws://190.15.240.11:6088/Media/Camera_00001.flv
			
		 http-hls: 【http-hls输出格式说明：视频支持 H264、H265 ,音频支持AAC 】
            http://190.15.240.11:9088/Media/Camera_00001.m3u8
			
    4）、使用ffmpeg往 ABLMediaServer 推送rtmp 码流 【假定 源摄像机rtsp RUL为 rtsp://admin:abldyjh2020@192.168.1.120:554 , ABLMediaServer 所在服务器地址为 190.15.240.11 】
	    【推送rtmp方式说明：视频支持 H264 ,音频支持AAC 】
		
		ffmpeg -rtsp_transport tcp -i rtsp://admin:abldyjh2020@192.168.1.120:554 -acodec copy -vcodec copy -f flv rtmp://190.15.240.11:1935/Media/Camera_00001
		
	      rtsp: 【rtsp输出格式说明：视频支持 H264、H265 ,音频支持AAC、G711A、G711U 】
		    rtsp://190.15.240.11:554/Media/Camera_00001
			
		  rtmp: 【rtmp输出格式说明：视频支持 H264、H265 ,音频支持AAC 】
            rtmp://190.15.240.11:1935/Media/Camera_00001	 

		  http-flv: 【http-flv输出格式说明：视频支持 H264、H265 ,音频支持AAC 】
            http://190.15.240.11:8088/Media/Camera_00001.flv
	
		 ws-flv: 【http-flv输出格式说明：视频支持 H264、H265 ,音频支持AAC 】
            ws://190.15.240.11:6088/Media/Camera_00001.flv

		 http-hls: 【http-hls输出格式说明：视频支持 H264、H265 ,音频支持AAC 】
             http://190.15.240.11:9088/Media/Camera_00001.m3u8
			
    5）、使用ffmpeg往 ABLMediaServer 推送rtsp的文件码流 【假定媒体文件为：F:\video\MP4有声音\H264_AAC_2021-02-10_1080P.mp4  , ABLMediaServer 所在服务器地址为 190.15.240.11 】
	    【推送rtsp方式说明：视频支持 H264、H265 ,音频支持AAC、G711A、G711U 】
		
		ffmpeg -re -stream_loop -1 -i F:\video\MP4有声音\H264_AAC_2021-02-10_1080P.mp4 -vcodec copy -acodec copy -rtsp_transport tcp -f rtsp rtsp://190.15.240.11:554/Media/Camera_00001
		
	    媒体输出：
	      rtsp: 【rtsp输出格式说明：视频支持 H264、H265 ,音频支持AAC、G711A、G711U 】
		    rtsp://190.15.240.11:554/Media/Camera_00001
			
		  rtmp: 【rtmp输出格式说明：视频支持 H264、H265 ,音频支持AAC 】
            rtmp://190.15.240.11:1935/Media/Camera_00001	 

		  http-flv: 【http-flv输出格式说明：视频支持 H264、H265 ,音频支持AAC 】
            http://190.15.240.11:8088/Media/Camera_00001.flv
	
		 ws-flv: 【http-flv输出格式说明：视频支持 H264、H265 ,音频支持AAC 】
            ws://190.15.240.11:6088/Media/Camera_00001.flv

		 http-hls: 【http-hls输出格式说明：视频支持 H264、H265 ,音频支持AAC 】
             http://190.15.240.11:9088/Media/Camera_00001.m3u8
	
      【特别注明：可以往10000 的udp端口推送TS码流，推送成功后，可以调用 http函数getMediaList来获取接入的rtp码流 】
	     ffmpeg.exe -re -stream_loop -1 -i F:\video\H264_20191021094432.mp4 -vcodec copy -f rtp_mpegts rtp://127.0.0.1:100000
    		
	
    6）、流媒体输出播放验证
         如果视频是rtsp方式，可以采用VLC进行播放验证　
　　　　 如果rtmp、http-flv 协议，视频为h264 ,可以采用VLC播放验证、或者B站的 flv.js 播放器验证
		 如果rtmp、http-flv 协议，视频为h265 ,可以采用EasyPlayer.js 播放器验证，【注：VLC 、flv.js 不支持Rtmp的H265视频、也不支持http-flv的265视频 】
			
	7）、申请代理rtsp、rtmp、flv 拉流 、申请删除代理拉流 
		 1) 申请代理rtsp、rtmp、flv 拉流
 		    
			   URL: /index/api/addStreamProxy
			   
			        参数：               参数说明            参数参考值
					secret                   服务器密码        比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc
					vhost                                      比如  _defaultVhost_
					app                      应用名            比如 Media
					stream                   媒体流名          比如 Camera_00001            【/app/stream 组合起来不能重复】
					url                      代理拉流url       比如 rtsp://admin:abldyjh2020@192.168.1.120:554 或者 rtmp://190.15.240.36:1935/Media/Camera_00001 或者  http://190.15.240.36:8088/Media/Camera_00001.flv 
					isRtspRecordURL                            代理拉流的url是否是rtsp录像回放的url 默认0 ，1 是【可选参数】，如果是rtsp录像回放的url，可以进行控制代理拉流，比如 暂停、继续、控制倍速播放，拖动播放等等 ，参考
					                                           函数 /index/api/controlStreamProxy 
					enable_mp4                是否录像         1 录像，0 不录像             【可选参数】
					enable_hls                是否hls切片       1 进行hls 切片 ，0 不切片   【可选参数】
				    convertOutWidth           转码宽            转码输出宽 如果指定宽、高   【可选参数】[1920 x 1080, 1280 x 720 ,960 x 640 ,800 x 480 ,720 x 576 , 720 x 480 ,640 x 480 ,352 x 288 ] 就说明 H265 进行转码为 H264 
				    convertOutHeight          转码高            转码输出高 如果指定宽、高   【可选参数】[1920 x 1080, 1280 x 720 ,960 x 640 ,800 x 480 ,720 x 576 , 720 x 480 ,640 x 480 ,352 x 288 ] 就说明 H265 进行转码为 H264  
				    H264DecodeEncode_enable   H264是否解码      H264分辨率高再编码降分辨率，【可选参数】有时候需要H264视频进行先解码再重新编码降低分辨率，可以设置 H264DecodeEncode_enable 为 1 ，降下来的分辨率为 convertOutWidth 、 convertOutHeight
					
               http  GET 方式 		
			        1 请求rtsp拉流样例
			            http://190.15.240.11:7088/index/api/addStreamProxy?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&vhost=_defaultVhost_&app=Media&stream=Camera_00001&url=rtsp://admin:abldyjh2020@192.168.1.120:554&enable_mp4=0

			   http POST 方式  
                    1 请求rtsp拉流样例		
                       http请求 url 样例
                         http://190.15.240.11:7088/index/api/addStreamProxy					   
					   body 参数 , json 格式	  
			             {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","vhost":"_defaultVhost_","app":"Media","stream":"Camera_00001","url":"rtsp://admin:abldyjh2020@192.168.1.120:554","enable_mp4":0}
						 
				返回Body：
					{
						"code": 0,           # 0为操作成功，其他值为操作失败
						"memo": "success",   # success 为成功 
						"key": 93            # 成功时返回大于0的值，为代理拉流的Key ,删除代理拉流时需要用的   
					}
					
		 2) 控制代理拉流，比如 暂停、继续、控制倍速播放，拖动播放等等 
			     URL: /index/api/controlStreamProxy
			   
			     参数：
					secret  服务器密码 ，比如  035c73f7-bb6b-4889-a715-d9eb2d1925cc
					key                  比如  93 ，调用 addStreamProxy 返回的 key 的值 
					command              比如  pause、resume、seek、scale 对于对应意思：暂停、继续、拖动播放、倍速播放 
 			        value  (字符串)      比如  1、2、4、8、16(倍速播放) ，360、1800、3600（拖动播放），2022-06-10T14:17:20.000（拖动播放）
					                           value 为可选参数，当 command 为 pause,resume 时，value 不用 ，当 command 为seek,sacale
											   是，需要填写value的值 
					
			    http GET 方式 
			        http://190.15.240.11:7088/index/api/controlStreamProxy?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&key=93&command=pause
				   
				http  POST 方式 
				    http URL :
				      http://190.15.240.11:7088/index/api/controlStreamProxy
					  
				    body 参数 Json格式 
				       {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","key":93,"command":"pause"} 
				       {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","key":93,"command":"sacale","value":"2"} 
				   
				返回Body：
					{
						"code": 0,           # 0为操作成功，其他值为操作失败
						"memo": "success",   # success 为成功 ,如果失败是其他值 
					}		
		
		         【注：发送http请求 可以使用curl、postman、或者其他标准的http工具 】
				 
				
		 3) 申请删除代理rtsp、rtmp、flv 拉流
 			     URL: /index/api/delStreamProxy
			   
			     参数：
					secret  服务器密码 ，比如  035c73f7-bb6b-4889-a715-d9eb2d1925cc
					key                  比如  93 ，调用 addStreamProxy 返回的 key 的值 
 			   
			    http GET 方式 
			        http://190.15.240.11:7088/index/api/delStreamProxy?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&key=93
				   
				http  POST 方式 
				    http URL :
				      http://190.15.240.11:7088/index/api/delStreamProxy
					  
				    body 参数 Json格式 
				       {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","key":93} 
				   
				返回Body：
					{
						"code": 0,           # 0为操作成功，其他值为操作失败
						"memo": "success",   # success 为成功 ,如果失败是其他值 
					}		
		
		         【注：发送http请求 可以使用curl、postman、或者其他标准的http工具 】
				 
				 
		   
	 8）、申请代理rtsp、rtmp、推流 、申请删除代理拉流 
		 1) 申请代理rtsp、rtmp 推流（注意：不是国标GB28181推流） 
 		    
			     URL: /index/api/addPushProxy
			   
			     参数：     参数说明          参考值
					secret  服务器密码 ，比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc
					vhost                比如 _defaultVhost_
					app     应用名       比如 Media
					stream  媒体流名     比如 Camera_00001
					url     代理推流url  比如 rtsp://190.15.240.36:554/Media/Camera_00001 或者 rtmp://190.15.240.36:1935/Media/Camera_00001 
					
               http  GET 方式 			   
			         http://190.15.240.11:7088/index/api/addPushProxy?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&vhost=_defaultVhost_&app=Media&stream=Camera_00001&url=rtsp://190.15.240.36:554/Media/Camera_00001
 
			   http  POST 方式  
                    http URL 
                      http://190.15.240.11:7088/index/api/addPushProxy

					http Body 参数 (json格式) 					  
 			          {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","vhost":"_defaultVhost_","app":"Media","stream":"Camera_00001","url":"rtsp://190.15.240.36:554/Media/Camera_00001"}
 			
				返回Body：
					{
						"code": 0,           # 0为操作成功，其他值为操作失败
						"memo": "success",   # success 为成功 
						"key": 93            # 成功时返回大于0的值，为代理推流的Key ,删除代理推流时需要用的   
					}
				
		 2) 申请删除代理rtsp、rtmp 推流
 			     URL: /index/api/delPushProxy
			   
			     参数：      参数说明          参数参考值
					secret  服务器密码 ，      比如  035c73f7-bb6b-4889-a715-d9eb2d1925cc
					key     主键ID             比如  93 ，调用 /index/api/addPushProxy 返回的 key 的值 
 			   
			    http GET 方式 
			        http://190.15.240.11:7088/index/api/delPushProxy?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&key=93
				   
				http POST 方式 
				    http URL 
					  http://190.15.240.11:7088/index/api/delPushProxy
					  
					http Body json 格式  
				      {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","key":93}
				   
				返回Body：
					{
						"code": 0,           # 0为操作成功，其他值为操作失败
						"memo": "success",   # success 为成功 ,如果失败是其他值 
					}		
		   		   
	9）、创建GB28181接收端口、删除GB28181接收端口 
	
         1 	创建GB28181接收端口
				 
			     URL: /index/api/openRtpServer
				 功能：
				      创建GB28181接收端口，如果该端口接收超时，会自动回收，不用调用  /index/api/closeRtpServer
			   
			     参数：                      参数说明           参数参考值
					secret                    服务器密码        比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc
					vhost                                       比如 _defaultVhost_
					app                       应用名            比如 gb28181 、rtp 等等 
					stream_id                 媒体流名          比如 44030012343220234234 （最好是国标编号）
					payload                   PS负载值          国标SDP里面PS负载值　，比如 96,98 ,108 ,一定要从国标SDP里面获取 
					port                      端口号            0 ，由服务器自动分配，别的值 比如 26324 为指定端口
					enable_tcp                是否为tcp         0 为 udp ,1 为tcp 方式 
					enable_mp4                是否录像          1 录像，0 不录像            【可选参数】
					enable_hls                是否hls切片       1 进行hls 切片 ，0 不切片   【可选参数】
				    convertOutWidth           转码宽            转码输出宽 如果指定宽、高   【可选参数】[1920 x 1080, 1280 x 720 ,960 x 640 ,800 x 480 ,720 x 576 , 720 x 480 ,640 x 480 ,352 x 288 ] 就说明 H265 进行转码为 H264 
				    convertOutHeight          转码高            转码输出高 如果指定宽、高   【可选参数】[1920 x 1080, 1280 x 720 ,960 x 640 ,800 x 480 ,720 x 576 , 720 x 480 ,640 x 480 ,352 x 288 ] 就说明 H265 进行转码为 H264  
				    H264DecodeEncode_enable   H264是否解码      H264分辨率高再编码降分辨率，【可选参数】有时候需要H264视频进行先解码再重新编码降低分辨率，可以设置 H264DecodeEncode_enable 为 1 ，降下来的分辨率为 convertOutWidth 、 convertOutHeight
					
	             http  GET 方式 			   
			        http://190.15.240.11:7088/index/api/openRtpServer?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&vhost=_defaultVhost_&app=gb28181&stream_id=44030012343220234234&payload=96&port=0&enable_tcp=0&enable_mp4=0
  
			     http  POST 方式    
				    http URL
					  http://190.15.240.11:7088/index/api/openRtpServer
					  
					http 参数值 
			           {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","vhost":"_defaultVhost_","app":"Media","stream_id":"Camera_00001","payload":96,"port":0,"enable_tcp":0,"enable_mp4":0}
 			
				返回Body：
					  {
						"code": 0,           # 0为操作成功，其他值为操作失败
						"port": 8356,        # 端口号
						"memo": "success",   # success 为成功 
						"key": 93            # 成功时返回大于0的值，GB28181接收实例key ,关闭时需要   
					  }
		
           2    删除 GB28181接收端口		
			     URL: /index/api/closeRtpServer
				 功能：
				      删除GB28181接收端口 
			   
			     参数：              参数说明     参数参考值   
					secret           服务器密码   比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc
				    key              主键值ID     比如  93 ，  调用 /index/api/openRtpServer 返回的 key 的值 
 			   
			    http GET 方式 
			       http://190.15.240.11:7088/index/api/closeRtpServer?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&key=93
				   
				http  POST 方式 
				   http URL 
				     http://190.15.240.11:7088/index/api/closeRtpServer
					 
				   http Body 参数 json 格式
				     {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","key":93}
				   
				返回Body：
					{
						"code": 0,           # 0为操作成功，其他值为操作失败
						"memo": "success",   # success 为成功 ,如果失败是其他值 
					}				
					
	 10）、创建GB28181发送端口、删除GB28181发送端口 
	
         1 	创建GB28181发送端口
				 
			     URL: /index/api/startSendRtp
				 功能：
				      创建GB28181发送端口，如果该发送端端口没有数据发送，会自动回收，不用调用  /index/api/stopSendRtp
			   
			     参数：              参数说明              参数参考值
					secret           服务器密码            比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc
					vhost                                  比如 _defaultVhost_
					app              应用名                比如 gb28181 、rtp 等等 
					stream           媒体流名              比如 44030012343220234234 （最好是国标编号）
					payload          PS负载值              国标SDP里面PS负载值　，比如 96,98 ,108 ,rtp打包时需要
					ssrc             同步源                ssrc
					src_port         发送端绑定的端口号    指定服务器在发送国标流时绑定的端口号，如果为 0 ，由服务器自动分配，别的值 比如 26324 为指定端口
					dst_url          目标IP                目标IP地址 
					dst_port         目标端口              目标端口 
					is_udp           是否设置udp           是否设置为UDP通讯，0 TCP，1 udp
					
					
	             http  GET 方式 			   
			        http://190.15.240.11:7088/index/api/startSendRtp?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&vhost=_defaultVhost_&app=gb28181&stream=44030012343220234234&payload=96&ssrc=5224&src_port=26324&dst_url=190.15.240.11&dst_port=9824&is_udp=1
  
			     http  POST 方式    
				    http URL
					  http://190.15.240.11:7088/index/api/startSendRtp
					  
					http 参数值 
			           {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","vhost":"_defaultVhost_","app":"Media","stream":"Camera_00001","payload":96,"ssrc":2432,"src_port":26324,"dst_url":"190.15.240.11","dst_port":9824,"is_udp":1}
 			
				返回Body：
					  {
						"code": 0,           # 0为操作成功，其他值为操作失败
						"port": 8356,        # 端口号
						"memo": "success",   # success 为成功 
						"key": 93            # 成功时返回大于0的值，GB28181发送码流实例key ,关闭时需要   
					  }
		
           2    删除 GB28181发送端口		
			     URL: /index/api/stopSendRtp
				 功能：
				      删除GB28181发送端口 
			   
			     参数：              参数说明     参数参考值   
					secret           服务器密码   比如  035c73f7-bb6b-4889-a715-d9eb2d1925cc
				    key              主键值ID     比如  93 ，  调用 /index/api/startSendRtp 返回的 key 的值 
 			   
			    http GET 方式 
			       http://190.15.240.11:7088/index/api/stopSendRtp?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&key=93
				   
				http  POST 方式 
				   http URL 
				     http://190.15.240.11:7088/index/api/stopSendRtp
					 
				   http Body 参数 json 格式
				     {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","key":93}
				   
				返回Body：
					{
						"code": 0,           # 0为操作成功，其他值为操作失败
						"memo": "success",   # success 为成功 ,如果失败是其他值 
					}				
					
	11）、获取流媒体服务器所有可用的媒体源
			 URL: /index/api/getMediaList
			 
			 功能：
				 获取流媒体服务器所有媒体源

			 参数：              参数说明     参数参考值
				secret           服务器密码   比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc        【必填参数】
				app              应用名       比如 rtp、gb28181、Media 等等 ，自己起的名字     【可选参数】
				stream           媒体流名     比如 Camera_00001、44303403343034243200234 等等  【可选参数】
				
				参数填写样例说明：
			  样例1（app、stream 都不填写） 
				   secret  035c73f7-bb6b-4889-a715-d9eb2d1925cc
				    
				  返回所有在线的媒体源
				 
			  样例2 （只填写 app ）
				   secret  035c73f7-bb6b-4889-a715-d9eb2d1925cc
				   app     rtp 
				   返回 app 等于 rtp 的所有媒体源
				   
			  样例3 （填写 app = rtp , stream = 44303403343034243200234）
				   secret  035c73f7-bb6b-4889-a715-d9eb2d1925cc
				   app     rtp 
				   stream  44303403343034243200234 
				  返回 app 等于 rtp、并且 stream 等于 44303403343034243200234 的所有媒体源
				  
			  样例4 （填写 stream = 44303403343034243200234）
				   secret  035c73f7-bb6b-4889-a715-d9eb2d1925cc
 				   stream  44303403343034243200234 
				   返回 stream 等于 44303403343034243200234 的所有媒体源
 				  
	         http  GET 方式 
               http://127.0.0.1:7088/index/api/getMediaList?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc
			   
			http  POST 方式 
			   http URL 
				 http://190.15.240.11:7088/index/api/getMediaList
				 
			   http Body 参数 json 格式
				 {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc"}
				 
			 返回Body：
				{
					"code": 0,
					"memo": "success",
					"mediaList": [
						{
							"key": 34,    
							"app": "Media",  
							"stream": "Camera_00001",
							"status": false ,          【 false 尚未录像，true 正在录像 】
							"sourceURL": "rtsp://10.0.0.239:554/Media/Camera_00001",
							"sourceType": 23,
							"readerCount": 0,
							"videoCodec": "H264",
							"width": 1920,
							"height": 1080,
							"networkType": 24,
							"audioCodec": "AAC",
							"audioChannels": 1,
							"audioSampleRate": 16000,
							"url": {
								"rtsp": "rtsp://10.0.0.239:554/Media/Camera_00001",
								"rtmp": "rtmp://10.0.0.239:1935/Media/Camera_00001",
								"http-flv": "http://10.0.0.239:8088/Media/Camera_00001.flv",
								"ws-flv": "ws://10.0.0.239:6088/Media/Camera_00001.flv",
								"http-mp4": "http://10.0.0.239:5088/Media/Camera_00001.mp4",
								"http-hls": "http://10.0.0.239:9088/Media/Camera_00001.m3u8"
							}
						}
					]
				}	
				
              【注释：可以根据 "networkType": 24, 这个字段值区分 媒体接入的类型 ，具体详见网络类型的对照表 】				
			 
   12)   删除 某一个媒体源 		
		 URL: /index/api/delMediaStream
		 功能：
			  某一个媒体源，这媒体源，可以是rtp推流、rtmp推流，各种方式代理拉流接入的，国标接入 等等。
	   
		 参数：              参数说明     参数参考值   
			secret           服务器密码   比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc
			key              主键值ID     比如  93 ，  调用 /index/api/getMediaList 返回的 key 的值 
	   
		http GET 方式 
		   http://190.15.240.11:7088/index/api/delMediaStream?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&key=93
		   
		http  POST 方式 
		   http URL 
			 http://190.15.240.11:7088/index/api/delMediaStream
			 
		   http Body 参数 json 格式
			 {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","key":93}
		   
		返回Body：
			{
				"code": 0,           # 0为操作成功，其他值为操作失败
				"memo": "success",   # success 为成功 ,如果失败是其他值 
			}				
			
	13）、获取流媒体服务器所有往外部输出码流列表，包括外部请求的rtsp、rtmp、http-flv、ws-flv、hls 列表
	                                      也包括服务器代理rtsp推流、rtmp推流列表
										  也包括服务器以国标方式往上级推rtp流列表
						【必要时可以调用 /index/api/delOutList 接口删除某一个列表对象，比如删除某一路国标推流、删除某一路rtsp推流、 删除某一路rtmp推流】
			 URL: /index/api/getOutList
			 
			 功能：
				 获取流媒体服务器所有输出流列表

			 参数：              参数说明     参数参考值
				secret           服务器密码   比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc
				
	         http  GET 方式 
               http://44.35.33.239:7088/index/api/getOutList?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc	
			   
			http  POST 方式 
			   http URL 
				 http://44.35.33.239:7088/index/api/getOutList
				 
			   http Body 参数 json 格式
				 {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc"}
				 
			 返回Body：
					{
						"code": 0,
						"memo": "success",
						"outList": [
							{ 
								"key": 103,                【请求客户端的标识ID ，可以调用  /index/api/delOutList 删除 该请求】
								"app": "Media",
								"stream": "Camera_00001",
								"sourceURL": "rtsp://44.35.33.239:554/Media/Camera_00001",  【表示外界以rtsp方式向服务器请求码流】
								"videoCodec": "H264",
								"audioCodec": "AAC",
								"audioChannels": 1,
								"audioSampleRate": 16000,
								"networkType": 24,         【网络类型为24 ，标识为rtsp 方式】
								"dst_url": "44.35.33.39",  【 请求码流客户端IP   】
								"dst_port": 43801          【 请求码流客户端端口 】
							},
							{
								"key": 85,                 【请求客户端的标识ID ，可以调用  /index/api/delOutList 删除 该请求】
								"app": "Media",
								"stream": "Camera_00001",
								"sourceURL": "http://localhost:8088/Media/Camera_00001.flv",【表示外界以 http-flv 方式向服务器请求码流】 
								"videoCodec": "H264",
								"audioCodec": "AAC",
								"audioChannels": 1,
								"audioSampleRate": 16000,
								"networkType": 25,         【网络类型为25 ，标识为 http-flv 方式】
								"dst_url": "44.35.33.39",  【 请求码流客户端IP   】
								"dst_port": 43806          【 请求码流客户端端口 】  
							}
						]
					}			
	
	              【注释：可以根据 "networkType": 24, 这个字段值区分 媒体输出的类型 ，具体详见网络类型的对照表 】				

   14)   删除 某一个服务器所有往外部输出码流列表	
		 URL: /index/api/delOutList 
		 功能：
			  删除某一个流媒体服务器所有往外部输出码流列表，包括外部请求的rtsp、rtmp、http-flv、ws-flv、hls 点播 。国标推流、rtsp推流、rtmp 推流 等等
	   
		 参数：              参数说明     参数参考值   
			secret           服务器密码   比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc
			key              主键值ID     比如  93 ，  调用 /index/api/getOutList 返回的 key 的值 
	   
		http GET 方式 
		   http://190.15.240.11:7088/index/api/delOutList?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&key=93
		   
		http  POST 方式 
		   http URL 
			 http://190.15.240.11:7088/index/api/delOutList
			 
		   http Body 参数 json 格式
			 {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","key":93}
		   
		返回Body：
			{
				"code": 0,           # 0为操作成功，其他值为操作失败
				"memo": "success",   # success 为成功 ,如果失败是其他值 
			}	
			
   15）根据条件组合，删除任意一个或一组或者全部媒体输入列表
       URL: /index/api/close_streams
	   
	   功能
	      删除任意一个或一组或者全部媒体输入列表 
		    
			secret           服务器密码            比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc                           【必填参数】
			vhost                                  比如 _defaultVhost_                                                 【可选参数】
			app              应用名                比如 gb28181 、rtp 等等                                             【可选参数】
			stream           媒体流名              比如 Camera_00001、dsafdsafassdafadsfas、等等                       【可选参数】
			force            是否强制关闭          1 强制关闭，不管是否有人在观看、0 非强制关闭，当有人观看时不关闭。  【必填参数】
			
		 http GET 方式 
            示例1： http://190.168.24.112:7088/index/api/close_streams?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&app=live&force=1
		            【表示强行关闭 app 等于 live 的码流接入】
            示例2： http://190.168.24.112:7088/index/api/close_streams?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&app=live&stream=Camera_00001&force=1
		            【表示强行关闭 app 等于 live, 并且 stream 等于 Camera_00001  的码流接入 】
            示例3： http://190.168.24.112:7088/index/api/close_streams?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&force=1
		            【表示强行关闭 所有码流（app全部、stream全部） 接入 】
				   
 		 http pos 方式 
            示例1： http URL:
			          http://190.168.24.112:7088/index/api/close_streams
			        body:
			          {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","app":"live","force":1}
					
		             【表示强行关闭 app 等于 live 的码流接入】
					 
            示例2： http URL:
			          http://190.168.24.112:7088/index/api/close_streams
			        body:
			          {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","app":"live","stream":"Camera_00001","force":1}
					  
		           【表示强行关闭 app 等于 live, 并且 stream 等于 Camera_00001  的码流接入 】
				   
            示例3： http URL:
			          http://190.168.24.112:7088/index/api/close_streams
			        body:
			          {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","force":1}
			
		           【 表示强行关闭 所有码流（app全部、stream全部） 接入 】
 
	 16）、 开始录像、停止录像
		 1) 申请开始录像
 		    
			     URL: /index/api/startRecord
			   
			     参数：     参数说明          参考值
					secret  服务器密码 ，比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc
					vhost                比如 _defaultVhost_
					app     应用名       比如 Media
					stream  媒体流名     比如 Camera_00001
					
               http  GET 方式 			   
			         http://190.15.240.11:7088/index/api/startRecord?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&vhost=_defaultVhost_&app=Media&stream=Camera_00001
 
			   http  POST 方式  
                    http URL 
                      http://190.15.240.11:7088/index/api/startRecord

					http Body 参数 (json格式) 					  
 			          {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","vhost":"_defaultVhost_","app":"Media","stream":"Camera_00001"}
 			
				返回Body：
					{
						"code": 0,           # 0为操作成功，其他值为操作失败
						"memo": "MediaSource: /Media/Camera_00001 start Record",   #  "code": 0 为成功 
					}
				
		 2) 申请停止录像
 			     URL: /index/api/stopRecord
			   
			     参数：     参数说明          参考值
					secret  服务器密码 ，比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc
					vhost                比如 _defaultVhost_
					app     应用名       比如 Media
					stream  媒体流名     比如 Camera_00001
					
               http  GET 方式 			   
			         http://190.15.240.11:7088/index/api/stopRecord?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&vhost=_defaultVhost_&app=Media&stream=Camera_00001
 
			   http  POST 方式  
                    http URL 
                      http://190.15.240.11:7088/index/api/stopRecord

					http Body 参数 (json格式) 					  
 			          {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","vhost":"_defaultVhost_","app":"Media","stream":"Camera_00001"}
 			
				返回Body：
					{
						"code": 0,           # 0为操作成功，其他值为操作失败
						"memo": "success",   # success 为成功 
					}
 
   17  获取系统配置参数 		
		 URL: /index/api/getServerConfig
		 功能：
			  获取服务器的配置参数
	   
		 参数：              参数说明     参数参考值   
			secret           服务器密码   比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc
	   
		http GET 方式 
		   http://190.15.240.11:7088/index/api/getServerConfig?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc
		   
		http  POST 方式 
		   http URL 
			 http://190.15.240.11:7088/index/api/getServerConfig
			 
		   http Body 参数 json 格式
			 {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc"}
		   
		返回Body：
			{
				"code": 0,
				"params": [
					{
						"secret": "035c73f7-bb6b-4889-a715-d9eb2d1925cc",
						"memo": "server password"
					},
					{
						"ServerIP": "44.35.33.239",
						"memo": "ABLMediaServer ip address"
					},
					{
						"mediaServerID": "ABLMediaServer_00001",
						"memo": "media Server ID "
					},
					{
						"hook_enable": 0,
						"memo": "hook_enable = 1 open notice , hook_enable = 0 close notice "
					},
					{
						"enable_audio": 1,
						"memo": "enable_audio = 1 open Audio , enable_audio = 0 Close Audio "
					},
					{
						"httpServerPort": 7088,
						"memo": "http api port "
					},
					{
						"rtspPort": 554,
						"memo": "rtsp port "
					},
					{
						"rtmpPort": 1935,
						"memo": "rtmp port "
					},
					{
						"httpFlvPort": 8088,
						"memo": "http-flv port "
					},
					{
						"hls_enable": 0,
						"memo": "hls whether enable "
					},
					{
						"hlsPort": 9088,
						"memo": "hls port"
					},
					{
						"wsPort": 6088,
						"memo": "websocket flv port"
					},
					{
						"mp4Port": 5088,
						"memo": "http mp4 port"
					},
					{
						"ps_tsRecvPort": 10000,
						"memo": "recv ts , ps Stream port "
					},
					{
						"hlsCutType": 2,
						"memo": "hlsCutType = 1 hls cut to Harddisk,hlsCutType = 2  hls cut Media to memory"
					},
					{
						"h265CutType": 1,
						"memo": " 1 h265 cut TS , 2 cut fmp4 "
					},
					{
						"RecvThreadCount": 128,
						"memo": " RecvThreadCount "
					},
					{
						"SendThreadCount": 128,
						"memo": "SendThreadCount"
					},
					{
						"GB28181RtpTCPHeadType": 2,
						"memo": "rtp Length Type"
					},
					{
						"ReConnectingCount": 40320,
						"memo": "Try reconnections times ."
					},
					{
						"maxTimeNoOneWatch": 9999999,
						"memo": "maxTimeNoOneWatch ."
					},
					{
						"pushEnable_mp4": 0,
						"memo": "pushEnable_mp4 ."
					},
					{
						"fileSecond": 180,
						"memo": "fileSecond ."
					},
					{
						"fileKeepMaxTime": 1,
						"memo": "fileKeepMaxTime ."
					},
					{
						"httpDownloadSpeed": 6,
						"memo": "httpDownloadSpeed ."
					},
					{
						"RecordReplayThread": 32,
						"memo": "Total number of video playback threads ."
					}
				]
			}
				
		18）、查询录像列表
			 URL: /index/api/queryRecordList
			 
			 功能：
				 查询某一路输入源的录像列表(可以查询代理拉流输入、推流输入、国标输入等等 )

			 参数：              参数说明     参数参考值
				secret           服务器密码   比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc        【必填参数】
				vhost                         比如 _defaultVhost_                              【可选参数】 
				app              应用名       比如 rtp、gb28181、Media 等等 ，自己起的名字     【必填参数】
				stream           媒体流名     比如 Camera_00001、44303403343034243200234 等等  【必填参数】
			    starttime        开始时间     比如 20220116154810 年月日时分秒                 【必填参数】
			    endtime          结束时间     比如 20220116155115 年月日时分秒                 【必填参数】
				
			【注意：1、开始时间必须小于 当前时间减去切片时长的时间 2、从 开始时间 至 结束时间 不能超过3天】
 				  
	         http  GET 方式 
               http://10.0.0.239:7088/index/api/queryRecordList?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&app=Media&stream=Camera_00001&starttime=20220116154810&endtime=20220116155115
			   
			http  POST 方式 
			   http URL 
				 http://190.15.240.11:7088/index/api/queryRecordList
				 
			   http Body 参数 json 格式
				 {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","vhost":"_defaultVhost_","app":"Media","stream":"Camera_00001","starttime":"20220116154810","endtime":"20220116155115"}
				 
			 返回Body：
				{
					"code": 0,
					"app": "Media",
					"stream": "Camera_00001",
					"starttime": "20220116154810",
					"endtime": "20220116155115",
					"recordFileList": [
						{
							"file": "20220116154810.mp4",
							"url": {
								"rtsp": "rtsp://10.0.0.239:554/Media/Camera_00001__ReplayFMP4RecordFile__20220116154810",
								"rtmp": "rtmp://10.0.0.239:1935/Media/Camera_00001__ReplayFMP4RecordFile__20220116154810",
								"http-flv": "http://10.0.0.239:8088/Media/Camera_00001__ReplayFMP4RecordFile__20220116154810.flv",
								"ws-flv": "ws://10.0.0.239:6088/Media/Camera_00001__ReplayFMP4RecordFile__20220116154810.flv",
								"http-mp4": "http://10.0.0.239:5088/Media/Camera_00001__ReplayFMP4RecordFile__20220116154810.mp4?download_speed=1",
								"download": "http://10.0.0.239:5088/Media/Camera_00001__ReplayFMP4RecordFile__20220116154810.mp4?download_speed=6"
							}
						},
						{
							"file": "20220116155110.mp4",
							"url": {
								"rtsp": "rtsp://10.0.0.239:554/Media/Camera_00001__ReplayFMP4RecordFile__20220116155110",
								"rtmp": "rtmp://10.0.0.239:1935/Media/Camera_00001__ReplayFMP4RecordFile__20220116155110",
								"http-flv": "http://10.0.0.239:8088/Media/Camera_00001__ReplayFMP4RecordFile__20220116155110.flv",
								"ws-flv": "ws://10.0.0.239:6088/Media/Camera_00001__ReplayFMP4RecordFile__20220116155110.flv",
								"http-mp4": "http://10.0.0.239:5088/Media/Camera_00001__ReplayFMP4RecordFile__20220116155110.mp4?download_speed=1",
								"download": "http://10.0.0.239:5088/Media/Camera_00001__ReplayFMP4RecordFile__20220116155110.mp4?download_speed=6"
							}
						}
					]
				}
	  19）、消息通知使用 
            功能说明：消息通知是流媒体服务器的一些消息比如无人观看、fmp4录像切片完成、播放时流地址不存在等等信息能及时的通知到另外一个http服务器上，需要此功能
			          消息通知功能用在什么地方，比如说无人观看消息通知，当收到无人观看消息时，国标服务器可以关闭国标发流，断开代理拉流，断开推流等等操作 
			          要使用此功能把配置文件的参数hook_enable 值设置为 1，同时通知的http服务器地址、端口号一定要设置对，下面列举出配置文件中的相关参数
 				 
				hook_enable=1                                                                  #事件通知部分,当 hook_enable=1 时，开启事件通知，hook_enable=0时关闭事件通知 
                on_stream_arrive=http://10.0.0.238:7088/index/hook/on_stream_arrive            #当某一路的码流达到时会通知一次
  				on_stream_none_reader=http://10.0.0.238:8080/index/hook/on_stream_none_reader  #当某一路流无人观看时，会触发该通知事件，接收端收到后可以进行断流操作
                on_stream_disconnect=http://10.0.0.238:7088/index/hook/on_stream_disconnect    #当某一路码流断开时会通知一次
				on_stream_not_found=http://10.0.0.238:8080/index/hook/on_stream_not_found      #播放时，找不到播放的码流，通过配合on_stream_none_reader事件可以完成按需拉流
 				on_record_mp4=http://10.0.0.238:8080/index/hook/on_record_mp4                  #录制完毕一段mp4文件通知
				
	            【注：http url的 IP，端口 是代表消息接收服务器的IP，端口，一定要填写正确，url 地址要合法，不要有空格 】
				1、当某一路码流到达时会发送通知：
					POST /index/hook/on_stream_arrive HTTP/1.1  # 根据 /index/hook/on_stream_arrive 这个可以判断是某一路码流到达
					Accept: */*
					Accept-Language: zh-CN,zh;q=0.8
					Connection: keep-alive
					Content-Length: 105
					Content-Type: application/json
					Host: 127.0.0.1
					Tools: ABLMediaServer-5.2.9(2022-03-28)
					User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2987.133 Safari/537.36

				  {
				   "app":"Media",                            # app 
				   "stream":"Camera_00001",                  # stream  
				   "mediaServerId":"ABLMediaServer_00001",   # 流媒体服务器编号 ，在配置文件可以配置，如果集群流媒体服务器时，可以给每台流媒体服务器起个编号 
				   "networkType":23,                         # 媒体流来源网络编号，可参考附表  
				   "key":130                                 # 媒体流来源编号，可以根据这个key进行关闭流媒体 可以调用delMediaStream或close_streams 函数进行关闭
				  }
				
				2、无人观看消息通知样例：
					POST /index/hook/on_stream_none_reader HTTP/1.1  # 根据 /index/hook/on_stream_none_reader 这个可以判断是无人观看消息通知
					Accept: */*
					Accept-Language: zh-CN,zh;q=0.8
					Connection: keep-alive
					Content-Length: 105
					Content-Type: application/json
					Host: 127.0.0.1
					Tools: ABLMediaServer-5.2.9(2022-03-28)
					User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2987.133 Safari/537.36

				  {
				   "app":"Media",                            # app 
				   "stream":"Camera_00001",                  # stream  
				   "mediaServerId":"ABLMediaServer_00001",   # 流媒体服务器编号 ，在配置文件可以配置，如果集群流媒体服务器时，可以给每台流媒体服务器起个编号 
				   "networkType":23,                         # 媒体流来源网络编号，可参考附表  
				   "key":130                                 # 媒体流来源编号，可以根据这个key进行关闭流媒体 可以调用delMediaStream或close_streams 函数进行关闭
				  }
				  
				 3、 fmp4录像切片录像完成一个文件时会发送一个消息通知 
					POST /index/hook/on_record_mp4 HTTP/1.1  # 根据 /index/hook/on_record_mp4 这个可以判断是mp4录像切片完毕一个通知 
					Accept: */*
					Accept-Language: zh-CN,zh;q=0.8
					Connection: keep-alive
					Content-Length: 127
					Content-Type: application/json
					Host: 127.0.0.1
					Tools: ABLMediaServer-5.2.9(2022-03-28)
					User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2987.133 Safari/537.36

					{
					  "app":"Media",                          # app 
					  "stream":"Camera_00001",                # stream  
					  "mediaServerId":"ABLMediaServer_00001", # 流媒体服务器编号 ，在配置文件可以配置，如果集群流媒体服务器时，可以给每台流媒体服务器起个编号 
					  "networkType":70,                       # 媒体流来源网络编号，可参考附表     
					  "fileName":"20220312212546.mp4"         # 录像切片完成的文件名字    
					} 

				4、当某一路码流断开时会发送通知：
					POST /index/hook/on_stream_disconnect HTTP/1.1  # 根据 /index/hook/on_stream_disconnect 这个可以判断是某一路码流断开
					Accept: */*
					Accept-Language: zh-CN,zh;q=0.8
					Connection: keep-alive
					Content-Length: 105
					Content-Type: application/json
					Host: 127.0.0.1
					Tools: ABLMediaServer-5.2.9(2022-03-28)
					User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2987.133 Safari/537.36

				  {
				   "app":"Media",                            # app 
				   "stream":"Camera_00001",                  # stream  
				   "mediaServerId":"ABLMediaServer_00001",   # 流媒体服务器编号 ，在配置文件可以配置，如果集群流媒体服务器时，可以给每台流媒体服务器起个编号 
				   "networkType":23,                         # 媒体流来源网络编号，可参考附表  
				   "key":130                                 # 媒体流来源编号，可以根据这个key进行关闭流媒体 可以调用delMediaStream或close_streams 函数进行关闭
				  }

				 5、 当播放一个url，如果不存在时，会发出一个消息通知 
					POST /index/hook/on_stream_not_found HTTP/1.1  # 根据 /index/hook/on_stream_not_found ，Http接收服务器得知流不不存在 
					Accept: */*
					Accept-Language: zh-CN,zh;q=0.8
					Connection: keep-alive
					Content-Length: 127
					Content-Type: application/json
					Host: 127.0.0.1
					Tools: ABLMediaServer-5.2.9(2022-03-28)
					User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_12_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/57.0.2987.133 Safari/537.36

					{
					  "app":"Media",                          # app      不存在的app
					  "stream":"Camera_00001",                # stream   不存在的stream 
					  "mediaServerId":"ABLMediaServer_00001"  # 流媒体服务器编号 ，在配置文件可以配置，如果集群流媒体服务器时，可以给每台流媒体服务器起个编号 
					} 
					
	  20) 图片抓拍 
			 URL: /index/api/getSnap
			 
			 功能：
				 查询某一接入的媒体源进行抓拍

			 参数：              参数说明     参数参考值
				secret           服务器密码   比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc        【必填参数】
				vhost                         比如 _defaultVhost_                              【可选参数】 
				app              应用名       比如 rtp、gb28181、Media 等等 ，自己起的名字     【必填参数】
				stream           媒体流名     比如 Camera_00001、44303403343034243200234 等等  【必填参数】
 				timeout_sec      超时时长     10  即本次抓拍最大超时时长 单位 秒               【必填参数】
 				  
	         http  GET 方式 
               http://127.0.0.1:7088/index/api/getSnap?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&vhost=_defaultVhost_&app=Media&stream=Camera_00001&timeout_sec=10
			   
			http  POST 方式 
			   http URL 
				 http://190.15.240.11:7088/index/api/getSnap
				 
 			   http Body 参数 json 格式
				 {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","vhost":"_defaultVhost_","app":"Media","stream":"Camera_00001","timeout_sec":10}
	  
            抓拍成功返回：
			{
				"code": 0,
				"memo": "success , Catpuring takes time 219 millisecond .",
				"url": "http://10.0.0.239:7088/Media/Camera_00001/2022031910034501.jpg"
			}
		
		21）图片列表查询
			 URL: /index/api/queryPictureList
			 
			 功能：
				 查询某一路输入源的抓拍图片列表 

			 参数：              参数说明     参数参考值
				secret           服务器密码   比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc        【必填参数】
				vhost                         比如 _defaultVhost_                              【可选参数】 
				app              应用名       比如 rtp、gb28181、Media 等等 ，自己起的名字     【必填参数】
				stream           媒体流名     比如 Camera_00001、44303403343034243200234 等等  【必填参数】
			    starttime        开始时间     比如 20220317081201 年月日时分秒                 【必填参数】
			    endtime          结束时间     比如 20220319231201 年月日时分秒                 【必填参数】
				
			【注意：1、开始时间必须小于 当前时间减去切片时长的时间 2、从 开始时间 至 结束时间 不能超过7天】
		
	         http  GET 方式 
               http://10.0.0.239:7088/index/api/queryPictureList?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&app=Media&stream=Camera_00001&starttime=20220317081201&endtime=20220319231201
			   
			http  POST 方式 
			   http URL 
				 http://190.15.240.11:7088/index/api/queryPictureList
				 
			   http Body 参数 json 格式
				 {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc","vhost":"_defaultVhost_","app":"Media","stream":"Camera_00001","starttime":"20220317081201","endtime":"20220319231201"}

              成功返回Body值 
				{
				"code": 0,
				"app": "Media",
				"stream": "Camera_00001",
				"starttime": "20220317081201",
				"endtime": "20220319231201",
				"PictureFileList": [
					{
						"file": "2022031816153857.jpg",
						"url": "http://10.0.0.239:7088/Media/Camera_00001/2022031816153857.jpg"
					},
					{
						"file": "2022031816153958.jpg",
						"url": "http://10.0.0.239:7088/Media/Camera_00001/2022031816153958.jpg"
					},
					{
						"file": "2022031816154059.jpg",
						"url": "http://10.0.0.239:7088/Media/Camera_00001/2022031816154059.jpg"
					},
			 	]
			  }	
		
	 22、修改某一路的水印相关参数
	 
			URL: index/api/setTransFilter
		 
		 功能：
			 修改某一路的水印相关参数，比水印的内容、颜色、字体大小、字体位置、字体透明度 

		 参数：              参数说明     参数参考值
			secret           服务器密码   比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc        【必填参数】
			vhost                         比如 _defaultVhost_                              【可选参数】 
			app              应用名       比如 rtp、gb28181、Media 等等 ，自己起的名字     【必填参数】
			stream           媒体流名     比如 Camera_00001、44303403343034243200234 等等  【必填参数】
			text             水印内容     比如 某某市某某公安局                  【必填参数】
			size             字体大小     20、30 、40 、50                       【必填参数】
			color            字体颜色     red,green,blue,white,black,
			alpha            透明度       0.1 ~  0.9 ,
			left             水印x坐标    比如 5 、 10 、20 
			top              水印y坐标    比如 5 、 10 、 20 
			trans            是否转换     固定为 1
		
		 http  POST 方式 
			 http://127.0.0.1:7088/index/api/setTransFilter
			 
		 Body 参数内容为 
				{
				"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc",
				"app" : "live",
				"stream" : "test",
				"text" : "ABL",
				"size" : 60,
				"color" : "red",
				"alpha" : 0.8,
				"left" : 40,
				"top" : 40,
				"trans" : 1
				}
				
		23、为了功能更新的需要，增加设置参数值的接口，可以单独设置 ABLMediaServer.ini 的某一个值，并且服务器不用重启，立即起效 
		
			URL: index/api/setConfigParamValue
		 
			 功能：
				为了功能更新的需要，增加设置参数值的接口，可以单独设置 ABLMediaServer.ini 的某一个值，并且服务器不用重启，立即起效 

			 参数：              参数说明     参数参考值
				secret           服务器密码   比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc        【必填参数】
				vhost                         比如 _defaultVhost_                              【可选参数】 
				key              参数名       比如 saveGB28181Rtp （保存接入的国标PS流）、 saveProxyRtspRtp （保存rtsp代理拉流的rtp流）
				                              还有 ABLMediaServer.ini 里面的配置参数，如果参数值不填就设置空，不是空格 
												mediaServerID = ABLMediaServer_00001
												secret = 035c73f7-bb6b-4889-a715-d9eb2d1925cc
												localipAddress = 
												maxTimeNoOneWatch = 9999999
												recordPath =  
												picturePath =  
												maxSameTimeSnap = 16
												snapOutPictureWidth = 0
												snapOutPictureHeight = 0
												snapObjectDestroy = 1
												snapObjectDuration = 120
												captureReplayType = 1
												pictureMaxCount = 30
												pushEnable_mp4 = 0
												fileSecond = 300
												videoFileFormat = 1
												fileKeepMaxTime = 3
												httpDownloadSpeed = 6
												fileRepeat = 0
												H265ConvertH264_enable = 0
												H265DecodeCpuGpuType = 0
												H264DecodeEncode_enable = 0
												filterVideo_enable = 0
												filterVideo_text = ABL水印测试123
												FilterFontSize = 30
												FilterFontColor = red
												FilterFontLeft = 5
												FilterFontTop = 5
												FilterFontAlpha = 0.6
												convertOutWidth = 720
												convertOutHeight = 480
												convertMaxObject = 26
												convertOutBitrate = 1024
												hook_enable = 0
												noneReaderDuration = 15
												on_server_started = http://10.0.0.238:4088/index/hook/on_server_started
												on_server_keepalive = http://10.0.0.238:4088/index/hook/on_server_keepalive
												on_stream_arrive = http://10.0.0.238:4088/index/hook/on_stream_arrive
												on_stream_not_arrive = http://10.0.0.238:4088/index/hook/on_stream_not_arrive
												on_stream_none_reader = http://10.0.0.238:4088/index/hook/on_stream_none_reader
												on_stream_disconnect = http://10.0.0.238:4088/index/hook/on_stream_disconnect
												on_stream_not_found = 
												on_record_mp4 = http://10.0.0.238:4088/index/hook/on_record_mp4
												on_delete_record_mp4 = http://10.0.0.238:4088/index/hook/on_delete_record_mp4
												on_record_progress = http://10.0.0.238:4088/index/hook/on_record_progress
												on_record_ts = http://10.0.0.238:4088/index/hook/on_record_ts
												httpServerPort = 7088
												rtspPort = 554
												rtmpPort = 1935
												httpMp4Port = 5088
												wsFlvPort = 6088
												httpFlvPort = 8088
												ps_tsRecvPort = 10000
												hls_enable = 0
												hlsPort = 9088
												hlsCutTime = 3
												hlsCutType = 2
												h265CutType = 1
												enable_audio = 1
												G711ConvertAAC = 0
												IOContentNumber = 16
												ThreadCountOfIOContent = 8
												RecvThreadCount = 128
												SendThreadCount = 128
												RecordReplayThread = 32
												GB28181RtpTCPHeadType = 2
												ReConnectingCount = 40320
												MaxDiconnectTimeoutSecond = 36
												ForceSendingIFrame = 1				                               
				value            参数值         详见 ABLMediaServer.ini 的参数值及参数值说明
				
			 http  GET 方式 
			　 比如：
				 打开保存国标PS标志
				　　 http://44.35.33.239:7088/index/api/setConfigParamValue?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&key=saveGB28181Rtp&value=1
				 关闭存国标PS标志
				　　 http://44.35.33.239:7088/index/api/setConfigParamValue?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&key=saveGB28181Rtp&value=0
		
				 打开保存代理拉ｒｔｓｐ流标志
				　　 http://44.35.33.239:7088/index/api/setConfigParamValue?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&key=saveProxyRtspRtp&value=1
				 关闭保存代理拉ｒｔｓｐ流标志
				　　 http://44.35.33.239:7088/index/api/setConfigParamValue?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc&key=saveProxyRtspRtp&value=0
	
	    24）安全停止服务器 
			 URL: /index/api/shutdownServer
			 
			 功能：
				 安全停止服务器，如果服务器正在录像、抓拍等等操作，需要调用该函数安全停止服务器，这样录制的mp4才能正常播放

			 参数：              参数说明     参数参考值
				secret           服务器密码   比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc        【必填参数】
 				  
	         http  GET 方式 
               http://127.0.0.1:7088/index/api/shutdownServer?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc
			   
			http  POST 方式 
			   http URL 
				 http://127.0.0.1:7088/index/api/shutdownServer
				 
			   http Body 参数 json 格式
				 {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc"}
				 
			 返回Body：
				{
					"code": 0,
					"memo": "ABLMediaServer shutdown Successed !"
				}	
				
	    25）重新启动服务器
			 URL: /index/api/restartServer
			 
			 功能：
				 安全重启服务器，如果服务器正在录像、抓拍等等操作，需要调用该函数安全重启服务器，这样录制的mp4才能正常播放

			 参数：              参数说明     参数参考值
				secret           服务器密码   比如 035c73f7-bb6b-4889-a715-d9eb2d1925cc        【必填参数】
 				  
	         http  GET 方式 
               http://127.0.0.1:7088/index/api/restartServer?secret=035c73f7-bb6b-4889-a715-d9eb2d1925cc
			   
			http  POST 方式 
			   http URL 
				 http://127.0.0.1:7088/index/api/restartServer
				 
			   http Body 参数 json 格式
				 {"secret":"035c73f7-bb6b-4889-a715-d9eb2d1925cc"}
				 
			 返回Body：
				{
					"code": 0,
					"memo": "ABLMediaServer restartServer Successed ! "
				}	

				
		26） 为了方便某些特殊场合，服务器支持udp的10000 端口接入国标PS码流，就是人们常说的单端口模式，url的命名规则为 /rtp/ssrc ,其中ssrc为下级rtp打包
		     的16进制的值转换为大小的字符串，即可sprintf(url,"rtp/%X",ssrc) ,具体接入的url名字可以调用 getMediaList 查询出接入的国标流 。需要注意的是 
			 下级 rtp 打包时每路视频的rtp中的ssrc不能相同。
			 
			  
	    27） 网络类型的对照表
	      1 媒体输入网络类型对照表 
		  
		      整形值     代表意义
	          21         以rtmp方式推送接入流媒体服务器
			  23         以rtsp方式推送接入流媒体服务器
			  30         服务器以rtsp方式主动拉流接入
			  31         服务器以rtmp方式主动拉流接入  
			  32         服务器以flv方式主动拉流接入
			  33         服务器以hls方式主动拉流接入
			  50         代理拉流接入服务器  
			  60         服务器以国标28181的UDP方式接入
			  61         服务器以国标28181的TCP方式接入
 			  
 	          80         服务器录像文件点播以读取fmp4文件输入
			  81         服务器录像文件点播以读取TS文件输入
			  82         服务器录像文件点播以读取PS文件输入
			  83         服务器录像文件点播以读取FLV文件输入
 
		  2 媒体输出网络类型对照表 	  
		     整形值     代表意义
		      22         服务器以rtsp被动方式往外发送码流 ，即常见的vlc点播 
			  24         服务器以rtmp被动方式往外发送码流 ，即常见的vlc点播 
			  25         服务器以flv被动方式往外发送码流 ，即常见的vlc点播 、浏览器播放 
			  26         服务器以hls被动方式往外发送码流 ，即常见的vlc点播 、浏览器播放 
	          27         服务器以ws-flv被动方式往外发送码流 ,EasyPlayer.js插件播放、浏览器播放 
			  28         服务器以http-mp4被动方式往外发送码流 ，即常见的vlc点播 、浏览器播放 
			  
			  40         服务器以rtsp主动方式往外发送码流 ，即常见的rtsp推流
			  41         服务器以rtmp主动方式往外发送码流 ，即常见的rtmp推流
			  65         服务器以国标GB28181主动UDP方式往外发送码流 ，即常见的国标以UDP方式往上级推流
			  66         服务器以国标GB28181主动TCP方式往外发送码流 ，即常见的国标以TCP方式往上级推流
			  			