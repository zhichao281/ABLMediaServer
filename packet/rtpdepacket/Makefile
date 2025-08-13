INCLUDES = -I../boost/include
LIBS = -L../boost/lib -lboost_system -lboost_thread -lboost_log -lboost_regex -lboost_filesystem -lboost_date_time -lboost_chrono -lboost_atomic 

libRtpDepacket.so:common.o depacket.o rtp_depacket.o session.o session_manager.o
	g++ -shared -fvisibility=hidden -fPIC -o libRtpDepacket.so common.o depacket.o rtp_depacket.o session.o session_manager.o $(LIBS) 
common.o:common.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c common.cpp 
depacket.o:depacket.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c depacket.cpp  
rtp_depacket.o:rtp_depacket.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c rtp_depacket.cpp 
session.o:session.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c session.cpp 
session_manager.o:session_manager.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c session_manager.cpp  
	
clean:
	rm *.o
	rm libRtpDepacket.so