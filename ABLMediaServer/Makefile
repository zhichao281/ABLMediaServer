INCLUDES = -I../Linux/boost/include \
           -I../Linux/ffmpeg-6.0/include \
           -I../rapidjson-master/include \
           -I../media-server-master/libflv/include \
           -I../media-server-master/librtmp/include \
		   -I../media-server-master/libmpeg/include \
		   -I../media-server-master/libhls/include \
		   -I../media-server-master/librtp/include \
		   -I../media-server-master/libmov/include 
LIBS = -L../bin \
       -L../Linux/ffmpeg-6.0/lib \
       -L../Linux/faac-1.29.9.2/lib/lib \
       -L../Linux/boost/lib -lboost_system -lboost_thread -lboost_log -lboost_regex -lboost_filesystem -lboost_date_time -lboost_chrono -lboost_atomic -lrtmp -lflv -lmpeg -lhls -lmov -lrtp -lfaac -lavcodec -lavutil -lavformat -lswscale -lavfilter -lpostproc -lswresample

ABLMediaServer:ABLMediaServer.o ABLogFile.o ABLSipParse.o CNetRtmpServerRecv.o Ini.o DigestAuthentication.o LCbase64.o MediaFifo.o MediaSendThreadPool.o \
               MediaStreamSource.o netBase64.o NetBaseThreadPool.o NetClientAddPushProxy.o NetClientAddStreamProxy.o NetClientRecvFLV.o NetClientRecvHttpHLS.o NetClientRecvRtmp.o \
               NetClientRecvRtsp.o NetClientSendRtmp.o NetClientSendRtsp.o NetGB28181RtpClient.o NetGB28181RtpServer.o NetRecvBase.o NetRtspServer.o \
			   NetServerHLS.o NetServerHTTP.o NetServerHTTP_FLV.o ourMD5.o RtcpPacket.o strDup.o SHA1.o NetServerWS_FLV.o NetServerHTTP_MP4.o \
			   StreamRecordFMP4.o RecordFileSource.o ReadRecordFileInput.o g711.o g711_table.o AACEncode.o NetClientHttp.o VideoDecode.o PictureFileSource.o \
			   NetClientSnap.o AVFrameSWS.o FFVideoEncode.o FFVideoDecode.o NetServerRecvRtpTS_PS.o RtpPSStreamInput.o RtpTSStreamInput.o NetGB28181Listen.o StreamRecordMP4.o
	g++ -std=c++11 -o ABLMediaServer ABLMediaServer.o ABLogFile.o ABLSipParse.o CNetRtmpServerRecv.o Ini.o DigestAuthentication.o LCbase64.o MediaFifo.o MediaSendThreadPool.o \
		   MediaStreamSource.o netBase64.o NetBaseThreadPool.o NetClientAddPushProxy.o NetClientAddStreamProxy.o NetClientRecvFLV.o NetClientRecvHttpHLS.o NetClientRecvRtmp.o \
		   NetClientRecvRtsp.o NetClientSendRtmp.o NetClientSendRtsp.o NetGB28181RtpClient.o NetGB28181RtpServer.o NetRecvBase.o NetRtspServer.o NetGB28181Listen.o \
		   NetServerHLS.o NetServerHTTP.o NetServerHTTP_FLV.o ourMD5.o RtcpPacket.o strDup.o SHA1.o NetServerWS_FLV.o NetServerHTTP_MP4.o StreamRecordFMP4.o \
		   RecordFileSource.o ReadRecordFileInput.o g711.o g711_table.o AACEncode.o NetClientHttp.o  VideoDecode.o PictureFileSource.o NetClientSnap.o AVFrameSWS.o \
		   FFVideoEncode.o FFVideoDecode.o NetServerRecvRtpTS_PS.o RtpPSStreamInput.o RtpTSStreamInput.o StreamRecordMP4.o $(LIBS) -lpthread -lXHNetSDK -lPsDemux -lPsMux -lRtpDepacket -lRtpPacket -ldl
