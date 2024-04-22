

#include <map>
#include "rtc_obj_sdk.h"
#include "PeerConnectionManager.h"
#include "rtc_base/ssl_adapter.h"
#include "rtc_base/thread.h"
#include "p2p/base/basic_packet_socket_factory.h"
#include "system_wrappers/include/field_trial.h"
#include "p2p/base/port_interface.h"
#include "rtc_base/async_udp_socket.h"
#include "rtc_base/ip_address.h"
#include "rtc_base/physical_socket_server.h"
#include "rtc_base/socket_address.h"
#include "rtc_base/checks.h"
#include "turn_server.h"

#include "NSJson.h"
#include "turn_tool.h"
#include "SimpleIni.h"

#include "spdloghead.h"
#if (defined _WIN32 || defined _WIN64)
#include "rtc_base/win32_socket_init.h"
#endif

class RtcLogSink :public rtc::LogSink {
public:
	RtcLogSink() {}
	~RtcLogSink() {}
	virtual void OnLogMessage(const std::string& message)
	{
		std::cout<<"RtcLog OnLogMessage ="<< message;
		static FILE* file = fopen("./rtc.log", "ab+");
		if (file)
		{
			std::string tmp = message + "\r\n";
			fwrite(tmp.c_str(), 1, tmp.length(), file);
			fflush(file);
		}
	}
};



namespace {
	const char kSoftware[] = "libjingle TurnServer";

	class TurnFileAuth : public cricket::TurnAuthInterface {
	public:
		TurnFileAuth() {};
		explicit TurnFileAuth(std::map<std::string, std::string> name_to_key)
			: name_to_key_(std::move(name_to_key)) {}

		void SetKey(std::map<std::string, std::string> name_to_key) {
			name_to_key_ = std::move(name_to_key);
		}
	public:
		virtual bool GetKey(const std::string& username,
			const std::string& realm,
			std::string* key) {
			// File is stored as lines of <username>=<HA1>.
			// Generate HA1 via "echo -n "<username>:<realm>:<password>" | md5sum"
			auto it = name_to_key_.find(username);
			if (it == name_to_key_.end())
				return false;
			*key = it->second;
			return true;
		}

	private:
		std::map<std::string, std::string> name_to_key_;
	};

}  // namespace

TurnFileAuth auth;

void WebRtcEndpoint::init(const char* webrtcConfig, std::function<void(const char* callbackJson, void* pUserHandle)> callback)
{
	if (!bInit.load())
	{
		bInit.store(true);
		m_callback = callback;
		webrtc::PeerConnectionInterface::IceTransportsType transportType = webrtc::PeerConnectionInterface::IceTransportsType::kAll;
		std::string webrtcTrialsFields = "WebRTC-FrameDropper/Disabled/";
		webrtc::field_trial::InitFieldTrialsFromString(webrtcTrialsFields.c_str());
		// webrtc server
		std::list<std::string> iceServerList;
		std::string publishFilter(".*");
		Json::Value config;
		bool        useNullCodec = true;
		bool        usePlanB = false;
		int         maxpc = 0;
		std::string localWebrtcUdpPortRange = "0:65535";
		iceServerList.push_back(std::string("turn:") + "admin:admin@175.178.213.69:3478?transport=udp");
		webrtc::AudioDeviceModule::AudioLayer audioLayer = webrtc::AudioDeviceModule::kPlatformDefaultAudio;
		webRtcServer = new PeerConnectionManager(iceServerList, config["urls"], audioLayer, publishFilter, localWebrtcUdpPortRange, useNullCodec, usePlanB, maxpc, transportType);	if (!webRtcServer->InitializePeerConnection())
		{
			std::cout << "Cannot Initialize WebRTC server" << std::endl;
		}
		std::map<std::string, HttpServerRequestHandler::httpFunction> func = webRtcServer->getHttpApi();

		webRtcServer->setCallBcak(callback);
		// http server
		const char* webroot = "./html";

		std::string httpAddress("0.0.0.0:");
		std::string httpPort =std::to_string(ABL::NSJson::ParseStr(webrtcConfig).GetInt("webrtcPort"));
		createIceServers(webrtcConfig);

		if (httpPort.size()<1)
		{
			httpPort = "8000";
		}
		httpAddress.append(httpPort);


		std::vector<std::string> options;
		options.push_back("document_root");
		options.push_back(webroot);
		options.push_back("enable_directory_listing");
		options.push_back("no");

		options.push_back("access_control_allow_origin");
		options.push_back("*");
		options.push_back("listening_ports");
		options.push_back(httpAddress);
		options.push_back("enable_keep_alive");
		options.push_back("yes");
		options.push_back("keep_alive_timeout_ms");
		options.push_back("1000");
		options.push_back("decode_url");
		options.push_back("no");
		httpServer = new HttpServerRequestHandler(func, options);
		
	}
}

void WebRtcEndpoint::Uninit()
{
	if (httpServer)
	{
		delete httpServer;
		httpServer = nullptr;
	}
	if (webRtcServer)
	{
		delete webRtcServer;
		webRtcServer = nullptr;
	}
	bInit.store(false);

}

bool WebRtcEndpoint::stopWebRtcPlay(const char* peerid)
{
	if (webRtcServer)
	{
		webRtcServer->hangUp(peerid);
	}


	return false;
}

