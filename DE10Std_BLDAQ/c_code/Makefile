#
TARGET = bin/MainTDAQServer

#
CROSS_COMPILE = arm-linux-gnueabihf-
CFLAGS = -static -g -Wall  -I${SOCEDS_DEST_ROOT}/ip/altera/hps/altera_hps/hwlib/include
CXXFLAGS = -static -std=c++11 -g -Wall  -I${SOCEDS_DEST_ROOT}/ip/altera/hps/altera_hps/hwlib/include -Isrc
LDFLAGS =  -g -Wall  -lpthread
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
ARCH= arm


build: $(TARGET) bin/printInfo bin/MainTDAQStandAlone bin/testRegs bin/BLDAQScope
	echo -e "\n\n  Executables ready in the bin directory \n\n"
	ls -l $(TARGET) bin/printInfo bin/MainTDAQStandAlone bin/BLDAQScope 
$(TARGET): obj/MainTDAQServer.o  obj/TDAQServerBase.o obj/EventChecker.o obj/DAQServerInterface.o obj/SOCSimuServerInterface.o obj/SOCServerInterface.o obj/FpgaIOBase.o obj/FpgaInterface.o obj/tcpstream.o obj/tcpacceptor.o
	$(CXX) $(LDFLAGS)   $^ -o $@  
bin/MainTDAQStandAlone: obj/MainTDAQStandAlone.o  obj/TDAQServerStandAlone.o obj/TDAQServerBase.o obj/EventChecker.o obj/DAQServerInterface.o obj/SOCSimuServerInterface.o obj/SOCServerInterface.o obj/FpgaIOBase.o obj/FpgaInterface.o obj/tcpstream.o obj/tcpacceptor.o
	$(CXX) $(LDFLAGS)   $^ -o $@  	
bin/reg: obj/reg.o  obj/FpgaIOBase.o obj/FpgaInterface.o
	$(CXX) $(LDFLAGS)   $^ -o $@  
bin/printInfo: obj/printInfo.o  obj/FpgaIOBase.o obj/FpgaInterface.o
	$(CXX) $(LDFLAGS)   $^ -o $@  
bin/BLDAQScope: obj/BLDAQScope.o obj/FpgaIOBase.o obj/FpgaInterface.o
	$(CXX) $(LDFLAGS)   $^ -o $@  
obj/BLDAQScope.o : src/BLDAQScope.cpp src/FpgaInterface.h src/FpgaIOBase.h src/hps_0.h	
	$(CXX) $(CXXFLAGS) -c $< -o $@
obj/printInfo.o : src/printInfo.cpp src/FpgaInterface.h src/FpgaIOBase.h src/hps_0.h	
	$(CXX) $(CXXFLAGS) -c $< -o $@
obj/reg.o : src/reg.cpp src/FpgaInterface.h src/FpgaIOBase.h src/hps_0.h	
	$(CXX) $(CXXFLAGS) -c $< -o $@
bin/testRegs: obj/testRegs.o  obj/FpgaIOBase.o obj/FpgaInterface.o
	$(CXX) $(LDFLAGS)   $^ -o $@  
obj/testRegs.o : src/testRegs.cpp src/FpgaInterface.h src/FpgaIOBase.h src/hps_0.h	
	$(CXX) $(CXXFLAGS) -c $< -o $@
obj/MainTDAQServer.o : src/MainTDAQServer.cpp src/TDAQServerBase.hh src/EventChecker.hh src/SOCServerInterface.hh src/DAQServerInterface.hh src/FpgaInterface.h src/FpgaIOBase.h src/hps_0.h	
	$(CXX) $(CXXFLAGS) -c $< -o $@
obj/MainTDAQStandAlone.o : src/MainTDAQStandAlone.cpp src/TDAQServerStandAlone.hh src/TDAQServerBase.hh src/EventChecker.hh src/SOCServerInterface.hh src/DAQServerInterface.hh src/FpgaInterface.h src/FpgaIOBase.h src/hps_0.h	
	$(CXX) $(CXXFLAGS) -c $< -o $@	
obj/TDAQServerBase.o : src/TDAQServerBase.cpp src/TDAQServerBase.hh src/EventChecker.hh src/SOCServerInterface.hh src/DAQServerInterface.hh src/FpgaInterface.h src/FpgaIOBase.h src/hps_0.h	
	$(CXX) $(CXXFLAGS) -c $< -o $@
obj/EventChecker.o : src/EventChecker.cpp src/EventChecker.hh
	$(CXX) $(CXXFLAGS) -c $< -o $@
obj/TDAQServerStandAlone.o : src/TDAQServerStandAlone.cpp src/TDAQServerStandAlone.hh src/TDAQServerBase.hh src/SOCServerInterface.hh src/DAQServerInterface.hh src/FpgaInterface.h src/FpgaIOBase.h src/hps_0.h	
	$(CXX) $(CXXFLAGS) -c $< -o $@
obj/tcpstream.o : src/tcpstream.cpp src/tcpstream.h 
	$(CXX) $(CXXFLAGS) -c $< -o $@
obj/tcpacceptor.o : src/tcpacceptor.cpp src/tcpstream.h src/tcpacceptor.h 
	$(CXX) $(CXXFLAGS) -c $< -o $@
obj/SOCSimuServerInterface.o : src/SOCSimuServerInterface.cpp src/SOCServerInterface.hh src/DAQServerInterface.hh src/FpgaInterface.h src/FpgaIOBase.h src/hps_0.h
	$(CXX) $(CXXFLAGS) -c $< -o $@
obj/SOCServerInterface.o : src/SOCServerInterface.cpp src/SOCServerInterface.hh src/DAQServerInterface.hh src/FpgaInterface.h src/FpgaIOBase.h src/hps_0.h
	$(CXX) $(CXXFLAGS) -c $< -o $@
obj/DAQServerInterface.o : src/DAQServerInterface.cpp src/DAQServerInterface.hh
	$(CXX) $(CXXFLAGS) -c $< -o $@
obj/FpgaIOBase.o : src/FpgaIOBase.cpp src/FpgaIOBase.h src/hps_0.h
	$(CXX) $(CXXFLAGS) -c $< -o $@
obj/FpgaInterface.o : src/FpgaInterface.cpp src/FpgaInterface.h src/FpgaIOBase.h src/hps_0.h
	$(CXX) $(CXXFLAGS) -c $< -o $@
obj/%.o : src/%.c src/hps_0.h
	$(CC) $(CFLAGS) -c $< -o $@
src/hps_0.h:
	cd src;	./generate_hps_qsys_header.sh
#obj/Fdaq.o : src/FootTDAQServerFork.cpp src/FpgaInterface.h src/FpgaIOBase.h src/hps_0.h
#	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f bin/$(TARGET) obj/*.a obj/*.o src/*~ 
