INCLUDES = -I../boost/include
LIBS = -L../boost/lib -lboost_system -lboost_thread -lboost_log -lboost_regex -lboost_filesystem -lboost_date_time -lboost_chrono -lboost_atomic 

libXHNetSDK.so:client.o client_manager.o identifier_generator.o XHNetSDK.o ClientReadPool.o ClientSendPool.o ConnectCheckPool.o ListenPool.o MediaFifo.o UdpPool.o
	g++ -std=c++11  -fPIC -shared -fvisibility=hidden -o libXHNetSDK.so client.o client_manager.o identifier_generator.o XHNetSDK.o ClientReadPool.o ClientSendPool.o ConnectCheckPool.o ListenPool.o MediaFifo.o UdpPool.o $(LIBS) -lpthread 
client.o:client.cpp
	g++ -std=c++11   $(INCLUDES)  -fvisibility=hidden -fPIC  -c client.cpp 
client_manager.o:client_manager.cpp
	g++ -std=c++11    $(INCLUDES)  -fvisibility=hidden -fPIC  -c client_manager.cpp  
identifier_generator.o:identifier_generator.cpp
	g++  -std=c++11   $(INCLUDES)  -fvisibility=hidden -fPIC   -c identifier_generator.cpp    
XHNetSDK.o:XHNetSDK.cpp
	g++ -std=c++11    $(INCLUDES)  -fvisibility=hidden -fPIC   -c XHNetSDK.cpp  
ClientReadPool.o:ClientReadPool.cpp
	g++ -std=c++11    $(INCLUDES)  -fvisibility=hidden -fPIC   -c ClientReadPool.cpp  
ClientSendPool.o:ClientSendPool.cpp
	g++ -std=c++11    $(INCLUDES)  -fvisibility=hidden -fPIC   -c ClientSendPool.cpp   
ConnectCheckPool.o:ConnectCheckPool.cpp
	g++ -std=c++11    $(INCLUDES)  -fvisibility=hidden -fPIC   -c ConnectCheckPool.cpp  
ListenPool.o:ListenPool.cpp
	g++ -std=c++11    $(INCLUDES)  -fvisibility=hidden -fPIC   -c ListenPool.cpp  
MediaFifo.o:MediaFifo.cpp
	g++ -std=c++11   $(INCLUDES)   -fvisibility=hidden -fPIC  -c MediaFifo.cpp  
UdpPool.o:UdpPool.cpp
	g++ -std=c++11   $(INCLUDES)  -fvisibility=hidden -fPIC   -c UdpPool.cpp  
	
clean:
	rm *.o
	rm libXHNetSDK.so