ABLMediaServer.o:ABLMediaServer.cpp
	g++ -std=c++11 $(INCLUDES) -c ABLMediaServer.cpp
ABLogFile.o:ABLogFile.cpp
	g++ -std=c++11 $(INCLUDES) -c ABLogFile.cpp
ABLSipParse.o:ABLSipParse.cpp
	g++ -std=c++11 $(INCLUDES) -c ABLSipParse.cpp	
CNetRtmpServerRecv.o:CNetRtmpServerRecv.cpp
	g++ -std=c++11 $(INCLUDES) -c CNetRtmpServerRecv.cpp	
Ini.o:Ini.cpp
	g++ -std=c++11 $(INCLUDES) -c Ini.cpp	
DigestAuthentication.o:DigestAuthentication.cpp
	g++ -std=c++11 $(INCLUDES) -c DigestAuthentication.cpp	
LCbase64.o:LCbase64.cpp
	g++ -std=c++11 $(INCLUDES) -c LCbase64.cpp	
MediaFifo.o:MediaFifo.cpp
	g++ -std=c++11 $(INCLUDES) -c MediaFifo.cpp	
MediaSendThreadPool.o:MediaSendThreadPool.cpp
	g++ -std=c++11 $(INCLUDES) -c MediaSendThreadPool.cpp	
MediaStreamSource.o:MediaStreamSource.cpp
	g++ -std=c++11 $(INCLUDES) -c MediaStreamSource.cpp	
netBase64.o:netBase64.cpp
	g++ -std=c++11 $(INCLUDES) -c netBase64.cpp	
NetBaseThreadPool.o:NetBaseThreadPool.cpp
	g++ -std=c++11 $(INCLUDES) -c NetBaseThreadPool.cpp	
NetClientAddPushProxy.o:NetClientAddPushProxy.cpp
	g++ -std=c++11 $(INCLUDES) -c NetClientAddPushProxy.cpp	
NetClientAddStreamProxy.o:NetClientAddStreamProxy.cpp
	g++ -std=c++11 $(INCLUDES) -c NetClientAddStreamProxy.cpp	
NetClientRecvFLV.o:NetClientRecvFLV.cpp
	g++ -std=c++11 $(INCLUDES) -c NetClientRecvFLV.cpp	
NetClientRecvHttpHLS.o:NetClientRecvHttpHLS.cpp
	g++ -std=c++11 $(INCLUDES) -c NetClientRecvHttpHLS.cpp	
NetClientRecvRtmp.o:NetClientRecvRtmp.cpp
	g++ -std=c++11 $(INCLUDES) -c NetClientRecvRtmp.cpp	
NetClientRecvRtsp.o:NetClientRecvRtsp.cpp
	g++ -std=c++11 $(INCLUDES) -c NetClientRecvRtsp.cpp	
NetClientSendRtmp.o:NetClientSendRtmp.cpp
	g++ -std=c++11 $(INCLUDES) -c NetClientSendRtmp.cpp	
NetClientSendRtsp.o:NetClientSendRtsp.cpp
	g++ -std=c++11 $(INCLUDES) -c NetClientSendRtsp.cpp	
NetGB28181RtpClient.o:NetGB28181RtpClient.cpp
	g++ -std=c++11 $(INCLUDES) -c NetGB28181RtpClient.cpp	
NetGB28181RtpServer.o:NetGB28181RtpServer.cpp
	g++ -std=c++11 $(INCLUDES) -c NetGB28181RtpServer.cpp	
NetRecvBase.o:NetRecvBase.cpp
	g++ -std=c++11 $(INCLUDES) -c NetRecvBase.cpp	
NetRtspServer.o:NetRtspServer.cpp
	g++ -std=c++11 $(INCLUDES) -c NetRtspServer.cpp	
NetServerHLS.o:NetServerHLS.cpp
	g++ -std=c++11 $(INCLUDES) -c NetServerHLS.cpp	
