
#include "SOCServerInterface.hh"
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <time.h>
#include <unistd.h>


SOCServerInterface::SOCServerInterface(unsigned int verbose) : m_verbose(verbose){
  m_isRunning=false;
  m_nextEventNumber = 0;
};

SOCServerInterface::~SOCServerInterface(){
};

int SOCServerInterface::initialize(){
  int rc = fpga.openMem();
  if( rc<0 ){
    std::cout<<"Closing prog"<<std::endl;
    return 1; // Not OK
  }
  rc = fpga.openMem_RAM();
  if( rc<0 ){
    std::cout<<"Closing prog"<<std::endl;
    return 1; // Not OK
  }
  
  if( m_verbose )
    std::cout<<"Set LEDs"<<std::endl;
  // toggle the LEDs a bit
  for(int idx=0; idx<8; idx++){
    fpga.setLEDs(1<<idx);
    usleep(10*1000);
  }
  fpga.setLEDs(0);
  
  if( m_verbose )
    std::cout<<"Cleaning output fifo"<<std::endl;
  fpga.cleanUpRAM();
  fpga.cleanUpOFifo();
  fpga.cleanUpORegFifo();
  fpga.cleanUpRAM();
  fpga.cleanUpOFifo();
  fpga.cleanUpORegFifo();

  if( m_verbose )
    std::cout<<"\n\nCheck firmware "<<std::endl;
  uint32_t fwver = fpga.ReadReg(16);
  std::cout<<"Firmware version: "<<(std::hex)<<fwver<<(std::dec)<<std::endl;
    // check status and go to CONFIG
  if( m_verbose )
    std::cout<<"Check current status"<<std::endl;
  uint32_t status = fpga.ReadReg(17) & 7;
  if( status>7 ){
    if( m_verbose )
      std::cout<<"Error reading register 16!!"<<std::endl;
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
      fpga.cleanUpRAM();
      fpga.cleanUpOFifo();
    } else {
      if( m_verbose )
	    std::cout<<"waiting for a status change "<<std::endl;
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
  if( m_verbose )
    std::cout<<"Got reset; closing "<<std::endl;
  fpga.closeMem();	
  return 0;
}

void SOCServerInterface::configure(std::vector<uint32_t> & param){
  if( m_verbose ){
	std::cout<<"Received CONFIG with "<<(std::dec)<<param.size()<<" parameters:"<<std::endl;  
  }
  // array of (regnum & values)
  for( unsigned int i=0; i<param.size(); i+=2){
//	if( m_verbose ){
   	  std::cout<<"Writing register "<<param[i]<<", value "<<(std::hex)<< param[i+1]<<(std::dec)<<std::endl;  		
//	}  
    sendWriteReg(param[i],param[i+1]);
  }

  uint32_t EB = fpga.ReadReg(0x2);
  std::cout<<"Event header: "<<(std::hex)<<EB<<(std::dec)<<std::endl;  		
   

  // Go to config state
  if( m_verbose ){
   	std::cout<<"Going to CONFIG State....."<<std::endl;  		
  }  
  uint32_t status = fpga.ReadReg(17) & 7;
  uint32_t tries = 0;
  while( status!=1 && tries<10){
    if( status==0){
	  fpga.sendGoTo(IdleToConfig);
    } else if( status==3){
	  fpga.sendGoTo(RunToConfig);
    } else if( status==5){
      fpga.cleanUpRAM();
//MV  fpga.cleanUpOFifo();
    } else {
      if( m_verbose )
	    std::cout<<"waiting for a status change "<<std::endl;
	  sleep(1);
    }
    usleep(100);
	tries++;
    status = fpga.ReadReg(17) & 0x7;		
  }
  if( m_verbose ) {
	if( status==1 ){
   	  std::cout<<"CONFIG State reached!"<<std::endl;  		
    } else {
   	  std::cout<<"state NOT reached!"<<std::endl;  			  
    }
  }
};

void SOCServerInterface::publish(std::vector<uint32_t> & param,
       std::vector<uint32_t> & results){
  if( m_verbose ){
	  std::cout<<"Received PUBLISH with "<<(std::dec)<<param.size()<<" parameters:"<<std::endl;  
  }  
  
  //
  if( ((fpga.getIFifoStatus()>>19)&1) ==1 ){
	  std::cout<<" Input fifo error! Status: "<<(std::hex)<<fpga.getIFifoStatus()<<(std::dec)<<std::endl; 
  }
  //
  for(unsigned int i=0; i<param.size(); i++){
    uint32_t val = fpga.ReadReg(param[i]);
	if( m_verbose ){
   	  std::cout<<"Read register "<<param[i]<<", value "<<(std::hex)<<val<<std::endl;  		
	}  
    results.push_back( val );
	if( param[i]==0 && val==0xfffffff ){
   	  std::cout<<"Error reading firmware register "<<std::endl;  		
	  fpga.printFifoStatus();
	  fpga.cleanUpORegFifo();
	  fpga.printFifoStatus();
	}	
  }
}

void SOCServerInterface::GoToRun(uint32_t /*runNumber*/, std::vector<uint32_t> & /*param*/){
  fpga.cleanUpRAM();
  fpga.cleanUpOFifo();
  fpga.cleanUpORegFifo();
  m_nextEventNumber = 0;
  
  if( m_verbose )
    std::cout<<"\n\nGo to run mode\n"<<std::endl;
  fpga.sendGoTo(ConfigToRun);    // GO TO RUN MODE
}

void SOCServerInterface::StopDC(){
  if( m_verbose )
    std::cout<<"\n\nGo to config mode\n"<<std::endl;
  fpga.sendGoTo(RunToConfig);
//  if( m_verbose )
//    std::cout<<"\n\nGo to Idle mode\n"<<std::endl;
//  fpga.sendGoTo(ConfigToIdle);
	
  std::vector<uint32_t> evt;  
  while( wordsAvailable()>0 ){
	fpga.readOfifo_on_RAM(evt);
//MV	fpga.readOfifo(evt);
	evt.clear();
  }
  fpga.cleanUpRAM();
};  


uint32_t SOCServerInterface::wordsAvailable(){
//MV  return (fpga.getOFifoStatus() & 0xffff);
  return fpga.wordsAvailable_on_RAM();
}

void SOCServerInterface::readData(std::vector<uint32_t>& evt, int maxValues){ // ?? maxValues not used
  fpga.readOfifo_on_RAM_fast2(evt, maxValues); // read a chunk of data (max 2000?) from FIFO	
//MV  fpga.readOfifo(evt); // read a chunk of data (max 2000?) from FIFO	
}


//  private member functions


void SOCServerInterface::sendGoTo(int stat){
  if( m_verbose ){
    std::cout<<"Go to state "<<stat<<std::endl;
    fpga.printFifoStatus();					
  }
  fpga.setIFifoData(4);
  fpga.setIFifoData(Header_ChangeState);
  fpga.setIFifoData(stat&3);  
  fpga.setIFifoData(Instruction_Footer);  
  if( m_verbose )
    fpga.printFifoStatus();					
}


void SOCServerInterface::sendWriteReg(uint32_t reg, uint32_t value){
  if( m_verbose ){
    std::cout<<"Write reg "<<reg<<" with "<<(std::hex)<< value<<std::endl;
    fpga.printFifoStatus();					
  }
  if( fpga.getIFifoStatus() != 0x10000 ){
    std::cout<<"Error: sending a command to a not-empty input fifo. Write reg "<<reg<<" with "<<(std::hex)<< value<<std::endl;
    fpga.printFifoStatus();					
  }
  
  fpga.setIFifoData(5);
  fpga.setIFifoData(Header_WriteRegs);
  fpga.setIFifoData(reg);  
  fpga.setIFifoData(value);  
  fpga.setIFifoData(Instruction_Footer);  
  if( m_verbose )
    fpga.printFifoStatus();					
}
