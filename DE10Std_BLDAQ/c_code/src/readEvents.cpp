
#include <iostream>
#include <string>
#include <vector>
#include <time.h>
#include <unistd.h>
#include "FPGAInterface.h"

using namespace std;

// main function calling the thread for slow connection:
// it will call the fast connection 
int main( int argc, char *argv[] ){


  FpgaInterface fpga;
  fpga.setVerbosity(0);

  std::cout<<"Opening memory ....";
  int rc = fpga.openMem();
  if( rc<0 ){
    std::cout<<"error, code="<<rc<<std::endl;
    return 1; // Not OK
  }
  rc = fpga.openMem_RAM();
  if( rc<0 ){
    std::cout<<"error, code="<<rc<<std::endl;
    return 1; // Not OK
  }
  std::cout<<"System ID: "<<(std::hex)<<fpga.getSysID()<<std::endl;
  std::cout<<"Firmware version : "<<(std::hex)<<fpga.getFirmwareVersion()<<std::endl;
  std::cout<<"Switches position: "<<(std::hex)<<fpga.getSwitches()<<std::endl;
  std::cout<<"Keys position    : "<<(std::hex)<<fpga.getKeys()<<std::endl;
  
  fpga.printStatus();
  std::cout<<"Check firmware "<<std::endl;
  uint32_t fwver = fpga.ReadReg(16);
  std::cout<<"Firmware version: "<<fwver<<std::endl;
  std::cout<<"Printing all registers"<<std::endl;
  fpga.PrintAllRegs();
  fpga.printStatus();
  fpga.printRAMStatus();
  std::cout<<std::endl<<"RAM Offset 1:  "<<fpga.getHPSofs()<<std::endl;
  std::cout<<           "RAM Offset 2:  "<<fpga.getFPGAofs()<<std::endl;
  
  
  
  std::cout<<"Set LEDs\n"<<std::endl;
  // toggle the LEDs a bit
  for(int idx=0; idx<8; idx++){
    fpga.setLEDs(1<<idx);
    usleep(100*1000);
  }
  fpga.setLEDs(0);
  
  std::cout<<"Cleaning output fifo"<<std::endl;
  fpga.cleanUpOFifo();
  fpga.cleanUpORegFifo();
  fpga.cleanUpOFifo();
  fpga.cleanUpORegFifo();
  fpga.printStatus();
  std::cout<<"Check firmware "<<std::endl;
  uint32_t fwver = fpga.ReadReg(16);
  std::cout<<"Firmware version: "<<fwver<<std::endl;



  // check status 
  std::cout<<"Check current status"<<std::endl;
  uint32_t status = fpga.ReadReg(17) & 7;    // bitwise AND
  if( status>7 ){
    std::cout<<"Error reading register 16!!\n"<<std::endl;
    fpga.PrintAllRegs();
    fpga.closeMem();	
    return 1;
  }


  //-----CONFIG
  //----------------------------

  //go to CONFIG
  while( status!=1){    //if status is not config
    if( status==0){
	  fpga.sendGoTo(IdleToConfig);
    } else if( status==3){
	  fpga.sendGoTo(RunToConfig);
    } else  if( status==5){
	  fpga.cleanUpOFifo();
    } else {
      std::cout<<"waiting for a status change \n"<<std::endl;
	  sleep(1);
    }
    usleep(100);
    status = fpga.ReadReg(17) & 0x7;	
  }


  // here the FPGA FSM is in the CONFIG status
  fpga.printStatus();
  std::cout<<"Printing all registers"<<std::endl;
  fpga.PrintAllRegs();
  fpga.printStatus();
  fpga.printRAMStatus();
  std::cout<<std::endl<<"RAM Offset 1:  "<<fpga.getHPSofs()<<std::endl;
  std::cout<<           "RAM Offset 2:  "<<fpga.getFPGAofs()<<std::endl;


  // Read ADC values
  std::cout<<std::endl<<"ADC values "<<std::endl;
  for(unsigned int chan=0; chan<8; chan++){
    std::cout<<"Channel "<<chan<<", value="<<fpga.read_ADC_channel(chan)<<std::endl;
  }
  std::cout<<"------------ "<<std::endl;
	



  //-----RUN
  //----------------------------
  	
  // now going to run mode	
  status = fpga.ReadReg(17) & 7;
  std::cout<<"Current status "<<status<<std::endl;
  if( status>7 ){
    std::cout<<"Error reading register 16!!\n"<<std::endl;
    fpga.PrintAllRegs();
    fpga.closeMem();	
    return 3;
  }
  
  while( status!=3){    //if status is not run
    if( status==0){
	  fpga.sendGoTo(IdleToConfig);
    } else if( status==1){
	  fpga.sendGoTo(ConfigToRun);
    } else  if( status==5){
	  fpga.cleanUpRAM();
    } else {
      std::cout<<"waiting for a status change \n"<<std::endl;
	  sleep(1);
    }
    usleep(100);
    status = fpga.ReadReg(17) & 0x7;	
    std::cout<<"Current status "<<status<<std::endl;
  }
// here the FPGA FSM is in the RUN status

  if( status==3 ) std::cout<<std::endl<<" RUN state reached "<<std::endl;
  std::cout<<"Printing all registers"<<std::endl;
  fpga.PrintAllRegs();
  fpga.printStatus();
  fpga.printRAMStatus(); 
  std::cout<<std::endl<<"RAM Offset 1:  "<<fpga.getHPSofs()<<std::endl;
  std::cout<<           "RAM Offset 2:  "<<fpga.getFPGAofs()<<std::endl;
  std::cout<<"waiting 10 seconds for triggers \n"<<std::endl;
  sleep(10);

  std::cout<<"Printing all registers"<<std::endl;
  fpga.PrintAllRegs();
  fpga.printStatus();
  fpga.printRAMStatus(); 
  	




  //-----go back to CONFIG MODE
  //----------------------------
  
  // shut down
  std::cout<<"Shutting down"<<std::endl;
  status = fpga.ReadReg(17) & 7;
	
  while( status!=1){
    if( status==0){
	  fpga.sendGoTo(IdleToConfig);
    } else if( status==3){
	  fpga.sendGoTo(RunToConfig);
    } else  if( status==5){
	  fpga.cleanUpRAM();
    } else {
      std::cout<<"waiting for a status change \n"<<std::endl;
	  sleep(1);
    }
    usleep(100);
    status = fpga.ReadReg(17) & 0x7;	
  }
  std::cout<<"Printing all registers"<<std::endl;
  fpga.PrintAllRegs();
  fpga.printStatus();
  fpga.printRAMStatus();
  return 0; // OK
}