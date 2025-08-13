INCLUDES = -I../boost/include
LIBS = -L../boost/lib -lboost_system -lboost_thread -lboost_log -lboost_regex -lboost_filesystem -lboost_date_time -lboost_chrono -lboost_atomic 

libPsMux.so:common.o mux.o mux_manager.o ps_mux.o
	g++ -shared -fvisibility=hidden -fPIC -o libPsMux.so common.o mux.o mux_manager.o ps_mux.o $(LIBS)
common.o:common.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c common.cpp 
mux.o:mux.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c mux.cpp 
mux_manager.o:mux_manager.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c mux_manager.cpp 
ps_mux.o:ps_mux.cpp
	g++ $(INCLUDES) -fvisibility=hidden -fPIC -c ps_mux.cpp 
	
clean:
	rm *.o
	rm libPsMux.so