NetServerHTTP.o:NetServerHTTP.cpp
	g++ -std=c++11 $(INCLUDES) -c NetServerHTTP.cpp	
NetServerHTTP_FLV.o:NetServerHTTP_FLV.cpp
	g++ -std=c++11 $(INCLUDES) -c NetServerHTTP_FLV.cpp	
ourMD5.o:ourMD5.cpp
	g++ -std=c++11 $(INCLUDES) -c ourMD5.cpp	
RtcpPacket.o:RtcpPacket.cpp
	g++ -std=c++11 $(INCLUDES) -c RtcpPacket.cpp	
strDup.o:strDup.cpp
	g++ -std=c++11 $(INCLUDES) -c strDup.cpp		
SHA1.o:SHA1.cpp
	g++ -std=c++11 $(INCLUDES) -c SHA1.cpp		
NetServerWS_FLV.o:NetServerWS_FLV.cpp
	g++ -std=c++11 $(INCLUDES) -c NetServerWS_FLV.cpp
NetServerHTTP_MP4.o:NetServerHTTP_MP4.cpp
	g++ -std=c++11 $(INCLUDES) -c NetServerHTTP_MP4.cpp	
StreamRecordFMP4.o:StreamRecordFMP4.cpp
	g++ -std=c++11 $(INCLUDES) -c StreamRecordFMP4.cpp
StreamRecordMP4.o:StreamRecordMP4.cpp
	g++ -std=c++11 $(INCLUDES) -c StreamRecordMP4.cpp
RecordFileSource.o:RecordFileSource.cpp
	g++ -std=c++11 $(INCLUDES) -c RecordFileSource.cpp
ReadRecordFileInput.o:ReadRecordFileInput.cpp
	g++ -std=c++11 $(INCLUDES) -c ReadRecordFileInput.cpp
g711.o:g711.cpp
	g++ -std=c++11 $(INCLUDES) -c g711.cpp
g711_table.o:g711_table.cpp
	g++ -std=c++11 $(INCLUDES) -c g711_table.cpp
AACEncode.o:AACEncode.cpp
	g++ -std=c++11 $(INCLUDES) -c AACEncode.cpp
NetClientHttp.o:NetClientHttp.cpp
	g++ -std=c++11 $(INCLUDES) -c NetClientHttp.cpp
VideoDecode.o:VideoDecode.cpp
	g++ -std=c++11 $(INCLUDES) -c VideoDecode.cpp
PictureFileSource.o:PictureFileSource.cpp
	g++ -std=c++11 $(INCLUDES) -c PictureFileSource.cpp
NetClientSnap.o:NetClientSnap.cpp
	g++ -std=c++11 $(INCLUDES) -c NetClientSnap.cpp
AVFrameSWS.o:AVFrameSWS.cpp
	g++ -std=c++11 $(INCLUDES) -c AVFrameSWS.cpp
FFVideoEncode.o:FFVideoEncode.cpp
	g++ -std=c++11 $(INCLUDES) -c FFVideoEncode.cpp
FFVideoDecode.o:FFVideoDecode.cpp
	g++ -std=c++11 $(INCLUDES) -c FFVideoDecode.cpp
NetServerRecvRtpTS_PS.o:NetServerRecvRtpTS_PS.cpp
	g++ -std=c++11 $(INCLUDES) -c NetServerRecvRtpTS_PS.cpp
RtpPSStreamInput.o:RtpPSStreamInput.cpp
	g++ -std=c++11 $(INCLUDES) -c RtpPSStreamInput.cpp
RtpTSStreamInput.o:RtpTSStreamInput.cpp
	g++ -std=c++11 $(INCLUDES) -c RtpTSStreamInput.cpp
NetGB28181Listen.o:NetGB28181Listen.cpp
	g++ -std=c++11 $(INCLUDES) -c NetGB28181Listen.cpp

clean:
	rm *.o
	rm ABLMediaServer