#ifndef SERVINT_HH
#define SERVINT_HH

#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"
#include "hps_0.h"
#include "FpgaInterface.h"


class ServerInterface {
public:
  ServerInterface(){};
  ~ServerInterface(){};

  int initialize(){ return 0;};
  int shutdown(){return 0;};

  void configure(std::vector<int> & param){};

  void publish(std::vector<int> & param,
	       std::vector<int> & results){};
  void GoToRun(){};
  void StopDC(){};

private:

};



const int IdleToConfig=0;
const int ConfigToRun=1;
const int RunToConfig=2;
const int ConfigToIdle=3;

const int Header_ChangeState = 0xEADE0080;
//-- Read registers from register file --
const int Header_ReadRegs = 0xEADE0081;
//-- Write registers from register file --
const int Header_WriteRegs = 0xEADE0082;
//-- Reset Registers of the register file --
const int Header_ResetRegs = 0xEADE0083;
//-- Footer for the Ethernet instructons --
const int Instructon_Footer = 0xF00E0099; 


class SOCInterface : public ServerInterface {
public:
  SOCInterface(){};
  ~SOCInterface(){};

  int initialize(){
    int rc = fpga.openMem();
    if( rc<0 ){
      printf("Closing prog\n");
      return 1; // Not OK
    }
    
    printf("Set LEDs\n");
    // toggle the LEDs a bit
    for(idx=0; idx<8; idx++){
      fpga.setLEDs(1<<idx);
      usleep(50*1000);
    }
    fpga.setLEDs(0);
    
    printf(" Cleaning output fifo\n");
    fpga.cleanUpOFifo();
    fpga.cleanUpORegFifo();
    fpga.cleanUpOFifo();
    fpga.cleanUpORegFifo();

    printf("\n\n Check firmware \n");
    uint32_t fwver = fpga.ReadReg(16);
    printf("Firmware version: %x\n", fwver);
      // check status and go to CONFIG
    printf("\n\n Check current status\n");
    uint32_t status = fpga.ReadReg(17) & 7;
    if( status>7 ){
      printf("Error reading register 16!!\n");
      fpga.PrintAllRegs();
      fpga.closeMem();	
      return 1;
    }
    
    while( status!=1){
      if( status==0){
	fpga.sendGoTo(IdleToConfig);
      } else if( status==3){
	fpga.sendGoTo(RunToConfig);
      } else  if( status==5){
	fpga.cleanUpOFifo();
      } else {
	printf("waiting for a status change \n");
	sleep(1);
      }
      usleep(100);
      status = fpga.ReadReg(17) & 0x7;	
    }
    // here the FPGA FSM is in the CONFIG status
      
    return 0; // OK
  };

  int shutdown(){
    // clean up our memory mapping and exit    
    printf("Got reset; closing \n");
    fpga.closeMem();	
    return 0;
  }

  void configure(std::vector<int> & param){
    // array of (regnum & values)
    for( int i=0; i<param.size(); i+=2}{
      sendWriteReg(param[i],param[i+1]);
    }
  };
  
  void publish(std::vector<int> & param,
	       std::vector<int> & results){
    for(int i=0; i<param.size(); i++){
      results.push_back( fpga.ReadReg(param[i]) );
    }
  }

  void GoToRun(){
    printf("\n\nGo to run mode\n\n");
    fpga.sendGoTo(ConfigToRun);    // GO TO RUN MODE
  }

  void StopDC(){
    printf("\n\nGo to config mode\n\n");
    fpga.sendGoTo(RunToConfig);
    printf("\n\nGo to Idle mode\n\n");
    fpga.sendGoTo(ConfigToIdle);
  };  

private:
  

  void sendGoTo(int stat){
    printf("Go to state %d\n",stat);
    fpga.printFifoStatus();					
    fpga.setIFifoData(4);
    fpga.setIFifoData(Header_ChangeState);
    fpga.setIFifoData(stat&3);  
    fpga.setIFifoData(Instructon_Footer);  
    fpga.printFifoStatus();					
  }


  void sendWriteReg(int reg, int value){
    printf("Write reg %d with %08x\n",reg, value);
    fpga.printFifoStatus();					
    fpga.setIFifoData(5);
    fpga.setIFifoData(Header_WriteRegs);
    fpga.setIFifoData(reg);  
    fpga.setIFifoData(value);  
    fpga.setIFifoData(Instructon_Footer);  
    fpga.printFifoStatus();					
  }

   FpgaInterface fpga;

};

#endif
