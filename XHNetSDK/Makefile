INCLUDES = -I../boost/include
LIBS = -L../boost/lib -lboost_system -lboost_thread -lboost_log -lboost_regex -lboost_filesystem -lboost_date_time -lboost_chrono -lboost_atomic 

libXHNetSDK.so:client.o client_manager.o identifier_generator.o io_context_pool.o libnet.o server.o server_manager.o udp_session.o udp_session_manager.o circular_buffer.o
	g++ -fPIC -shared -o libXHNetSDK.so client.o client_manager.o identifier_generator.o io_context_pool.o libnet.o server.o server_manager.o udp_session.o udp_session_manager.o circular_buffer.o $(LIBS) -lpthread 
client.o:client.cpp
	g++ $(INCLUDES) -c client.cpp  -fPIC
client_manager.o:client_manager.cpp
	g++ $(INCLUDES) -c client_manager.cpp  -fPIC
identifier_generator.o:identifier_generator.cpp
	g++ $(INCLUDES) -c identifier_generator.cpp  -fPIC
io_context_pool.o:io_context_pool.cpp
	g++ $(INCLUDES) -c io_context_pool.cpp  -fPIC
libnet.o:libnet.cpp
	g++ $(INCLUDES) -c libnet.cpp  -fPIC
server.o:server.cpp
	g++ $(INCLUDES) -c server.cpp  -fPIC
server_manager.o:server_manager.cpp
	g++ $(INCLUDES) -c server_manager.cpp  -fPIC
udp_session.o:udp_session.cpp
	g++ $(INCLUDES) -c udp_session.cpp  -fPIC
udp_session_manager.o:udp_session_manager.cpp
	g++ $(INCLUDES) -c udp_session_manager.cpp  -fPIC
circular_buffer.o:circular_buffer.cpp
	g++ $(INCLUDES) -c circular_buffer.cpp  -fPIC
	
clean:
	rm *.o
	rm libXHNetSDK.so