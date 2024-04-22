
#include "ABLString.h"
#include <thread>
#include <algorithm>
#include<ctype.h>
#include "rapidjson/prettywriter.h"
#include "stdafx.h"
extern MediaServerPort                       ABL_MediaServerPort;
namespace ABL {

	char* strlwr(char* str)
	{
		if (str == NULL)
			return NULL;

		char* p = str;
		while (*p != '\0')
		{
			if (*p >= 'A' && *p <= 'Z')
				*p = (*p) + 0x20;
			p++;
		}
		return str;
	}

	inline char* strupr_(char* str)
	{
		char* origin = str;
		while (*str != '\0')
			*str++ = toupper(*str);
		return origin;
	}
	std::string& trim(std::string& s)
	{
		if (s.empty())
		{
			return s;
		}

		s.erase(0, s.find_first_not_of(" "));
		s.erase(s.find_last_not_of(" ") + 1);
		return s;
	}

	// 删除字符串中指定的字符串
	int erase_all(std::string& strBuf, const std::string& strDel)
	{
		if (strDel.empty())
		{
			return 0;  // 无需删除，返回删除次数为0
		}

		std::size_t pos = 0;
		int nCount = 0;

		while ((pos = strBuf.find(strDel, pos)) != std::string::npos)
		{
			strBuf.erase(pos, strDel.length());
			++nCount;
		}

		return nCount;
	}

	//std::string to_lower(std::string strBuf)
	//{
	//	if (strBuf.empty())
	//	{
	//		return "";
	//	}
	//	return strlwr((char*)strBuf.c_str());
	////	_strlwr_s((char*)strBuf.c_str(), strBuf.length() + 1);
	////	return strBuf;

	//}

	int	replace_all(std::string& strBuf, std::string  strSrc, std::string  strDes)
	{
		if (strSrc.empty())
		{
			return 0;  // 无需替换，返回替换次数为0
		}

		std::size_t pos = 0;
		int nCount = 0;

		while ((pos = strBuf.find(strSrc, pos)) != std::string::npos)
		{
			strBuf.replace(pos, strSrc.length(), strDes);
			pos += strDes.length();
			++nCount;
		}

		return nCount;

	}

	bool is_digits(const std::string& str)
	{
		return std::all_of(str.begin(), str.end(), ::isdigit); // C++11
	}

	/*
 *	Function:		StrToLwr
 *	Explanation:	字符串转小写
 *	Input:			strBuf		字符串
 *	Return:			小写字符串
 */

	void to_lower(char* str)
	{
		std::size_t length = std::strlen(str);
		for (std::size_t i = 0; i < length; ++i) {
			str[i] = std::tolower(static_cast<unsigned char>(str[i]));
		}
	}
	void to_lower(std::string& str)
	{
		for (char& c : str) {
			c = std::tolower(static_cast<unsigned char>(c));
		}
	}
	void to_upper(char* str)
	{
		std::size_t length = std::strlen(str);
		for (std::size_t i = 0; i < length; ++i) {
			str[i] = std::toupper(static_cast<unsigned char>(str[i]));
		}
	}
	void to_upper(std::string& str)
	{
		for (char& c : str) {
			c = std::toupper(static_cast<unsigned char>(c));
		}
	}
	std::string  StrToLwr(std::string  strBuf)
	{
		if (strBuf.empty())
		{
			return "";
		}
		return strlwr((char*)strBuf.c_str());
	//	_strlwr_s((char*)strBuf.c_str(), strBuf.length() + 1);
		return strBuf;
	}
	/*
*	Function:		StrToLwr
*	Explanation:	字符串转大写
*	Input:			strBuf		字符串
*	Return:			大写字符串
*/
	std::string  StrToUpr(std::string  strBuf)
	{
		if (strBuf.empty())
		{
			return "";
		}
		return strupr_((char*)strBuf.c_str());
	//	_strupr_s((char*)strBuf.c_str(), strBuf.length() + 1);
		//return strBuf;

	}