bool WebRtcEndpoint::deleteWebRtcSource(const char* szMediaSource)
{
	VideoCaptureManager::getInstance().RemoveInput(szMediaSource);
	return false;
}

void WebRtcEndpoint::createIceServers(const char* webrtcConfig)
{

	//--user=admin:admin --min-port=50000 --max-port=65535 --listening-ip=192.168.2.5 --listening-port=3478 --realm=yunshihome.com
	 // 创建 SimpleIni 对象
	CSimpleIniA ini;
	// 读取 INI 文件
	SI_Error rc = ini.LoadFile("ABLMediaServer.ini");
	if (rc < 0) {
		std::cerr << "Failed to load INI file" << std::endl;
		return ;
	}
	// 读取配置项
	std::string listeningIp = ini.GetValue("rtc", "listening-ip", "127.0.0.1");
	std::string username = ini.GetValue("rtc", "user", "admin:admin");
	std::string realm = ini.GetValue("rtc", "realm", "yunshiting.com");
	std::string externalIp = ini.GetValue("rtc", "external-ip", "127.0.0.1");
	int listeningPort= ini.GetLongValue("rtc", "listening-port", 3478);
	int minPort = ini.GetLongValue("rtc", "min-port", 49152);
	int maxPort = ini.GetLongValue("rtc", "max-port", 65535);

	//std::string turnurl = ini.GetValue("rtc", "turnurl", "admin:admin@127.0.0.1:3478?transport=udp");
	std::string turnurl = ini.GetValue("rtc", "turnurl", "");
	if (!turnurl.empty())
	{
		webRtcServer->addIceServers(turnurl);
	}
	createIceServers(username, realm, externalIp, listeningIp, listeningPort, minPort, maxPort);
	// 设置值并添加注释
	//SI_Error result = ini.SetValue("rtc", "timeoutSec1", "2042", "# Timeout for RTC in seconds",true);
	//if (result == SI_UPDATED) {
	//	std::cout << "Value updated successfully." << std::endl;
	//}
	//else if (result == SI_INSERTED) {
	//	std::cout << "Value inserted successfully." << std::endl;
	//}
	//else {
	//	std::cerr << "Error occurred while setting value." << std::endl;
	//	return ;
	//}

	// //保存修改后的INI文件
	//ini.SaveFile("ABLMediaServer.ini");

}

void WebRtcEndpoint::createIceServers(std::string username, std::string realm,
	std::string externalIp, std::string listeningIp,
	int listeningPort, int minPort,int maxPort)
{
	m_turnThread = new std::thread([=]() {
				
		rtc::PhysicalSocketServer socket_server;
		rtc::AutoSocketServerThread main(&socket_server);
		//rtc::ThreadManager::Instance()->SetCurrentThread(thread);
		std::string  strAuth = setAuth(username, realm);
		auth.SetKey(ReadAuth(strAuth));
		std::string addr = listeningIp + ":" + std::to_string(listeningPort);
		rtc::SocketAddress server_addr;
		server_addr.FromString(addr);
		std::unique_ptr<cricket::TurnServer> turnserver;
		turnserver.reset(new cricket::TurnServer(&main));
		turnserver->set_realm(realm);
		turnserver->set_auth_hook(&auth);
		turnserver->set_port(minPort, maxPort);

		rtc::Socket* tcp_server_socket = socket_server.CreateSocket(AF_INET, SOCK_STREAM);
		if (tcp_server_socket) {
			std::cout << "TURN Listening TCP at " << server_addr.ToString() << std::endl;
			tcp_server_socket->Bind(server_addr);
			tcp_server_socket->Listen(5);
			turnserver->AddInternalServerSocket(tcp_server_socket, cricket::PROTO_TCP);
		}
		else {
			std::cout << "Failed to create TURN TCP server socket" << std::endl;
		}

		rtc::AsyncUDPSocket* udp_server_socket = rtc::AsyncUDPSocket::Create(&socket_server, server_addr);
		if (udp_server_socket) {
			std::cout << "TURN Listening UDP at " << server_addr.ToString() << std::endl;
			turnserver->AddInternalSocket(udp_server_socket, cricket::PROTO_UDP);
		}
		else {
			std::cout << "Failed to create TURN UDP server socket" << std::endl;
		}
		rtc::SocketAddress external_server_addr;
		external_server_addr.FromString(externalIp + ":" + std::to_string(listeningPort));
		std::cout << "TURN external addr:" << external_server_addr.ToString() << std::endl;
		turnserver->SetExternalSocketFactory(new rtc::BasicPacketSocketFactory(&socket_server), rtc::SocketAddress(external_server_addr.ipaddr(), 0));
		std::string localturn = username + "@" + externalIp + ":" + std::to_string(listeningPort)+"?transport=udp";
		webRtcServer->addIceServers(localturn);
		main.Run();


		});



}

WebRtcEndpoint& WebRtcEndpoint::getInstance()
{
	static WebRtcEndpoint instance;
	return instance;
}

WebRtcEndpoint::WebRtcEndpoint()
{
#if (defined _WIN32 || defined _WIN64)
	rtc::WinsockInitializer winsock_init;
#endif

	rtc::PhysicalSocketServer ss;
	rtc::AutoSocketServerThread main_thread(&ss);
	spdlog::SPDLOG::getInstance().init("log/webrtc-streamer.txt", "webrtc-streamer");

	rtc::InitializeSSL();
	bInit.store(false);

}


