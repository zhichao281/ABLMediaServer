INCLUDES = 
LIBS = 

libRtpDepacket.so:depacket.o rtpdepacket.o
	g++ -std=c++11  -fPIC -shared -fvisibility=hidden -o libRtpDepacket.so depacket.o rtpdepacket.o $(LIBS)
depacket.o:depacket.cpp
	g++ -std=c++11   $(INCLUDES)  -fvisibility=hidden -fPIC  -c depacket.cpp 
rtpdepacket.o:rtpdepacket.cpp
	g++ -std=c++11   $(INCLUDES)  -fvisibility=hidden -fPIC  -c rtpdepacket.cpp  
	
clean:
	rm *.o
	rm libRtpDepacket.so