libPsDemux.so:common.o demux.o demux_manager.o header_consumer.o pes_consumer.o pes_nonmedia_consumer.o ps_demux.o psm_consumer.o sysheader_consumer.o
	g++ -shared -fvisibility=hidden -fPIC -o libPsDemux.so common.o demux.o demux_manager.o header_consumer.o pes_consumer.o pes_nonmedia_consumer.o ps_demux.o psm_consumer.o sysheader_consumer.o $(LIBS) 
common.o:common.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c common.cpp 
demux.o:demux.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c demux.cpp 
demux_manager.o:demux_manager.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c demux_manager.cpp 
header_consumer.o:header_consumer.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c header_consumer.cpp  
pes_consumer.o:pes_consumer.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c pes_consumer.cpp 
pes_nonmedia_consumer.o:pes_nonmedia_consumer.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c pes_nonmedia_consumer.cpp
ps_demux.o:ps_demux.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c ps_demux.cpp 
psm_consumer.o:psm_consumer.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c psm_consumer.cpp  
sysheader_consumer.o:sysheader_consumer.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c sysheader_consumer.cpp 
	
clean:
	rm *.o
	rm libPsDemux.so