	// 定义一个用于获取当前时间的函数
	unsigned long long getCurrentTime()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch())
			.count();
	}

	void parseString(const std::string& input, std::string& szSection, std::string& szKey) {

		if (input.find(".") == std::string::npos)
		{
			szKey = input;
			szSection = "ABLMediaServer";
		}
		else
		{
			std::stringstream ss(input);
			std::getline(ss, szSection, '.');
			std::getline(ss, szKey, '.');
		}
	
	}

	std::string GetCurrentWorkingDirectory() {
		char buff[FILENAME_MAX];
		GetCurrentDir(buff, FILENAME_MAX);
		std::string current_working_dir(buff);
		return current_working_dir;
	}

	std::string IniToJson()
	{
		// 创建一个空的文档对象
		Document document;
		document.SetObject();
		// 创建一个数组保存所有的数据项
		Value params(kArrayType);
		Document::AllocatorType& allocator = document.GetAllocator();

		// 添加 secret 参数对象
		Value secret(kObjectType);
		secret.AddMember("secret", Value(ABL_MediaServerPort.secret, allocator).Move(), allocator);
		secret.AddMember("memo", "server password", allocator);
		params.PushBack(secret, allocator);

		// 添加 ServerIP 参数对象
		Value ServerIP(kObjectType);
		ServerIP.AddMember("ServerIP", Value(ABL_MediaServerPort.ABL_szLocalIP, allocator).Move(), allocator);
		ServerIP.AddMember("memo", "ABLMediaServer ip address", allocator);
		params.PushBack(ServerIP, allocator);

		// 添加 mediaServerID 参数对象
		Value mediaServerID(kObjectType);
		mediaServerID.AddMember("mediaServerID", Value(ABL_MediaServerPort.mediaServerID, allocator).Move(), allocator);
		mediaServerID.AddMember("memo", "media Server ID", allocator);
		params.PushBack(mediaServerID, allocator);

		// 添加 hook_enable 参数对象
		Value hook_enable(kObjectType);
		hook_enable.AddMember("hook_enable", ABL_MediaServerPort.hook_enable, allocator);
		hook_enable.AddMember("memo", "hook_enable = 1 open notice , hook_enable = 0 close notice", allocator);
		params.PushBack(hook_enable, allocator);

		// 添加 enable_audio 参数对象
		Value enable_audio(kObjectType);
		enable_audio.AddMember("enable_audio", ABL_MediaServerPort.nEnableAudio, allocator);
		enable_audio.AddMember("memo", "enable_audio = 1 open Audio , enable_audio = 0 Close Audio", allocator);
		params.PushBack(enable_audio, allocator);

		// 添加 httpServerPort 参数对象
		Value httpServerPort(kObjectType);
		httpServerPort.AddMember("httpServerPort", ABL_MediaServerPort.nHttpServerPort, allocator);
		httpServerPort.AddMember("memo", "http api port", allocator);
		params.PushBack(httpServerPort, allocator);

		// 添加 rtspPort 参数对象
		Value rtspPort(kObjectType);
		rtspPort.AddMember("rtspPort", ABL_MediaServerPort.nRtspPort, allocator);
		rtspPort.AddMember("memo", "rtsp port", allocator);
		params.PushBack(rtspPort, allocator);

		// 添加 rtmpPort 参数对象
		Value rtmpPort(kObjectType);
		rtmpPort.AddMember("rtmpPort", ABL_MediaServerPort.nRtmpPort, allocator);
		rtmpPort.AddMember("memo", "rtmp port", allocator);
		params.PushBack(rtmpPort, allocator);

		// 添加 ht	tpFlvPort 参数对象
		Value httpFlvPort(kObjectType);
		httpFlvPort.AddMember("httpFlvPort", ABL_MediaServerPort.nHttpFlvPort, allocator);
		httpFlvPort.AddMember("memo", "http-flv port", allocator);
		params.PushBack(httpFlvPort, allocator);

		// 添加 hls_enable 参数对象
		Value hls_enable(kObjectType);
		hls_enable.AddMember("hls_enable", ABL_MediaServerPort.nHlsEnable, allocator);
		hls_enable.AddMember("memo", "hls whether enable", allocator);
		params.PushBack(hls_enable, allocator);

		// 添加 hlsPort 参数对象
		Value hlsPort(kObjectType);
		hlsPort.AddMember("hlsPort", ABL_MediaServerPort.nHlsPort, allocator);
		hlsPort.AddMember("memo", "hls port", allocator);
		params.PushBack(hlsPort, allocator);

		// 添加 wsPort 参数对象
		Value wsPort(kObjectType);
		wsPort.AddMember("wsPort", ABL_MediaServerPort.nWSFlvPort, allocator);
		wsPort.AddMember("memo", "websocket flv port", allocator);
		params.PushBack(wsPort, allocator);

		// 添加 mp4Port 参数对象
		Value mp4Port(kObjectType);
		mp4Port.AddMember("mp4Port", ABL_MediaServerPort.nHttpMp4Port, allocator);
		mp4Port.AddMember("memo", "http mp4 port", allocator);
		params.PushBack(mp4Port, allocator);

		// 添加 ps_tsRecvPort 参数对象
		Value ps_tsRecvPort(kObjectType);
		ps_tsRecvPort.AddMember("ps_tsRecvPort", ABL_MediaServerPort.ps_tsRecvPort, allocator);
		ps_tsRecvPort.AddMember("memo", "recv ts , ps Stream port", allocator);
		params.PushBack(ps_tsRecvPort, allocator);

		// 添加 hlsCutType 参数对象
		Value hlsCutType(kObjectType);
		hlsCutType.AddMember("hlsCutType", ABL_MediaServerPort.nHLSCutType, allocator);
		hlsCutType.AddMember("memo", "hlsCutType = 1 hls cut to Harddisk,hlsCutType = 2 hls cut Media to memory", allocator);
		params.PushBack(hlsCutType, allocator);

		// 添加 h265CutType 参数对象
		Value h265CutType(kObjectType);
		h265CutType.AddMember("h265CutType", ABL_MediaServerPort.nH265CutType, allocator);
		h265CutType.AddMember("memo", "1 h265 cut TS , 2 cut fmp4", allocator);
		params.PushBack(h265CutType, allocator);

		// 添加 RecvThreadCount 参数对象
		Value RecvThreadCount(kObjectType);
		RecvThreadCount.AddMember("RecvThreadCount", ABL_MediaServerPort.nRecvThreadCount, allocator);
		RecvThreadCount.AddMember("memo", "RecvThreadCount", allocator);
		params.PushBack(RecvThreadCount, allocator);

		// 添加 SendThreadCount 参数对象
		Value SendThreadCount(kObjectType);
		SendThreadCount.AddMember("SendThreadCount", ABL_MediaServerPort.nSendThreadCount, allocator);
		SendThreadCount.AddMember("memo", "SendThreadCount", allocator);
		params.PushBack(SendThreadCount, allocator);

		// 添加 GB28181RtpTCPHeadType 参数对象
		Value GB28181RtpTCPHeadType(kObjectType);
		GB28181RtpTCPHeadType.AddMember("GB28181RtpTCPHeadType", ABL_MediaServerPort.nGBRtpTCPHeadType, allocator);
		GB28181RtpTCPHeadType.AddMember("memo", "rtp Length Type", allocator);
		params.PushBack(GB28181RtpTCPHeadType, allocator);

		// 添加 ReConnectingCount 参数对象
		Value ReConnectingCount(kObjectType);
		ReConnectingCount.AddMember("ReConnectingCount", ABL_MediaServerPort.nReConnectingCount, allocator);
		ReConnectingCount.AddMember("memo", "Try reconnections times", allocator);
		params.PushBack(ReConnectingCount, allocator);

		// 添加 maxTimeNoOneWatch 参数对象
		Value maxTimeNoOneWatch(kObjectType);
		maxTimeNoOneWatch.AddMember("maxTimeNoOneWatch", ABL_MediaServerPort.maxTimeNoOneWatch, allocator);
		maxTimeNoOneWatch.AddMember("memo", "maxTimeNoOneWatch", allocator);
		params.PushBack(maxTimeNoOneWatch, allocator);

		// 添加 pushEnable_mp4 参数对象
		Value pushEnable_mp4(kObjectType);
		pushEnable_mp4.AddMember("pushEnable_mp4", ABL_MediaServerPort.pushEnable_mp4, allocator);
		pushEnable_mp4.AddMember("memo", "pushEnable_mp4", allocator);
		params.PushBack(pushEnable_mp4, allocator);

		// 添加 fileSecond 参数对象
		Value fileSecond(kObjectType);
		fileSecond.AddMember("fileSecond", ABL_MediaServerPort.fileSecond, allocator);
		fileSecond.AddMember("memo", "fileSecond", allocator);
		params.PushBack(fileSecond, allocator);

		// 添加 fileKeepMaxTime 参数对象
		Value fileKeepMaxTime(kObjectType);
		fileKeepMaxTime.AddMember("fileKeepMaxTime", ABL_MediaServerPort.fileKeepMaxTime, allocator);
		fileKeepMaxTime.AddMember("memo", "fileKeepMaxTime", allocator);
		params.PushBack(fileKeepMaxTime, allocator);

		// 添加 httpDownloadSpeed 参数对象
		Value httpDownloadSpeed(kObjectType);
		httpDownloadSpeed.AddMember("httpDownloadSpeed", ABL_MediaServerPort.httpDownloadSpeed, allocator);
		httpDownloadSpeed.AddMember("memo", "httpDownloadSpeed", allocator);
		params.PushBack(httpDownloadSpeed, allocator);

		// 添加 RecordReplayThread 参数对象
		Value RecordReplayThread(kObjectType);
		RecordReplayThread.AddMember("RecordReplayThread", ABL_MediaServerPort.nRecordReplayThread, allocator);
		RecordReplayThread.AddMember("memo", "Total number of video playback threads", allocator);
		params.PushBack(RecordReplayThread, allocator);

		// 添加 convertMaxObject 参数对象
		Value convertMaxObject(kObjectType);
		convertMaxObject.AddMember("convertMaxObject", ABL_MediaServerPort.convertMaxObject, allocator);
		convertMaxObject.AddMember("memo", "Max number of video Convert", allocator);
		params.PushBack(convertMaxObject, allocator);

		// 添加 version 参数对象
		Value version(kObjectType);
		version.AddMember("version", Value(MediaServerVerson, allocator).Move(), allocator);
		version.AddMember("memo", "ABLMediaServer current Version", allocator);
		params.PushBack(version, allocator);

		// 添加 recordPath 参数对象
		Value recordPath(kObjectType);
		recordPath.AddMember("recordPath", Value(ABL_MediaServerPort.recordPath, allocator).Move(), allocator);
		recordPath.AddMember("memo", "ABLMediaServer Record File Path", allocator);
		params.PushBack(recordPath, allocator);

		// 添加 picturePath 参数对象
		Value picturePath(kObjectType);
		picturePath.AddMember("picturePath", Value(ABL_MediaServerPort.picturePath, allocator).Move(), allocator);
		picturePath.AddMember("memo", "ABLMediaServer Snap Picture Path", allocator);
		params.PushBack(picturePath, allocator);

		// 添加 noneReaderDuration 参数对象
		Value noneReaderDuration(kObjectType);
		noneReaderDuration.AddMember("noneReaderDuration", ABL_MediaServerPort.noneReaderDuration,  allocator);
		noneReaderDuration.AddMember("memo", "How many seconds does it take for no one to watch and send notifications  .", allocator);
		params.PushBack(noneReaderDuration, allocator);

		// 添加 on_server_started 参数对象
		Value on_server_started(kObjectType);
		on_server_started.AddMember("on_server_started", Value(ABL_MediaServerPort.on_server_started, allocator).Move(), allocator);
		on_server_started.AddMember("memo", "How many seconds does it take for no one to watch and send notifications  .", allocator);
		params.PushBack(on_server_started, allocator);

		// 添加 on_server_keepalive 参数对象
		Value on_server_keepalive(kObjectType);
		on_server_keepalive.AddMember("on_server_keepalive", Value(ABL_MediaServerPort.on_server_keepalive, allocator).Move(), allocator);
		on_server_keepalive.AddMember("memo", "How many seconds does it take for no one to watch and send notifications  .", allocator);
		params.PushBack(on_server_keepalive, allocator);

		// 添加 on_play 参数对象
		Value on_play(kObjectType);
		on_play.AddMember("on_play", Value(ABL_MediaServerPort.on_play, allocator).Move(), allocator);
		on_play.AddMember("memo", "Play a certain stream to send event notifications  ", allocator);
		params.PushBack(on_play, allocator);

		// 添加 on_publish 参数对象
		Value on_publish(kObjectType);
		on_publish.AddMember("on_publish", Value(ABL_MediaServerPort.on_publish, allocator).Move(), allocator);
		on_publish.AddMember("memo", "Registering a certain stream to the server to send event notifications  .", allocator);
		params.PushBack(on_publish, allocator);

		// 添加 on_stream_arrive 参数对象
		Value on_stream_arrive(kObjectType);
		on_stream_arrive.AddMember("on_stream_arrive", Value(ABL_MediaServerPort.on_stream_arrive, allocator).Move(), allocator);
		on_stream_arrive.AddMember("memo", "Send event notification when a certain media source stream reaches its destination  .", allocator);
		params.PushBack(on_stream_arrive, allocator);

		// 添加 on_stream_not_arrive 参数对象
		Value on_stream_not_arrive(kObjectType);
		on_stream_not_arrive.AddMember("on_stream_not_arrive", Value(ABL_MediaServerPort.on_stream_not_arrive, allocator).Move(), allocator);
		on_stream_not_arrive.AddMember("memo", "A certain media source was registered but the stream timed out and did not arrive. Send event notification  .", allocator);
		params.PushBack(on_stream_not_arrive, allocator);

		// 添加 on_stream_none_reader 参数对象
		Value on_stream_none_reader(kObjectType);
		on_stream_none_reader.AddMember("on_stream_none_reader", Value(ABL_MediaServerPort.on_stream_none_reader, allocator).Move(), allocator);
		on_stream_none_reader.AddMember("memo", "Send event notification when no one is watching a certain media source  .", allocator);
		params.PushBack(on_stream_none_reader, allocator);

		// 添加 on_stream_disconnect 参数对象
		Value on_stream_disconnect(kObjectType);
		on_stream_disconnect.AddMember("on_stream_disconnect", Value(ABL_MediaServerPort.on_stream_disconnect, allocator).Move(), allocator);
		on_stream_disconnect.AddMember("memo", "Send event notification when no one is watching a certain media source  .", allocator);
		params.PushBack(on_stream_disconnect, allocator);

		// 添加 on_stream_not_found 参数对象
		Value on_stream_not_found(kObjectType);
		on_stream_not_found.AddMember("on_stream_not_found", Value(ABL_MediaServerPort.on_stream_not_found, allocator).Move(), allocator);
		on_stream_not_found.AddMember("memo", "Media source not found Send event notification .", allocator);
		params.PushBack(on_stream_not_found, allocator);

		// 添加 on_record_mp4 参数对象
		Value on_record_mp4(kObjectType);
		on_record_mp4.AddMember("on_record_mp4", Value(ABL_MediaServerPort.on_record_mp4, allocator).Move(), allocator);
		on_record_mp4.AddMember("memo", "Send event notification when a recording is completed .", allocator);
		params.PushBack(on_record_mp4, allocator);

		// 添加 on_delete_record_mp4 参数对象
		Value on_delete_record_mp4(kObjectType);
		on_delete_record_mp4.AddMember("on_delete_record_mp4", Value(ABL_MediaServerPort.on_delete_record_mp4, allocator).Move(), allocator);
		on_delete_record_mp4.AddMember("memo", "Send event notification when a video recording is overwritten .", allocator);
		params.PushBack(on_delete_record_mp4, allocator);

		// 添加 on_record_progress 参数对象
		Value on_record_progress(kObjectType);
		on_record_progress.AddMember("on_record_progress", Value(ABL_MediaServerPort.on_record_progress, allocator).Move(), allocator);
		on_record_progress.AddMember("memo", "Sending event notifications every 1 second while recording .", allocator);
		params.PushBack(on_record_progress, allocator);

		// 添加 on_record_ts 参数对象
		Value on_record_ts(kObjectType);
		on_record_ts.AddMember("on_record_ts", Value(ABL_MediaServerPort.on_record_ts, allocator).Move(), allocator);
		on_record_ts.AddMember("memo", "Send event notification when hls slicing completes a section of ts file .", allocator);
		params.PushBack(on_record_ts, allocator);

		// 添加 enable_GetFileDuration 参数对象
		Value enable_GetFileDuration(kObjectType);
		enable_GetFileDuration.AddMember("enable_GetFileDuration", ABL_MediaServerPort.enable_GetFileDuration, allocator);
		enable_GetFileDuration.AddMember("memo", "Whether to enable the acquistition of record File duration  .", allocator);
		params.PushBack(enable_GetFileDuration, allocator);

		// 添加 keepaliveDuration 参数对象
		Value keepaliveDuration(kObjectType);
		keepaliveDuration.AddMember("keepaliveDuration", ABL_MediaServerPort.keepaliveDuration,  allocator);
		keepaliveDuration.AddMember("memo", "Time interval for sending heartbeat .", allocator);
		params.PushBack(keepaliveDuration, allocator);

		// 添加 captureReplayType 参数对象
		Value captureReplayType(kObjectType);
		captureReplayType.AddMember("captureReplayType", ABL_MediaServerPort.captureReplayType, allocator);
		captureReplayType.AddMember("memo", "Time interval for sending heartbeat .", allocator);
		params.PushBack(captureReplayType, allocator);

		// 添加 pictureMaxCount 参数对象
		Value pictureMaxCount(kObjectType);
		pictureMaxCount.AddMember("pictureMaxCount", ABL_MediaServerPort.pictureMaxCount, allocator);
		pictureMaxCount.AddMember("memo", "Maximum number of saved captured images  .", allocator);
		params.PushBack(pictureMaxCount, allocator);

		// 添加 videoFileFormat 参数对象
		Value videoFileFormat(kObjectType);
		videoFileFormat.AddMember("videoFileFormat", ABL_MediaServerPort.videoFileFormat, allocator);
		videoFileFormat.AddMember("memo", "Video files are in sliced format [1  fmp4 , 2  mp4 , 3  ts ]  .", allocator);
		params.PushBack(videoFileFormat, allocator);

		// 添加 MaxDiconnectTimeoutSecond 参数对象
		Value MaxDiconnectTimeoutSecond(kObjectType);
		MaxDiconnectTimeoutSecond.AddMember("MaxDiconnectTimeoutSecond", ABL_MediaServerPort.MaxDiconnectTimeoutSecond, allocator);
		MaxDiconnectTimeoutSecond.AddMember("memo", "Maximum timeout for recviving data  .", allocator);
		params.PushBack(MaxDiconnectTimeoutSecond, allocator);

		// 添加 G711ConvertAAC 参数对象
		Value G711ConvertAAC(kObjectType);
		G711ConvertAAC.AddMember("G711ConvertAAC", ABL_MediaServerPort.nG711ConvertAAC, allocator);
		G711ConvertAAC.AddMember("memo", "Do G711a and g711u transcode to aac  .", allocator);
		params.PushBack(G711ConvertAAC, allocator);

		// 添加 filterVideo_enable 参数对象
		Value filterVideo_enable(kObjectType);
		filterVideo_enable.AddMember("filterVideo_enable", ABL_MediaServerPort.filterVideo_enable, allocator);
		filterVideo_enable.AddMember("memo", "Do you want to turn on video watermark  .", allocator);
		params.PushBack(filterVideo_enable, allocator);

		// 添加 filterVideo_text 参数对象
		Value filterVideo_text(kObjectType);
		filterVideo_text.AddMember("filterVideo_text", Value(ABL_MediaServerPort.filterVideoText, allocator).Move(), allocator);
		filterVideo_text.AddMember("memo", "Time interval for sending heartbeat .", allocator);
		params.PushBack(filterVideo_text, allocator);

		// 添加 FilterFontSize 参数对象
		Value FilterFontSize(kObjectType);
		FilterFontSize.AddMember("FilterFontSize", ABL_MediaServerPort.nFilterFontSize,  allocator);
		FilterFontSize.AddMember("memo", "Time interval for sending heartbeat .", allocator);
		params.PushBack(FilterFontSize, allocator);

		// 添加 FilterFontColor 参数对象
		Value FilterFontColor(kObjectType);
		FilterFontColor.AddMember("FilterFontColor", Value(ABL_MediaServerPort.nFilterFontColor, allocator).Move(), allocator);
		FilterFontColor.AddMember("memo", "Set Video watermark font color .", allocator);
		params.PushBack(FilterFontColor, allocator);

		// 添加 FilterFontLeft 参数对象
		Value FilterFontLeft(kObjectType);
		FilterFontLeft.AddMember("FilterFontLeft", ABL_MediaServerPort.nFilterFontLeft, allocator);
		FilterFontLeft.AddMember("memo", "Set the left coordinate of th Video watermark  .", allocator);
		params.PushBack(FilterFontLeft, allocator);

		// 添加 FilterFontTop 参数对象
		Value FilterFontTop(kObjectType);
		FilterFontTop.AddMember("FilterFontTop", ABL_MediaServerPort.nFilterFontTop, allocator);
		FilterFontTop.AddMember("memo", "Set the top coordinate of th Video watermark  .", allocator);
		params.PushBack(FilterFontTop, allocator);

		// 添加 FilterFontAlpha 参数对象
		Value FilterFontAlpha(kObjectType);
		FilterFontAlpha.AddMember("FilterFontAlpha", ABL_MediaServerPort.nFilterFontAlpha, allocator);
		FilterFontAlpha.AddMember("memo", "Set the transparency of th Video watermark  .", allocator);
		params.PushBack(FilterFontAlpha, allocator);

		// 添加 convertOutWidth 参数对象
		Value convertOutWidth(kObjectType);
		convertOutWidth.AddMember("convertOutWidth", ABL_MediaServerPort.convertOutWidth, allocator);
		convertOutWidth.AddMember("memo", "Set transcoding video Width  .", allocator);
		params.PushBack(convertOutWidth, allocator);

		// 添加 convertOutHeight 参数对象
		Value convertOutHeight(kObjectType);
		convertOutHeight.AddMember("convertOutHeight", ABL_MediaServerPort.convertOutHeight, allocator);
		convertOutHeight.AddMember("memo", "Set transcoding video Height  .", allocator);
		params.PushBack(convertOutHeight, allocator);

		// 添加 convertOutBitrate 参数对象
		Value convertOutBitrate(kObjectType);
		convertOutBitrate.AddMember("convertOutBitrate", ABL_MediaServerPort.convertOutBitrate, allocator);
		convertOutBitrate.AddMember("memo", "Set th bitrate for video transcoding  ..", allocator);
		params.PushBack(convertOutBitrate, allocator);

		// 添加 webrtcPort 参数对象
		Value webrtcPort(kObjectType);
		webrtcPort.AddMember("webrtcPort", ABL_MediaServerPort.nWebRtcPort, allocator);
		webrtcPort.AddMember("memo", "WebRtc Player port  .", allocator);
		params.PushBack(webrtcPort, allocator);

		// 添加 WsRecvPcmPort 参数对象
		Value WsRecvPcmPort(kObjectType);
		WsRecvPcmPort.AddMember("WsRecvPcmPort", ABL_MediaServerPort.WsRecvPcmPort, allocator);
		WsRecvPcmPort.AddMember("memo", "the port for recv audio by Websocket .", allocator);
		params.PushBack(WsRecvPcmPort, allocator);

		// 添加 flvPlayAddMute 参数对象
		Value flvPlayAddMute(kObjectType);
		flvPlayAddMute.AddMember("flvPlayAddMute", ABL_MediaServerPort.flvPlayAddMute, allocator);
		flvPlayAddMute.AddMember("memo", "When playing HTTP FLV and WS FLV, do you want to turn on mute when there is no audio in the source stream .", allocator);
		params.PushBack(flvPlayAddMute, allocator);

		// 添加 gb28181LibraryUse 参数对象
		Value gb28181LibraryUse(kObjectType);
		gb28181LibraryUse.AddMember("gb28181LibraryUse", ABL_MediaServerPort.gb28181LibraryUse, allocator);
		gb28181LibraryUse.AddMember("memo", "When playing HTTP FLV and WS FLV, do you want to turn on mute when there is no audio in the source stream .", allocator);
		params.PushBack(gb28181LibraryUse, allocator);

		// 添加 listeningip 参数对象
		Value listeningip(kObjectType);
		listeningip.AddMember("rtc.listening-ip", Value(ABL_MediaServerPort.listeningip, allocator).Move(), allocator);
		listeningip.AddMember("memo", "webrtc listeningip .", allocator);
		params.PushBack(listeningip, allocator);

		Value listeningport(kObjectType);
		listeningport.AddMember("rtc.listening-port", ABL_MediaServerPort.listeningport,  allocator);
		listeningport.AddMember("memo", "webrtc listening port .", allocator);
		params.PushBack(listeningport, allocator);

		// 添加 externalip 参数对象
		Value externalip(kObjectType);
		externalip.AddMember("rtc.external-ip", Value(ABL_MediaServerPort.externalip, allocator).Move(), allocator);
		externalip.AddMember("memo", "webrtc externalip  .", allocator);
		params.PushBack(externalip, allocator);

		// 添加 realm 参数对象
		Value realm(kObjectType);
		realm.AddMember("rtc.realm", Value(ABL_MediaServerPort.realm, allocator).Move(), allocator);
		realm.AddMember("memo", "webrtc realm  .", allocator);
		params.PushBack(realm, allocator);

		// 添加 user 参数对象
		Value user(kObjectType);
		user.AddMember("rtc.user", Value(ABL_MediaServerPort.user, allocator).Move(), allocator);
		user.AddMember("memo", "webrtc user  .", allocator);
		params.PushBack(user, allocator);
		Value minport(kObjectType);
		minport.AddMember("rtc.min-port", ABL_MediaServerPort.minport, allocator);
		minport.AddMember("memo", "webrtc minport port .", allocator);
		params.PushBack(minport, allocator);
		Value maxport(kObjectType);
		maxport.AddMember("rtc.max-port", ABL_MediaServerPort.maxport, allocator);
		maxport.AddMember("memo", "webrtc listening port .", allocator);
		params.PushBack(maxport, allocator);

		// 将数组对象添加到主对象中
		document.AddMember("code", 0, allocator);
		document.AddMember("params", params, allocator);

		// 使用StringBuffer来保存生成的JSON数据
		StringBuffer buffer;
		Writer<StringBuffer> writer(buffer);
		document.Accept(writer);
		// 输出生成的JSON数据
		std::cout << buffer.GetString() << std::endl;
		return buffer.GetString();
	
	}

	std::string JsonToIni()
	{
		return std::string();
	}

}