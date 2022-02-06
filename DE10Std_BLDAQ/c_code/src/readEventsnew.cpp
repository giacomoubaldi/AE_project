
#include <iostream>
#include <string>
#include <vector>
#include <time.h>
#include <unistd.h>
#include "FpgaInterface.h"

using namespace std;






//function to read important registers of an event

void readEvent(uint32_t & evcount, uint32_t & tgrcount, FpgaInterface& fpga) {
uint32_t reg = 0;
static uint32_t trgcount = 0;
evcount += 1;
std::cout<<"Event: "<< evcount<< std::endl;
std::cout<<"Printing registers:"<<std::endl;

reg = fpga.ReadReg(17);
uint32_t reg1 = (reg >> 25) & 1;     // busy flag
std::cout<<"FSM status signals - Busy Flag: "<<reg1<< std::endl;
uint32_t reg2 = (reg >> 24) & 1;     // busy EB flag
std::cout<<"FSM status signals - Busy EB Flag: "<<reg2<< std::endl;
uint32_t reg3 = (reg >> 23) & 1;     // busy Trigger flag
std::cout<<"FSM status signals - Busy Trigger Flag: "<<reg3<< std::endl;

reg = fpga.ReadReg(19);     // Trigger counter
std::cout<<"Trigger Counter:  "<<reg<< std::endl;
if (reg > trgcount) {trgcount +=1;}

reg = fpga.ReadReg(30);     // EvHeader
std::cout<<"EvHeader: "<<reg<< std::endl;
reg = fpga.ReadReg(31);     // EvTSCCnt
std::cout<<"EvTSCCnt: "<<reg<< std::endl;
reg = fpga.ReadReg(32);     // EvTrgAccCnt
std::cout<<"EvTrgAccCnt: "<<reg<< std::endl;
reg = fpga.ReadReg(33);     // EvTrgRecCnt
std::cout<<"EvTrgRecCnt: "<<reg<< std::endl;
reg = fpga.ReadReg(34);     // EvTrgMSBClkCnt
std::cout<<"EvTrgMSBClkCnt: "<<reg<< std::endl;
reg = fpga.ReadReg(35);     // EvTrgLSBClkCnt
std::cout<<"EvTrgLSBClkCnt: "<<reg<< std::endl;
reg = fpga.ReadReg(36);     // EvDDRPTRmin
std::cout<<"EvDDRPTRmin: "<<reg<< std::endl;
reg = fpga.ReadReg(37);     // EvDDRPTRmax
std::cout<<"EvDDRPTRmax: "<<reg<< std::endl;
reg = fpga.ReadReg(38);     // DDR3PtrTail
std::cout<<"DDR3PtrTail "<<reg<< std::endl;
reg = fpga.ReadReg(39);     // DDR3PtrHead
std::cout<<"DDR3PtrHead: "<<reg<< std::endl;

}






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
  fwver = fpga.ReadReg(16);
  std::cout<<"Firmware version: "<<fwver<<std::endl;



  // check status 
  std::cout<<"Check current status"<<std::endl;
  uint32_t status = fpga.ReadReg(17) & 7;    // bitwise AND
  if( status>7 ){   //se è maggiore di 111 vuol dire che ci sono dei flag accesi
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
    } else  if( status==5){   //state "WaitingEmptyFifo"
	  fpga.cleanUpOFifo();
    } else {
      std::cout<<"waiting for a status change \n"<<std::endl;
	  sleep(1);
    }
    usleep(100);
    status = fpga.ReadReg(17) & 0x7;	
  }


  // here the FPGA FSM is in the CONFIG status
  std::cout<<"-----------------------------------CONFIG STATUS"<<std::endl;



  //fpga.printStatus();
  std::cout<<"Printing all registers"<<std::endl;
  fpga.PrintAllRegs();
  fpga.printStatus();
  fpga.printRAMStatus();

  
  // config the DAQ:
    /*
        Control_Register_BUS(3)(10) = '1' for simulation
        Control_Register_BUS(3)(9)  = '1' for single trigger
        Control_Register_BUS(3)(3-2-1-0) = '1'  for sensorenablein
        Control_Register_BUS(3)(8) = '1' for do store

        ==> Bus(3) = 0111 0000 1111 = x"70f" = 1807 (decimal)
    */

    //uint32_t config;
    fpga.sendWriteReg(3, 0x70f ); 
    std::cout<<"Config reg(3) done "<<std::endl;
  uint32_t preticks = 0x500;
  uint32_t postticks = 0x2000;
  uint32_t waitticks = 0x4000;
  fpga.sendWriteReg( 4, preticks);
  fpga.sendWriteReg( 5, postticks);
  fpga.sendWriteReg( 6, waitticks);
  std::cout<<"Trigger window timing: "<<std::endl
		   <<"         pre-trigger ticks:  "<<preticks<<std::endl
		   <<"         post-trigger ticks: "<<postticks<<std::endl
		   <<"       waiting before erase: "<<waitticks<<std::endl;

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


  std::cout<<"-----------------------------------RUN STATUS"<<std::endl;
  
  uint32_t nev = 50;          //number of events i want to check
  uint32_t evcount = 0;      // counter of events
  uint32_t trgcount = 0;      // counter of triggered events  


  while (evcount <= nev) {
  
    uint32_t EVFifo_empty = (fpga.getOFifoStatus()>>13)&1 ;   //I take the information of emptiness of event fifo: it is the 14^bit, so i apply a shift 
    uint32_t EBfifostatus = fpga.ReadReg(23);
    if (EVFifo_empty ==0 ) {                      //if EVFifo_empty is 0, there is smth in the fifo i want to read out
      std::cout<<"Reading EventFifo..."<<std::endl;
	  std::cout<<"   EB Fifo status:"<<(std::hex)<<EBfifostatus<<endl;
      readEvent(evcount, trgcount, fpga);
      fpga.getOFifoData();          // after I  read the fifo, I say it to let it empty and fill with new data   (turn of rdreq of EventFifo)

    } else   {
        std::cout<<"The EventFifo is empty."<<std::endl;
    }


  } 
  	 
     std::cout<<"--------------------"<<std::endl;
     std::cout<<"Event read: "<< evcount <<std::endl;
     std::cout<<"Triggered Event read: "<< trgcount <<std::endl;

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

  std::cout<<"-----------------------------------CONFIG STATUS"<<std::endl;
  std::cout<<"Printing all registers"<<std::endl;
  fpga.PrintAllRegs();
  fpga.printStatus();
  fpga.printRAMStatus();
  return 0; // OK
}


