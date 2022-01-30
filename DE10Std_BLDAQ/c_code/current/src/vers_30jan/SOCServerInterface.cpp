
#include "SOCServerInterface.hh"
#include <stdio.h>
#include <time.h>
#include <unistd.h>


SOCServerInterface::SOCServerInterface(){
};

SOCServerInterface::~SOCServerInterface(){
};

int SOCServerInterface::initialize(){
  int rc = fpga.openMem();
  if( rc<0 ){
    printf("Closing prog\n");
    return 1; // Not OK
  }
  
  printf("Set LEDs\n");
  // toggle the LEDs a bit
  for(int idx=0; idx<8; idx++){
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

int SOCServerInterface::shutdown(){
  // clean up our memory mapping and exit    
  printf("Got reset; closing \n");
  fpga.closeMem();	
  return 0;
}

void SOCServerInterface::configure(std::vector<uint32_t> & param){
  // array of (regnum & values)
  for( unsigned int i=0; i<param.size(); i+=2){
    sendWriteReg(param[i],param[i+1]);
  }
};

void SOCServerInterface::publish(std::vector<uint32_t> & param,
       std::vector<uint32_t> & results){
  for(unsigned int i=0; i<param.size(); i++){
    results.push_back( fpga.ReadReg(param[i]) );
  }
}

void SOCServerInterface::GoToRun(){
  printf("\n\nGo to run mode\n\n");
  fpga.sendGoTo(ConfigToRun);    // GO TO RUN MODE
}

void SOCServerInterface::StopDC(){
  printf("\n\nGo to config mode\n\n");
  fpga.sendGoTo(RunToConfig);
  printf("\n\nGo to Idle mode\n\n");
  fpga.sendGoTo(ConfigToIdle);
};  


uint32_t SOCServerInterface::bytesAvailable(){
  return (fpga.getOFifoStatus() & 0xffff);
}

void SOCServerInterface::readData(std::vector<uint32_t>& evt, int /*maxValues*/){ // ?? maxValues not used
  fpga.readOfifo(evt); // read a chunk of data (max 2000?) from FIFO	
}


//  private member functions


void SOCServerInterface::sendGoTo(int stat){
  printf("Go to state %d\n",stat);
  fpga.printFifoStatus();					
  fpga.setIFifoData(4);
  fpga.setIFifoData(Header_ChangeState);
  fpga.setIFifoData(stat&3);  
  fpga.setIFifoData(Instruction_Footer);  
  fpga.printFifoStatus();					
}


void SOCServerInterface::sendWriteReg(uint32_t reg, uint32_t value){
  printf("Write reg %d with %08x\n",reg, value);
  fpga.printFifoStatus();					
  fpga.setIFifoData(5);
  fpga.setIFifoData(Header_WriteRegs);
  fpga.setIFifoData(reg);  
  fpga.setIFifoData(value);  
  fpga.setIFifoData(Instruction_Footer);  
  fpga.printFifoStatus();					
}
