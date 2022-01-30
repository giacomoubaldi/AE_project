
#include <iostream>
#include <string>
#include <vector>
#include <time.h>
#include <unistd.h>
#include <iomanip>
#include <cstdio>
#include "FPGAInterface.h"

using namespace std;

int getOption(int def, int max){
  int rc=0;
  bool done = false;
  while( !done ){
    cout<<" Select ["<<def<<"] ?";
    std::string s;
    cin>>s;
    if( s.size()==0 ){
	  rc = def;
	  done = true;
    } else {
      if(s[0]<'0' or s[0]>'0'+max ) done = false;
	  else {
		rc = s[0]-'0';
		done = true;
	  }		  
    }
	cout<<endl;
  }
  return rc;
}
  

void writeConfig(FpgaInterface & fpga){
  int reg=-10;
  while(reg<-1 || reg>15){
    std::cout<<std::endl<<"  Reg to change [0-15] (-1 to exit)? ";
	cin>>reg;
  }
  cout<<"..."<<reg<<std::endl;
  if( reg>=0 && reg<=15){
    int value=0;
    std::cout<<std::endl<<"  Value ? ";
	cin>>value;
	std::cout<<std::endl<<"  Setting config reg "<<reg
		<<" with "<<(std::dec)<<value<<"   0x"<<(std::hex)<<value<<std::endl;
    fpga.sendWriteReg(reg,value);
  }
}
void readEvents(FpgaInterface & fpga, int nevents){
  int n=0;
  while(n<nevents){
    uint32_t evstat = fpga.getOFifoStatus();
	std::cout<<"Event fifo status: "<<(std::hex)<<evstat<<(std::hex)<< "  "
	         <<((evstat&0x2000)? " Empty" : " ")<<((evstat&0x4000)? " Almost full " : " ")
	         <<((evstat&0x8000)? " Full" : " ")<<"."<<std::endl;
	if( (evstat&0x2000)==0 ){ // not full; events are present
      std::cout<<"DDR3 pointers: Tail="<<(std::hex)<<fpga.ReadReg(38)
	           <<              " Head="<<(std::hex)<<fpga.ReadReg(39)<<std::endl;
      EVHeader header = fpga.readCurrentEvent();
      std::cout<<(std::hex)<<"Header : "<<std::setw(8)<<header.header<<std::endl;
	  std::cout<<(std::hex)<<"Time stamp : "<<std::setw(8)<<header.timeStamp<<std::endl;
      std::cout<<(std::dec)<<"Triggers. Accepted: "<<header.acceptedTriggers
	                       <<"  Received: "<<header.receivedTriggers<<std::endl;
      std::cout<<(std::hex)<<"Clock ticks "<<std::setw(8)<<header.MSBClockCounter
	                                       <<std::setw(4)<<header.LSBClockCounter
	                       <<std::endl;
      std::cout<<(std::hex)<<"DDR3 Pointers. Tail: "
	           <<std::setw(8)<<header.DDR3PointerTail
			   <<"  Head: "<<std::setw(8)<<header.DDR3PointerHead
			   <<std::endl;
	  std::vector<uint32_t> data;
	  fpga.readOfifo_on_RAM_fast3(header.DDR3PointerTail,header.DDR3PointerHead,data);
	  std::cout<<"Event size: "<<(std::dec)<<data.size()<<(std::hex)<<std::endl;
      for(unsigned i=0; i<data.size(); i++){
		std::cout<<std::setw(8)<<data[i]<<" ";
		if( ((i+1) % 10)==0 ) std::cout<<std::endl;
	  }
	  uint32_t ofd = fpga.getOFifoData();
	  std::cout<<"Read acknowledge return: "<<(std::hex)<<ofd<<std::endl;
	  n++;
	} else {
	  usleep(100000);
	}
  }	  
}


void changeState(FpgaInterface & fpga){
 // here a start on loops ...
  bool done = false;
  while(!done ){
    std::cout<<std::endl<<" Change State Menu "<<std::endl<<std::endl;
    int val = fpga.ReadReg(17); // config reg
    std::cout<<"Current status: ";
    switch (val&0x7){
    case 0: std::cout<<" IDLE"; break;
    case 1: std::cout<<" CONFIG"; break;
    case 2: std::cout<<" PREPAREFORRUN"; break;
    case 3: std::cout<<" RUN"; break;
    case 4: std::cout<<" ENDOFRUN"; break;
    case 5: std::cout<<" WAITINGEMPTYFIFO"; break;
    default: std::cout<<" UNKNOWN STATE"; break;
    };
    if(val&0x10000000) std::cout<<" DAQIsRunning ";
    if(val&0x8000000) std::cout<<" DAQReset ";
    if(val&0x4000000) std::cout<<" DAQConfig ";
    if(val&0x2000000) std::cout<<" BUSY FLAG ";
    if(val&0x1000000) std::cout<<" BUSY_EB ";
    if(val&0x800000) std::cout<<" BUSY_TRG ";
    std::cout<<std::endl<<std::endl;
    
    std::cout<<"[1] Idle To Config"<<std::endl; 
    std::cout<<"[2] Config To Run"<<std::endl; 
    std::cout<<"[3] Run To Config"<<std::endl; 
    std::cout<<"[4] Config To Idle"<<std::endl; 
    std::cout<<"[5] Read again the status"<<std::endl; 
    std::cout<<""<<std::endl; 
    std::cout<<"[0] Exit "<<std::endl; 
	
	val = getOption(0,5);
    std::cout<<"option read out: "<<val<<std::endl; 
	
	if(val>0 && val<5){
	  fpga.sendGoTo(val-1);
      std::cout<<" to reg # 1 value :"<<val-1<<std::endl; 
	  usleep(50000);
	}
	if(val==0) done=true;
  }
}

// main function calling the thread for slow connection:
// it will call the fast connection 
int main( int argc, char *argv[] ){


  FpgaInterface fpga;
  fpga.setVerbosity(0);

  int rc = fpga.openMem();
  if( rc<0 ){
  std::cout<<"Opening memory ....";
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
  fpga.printFifoStatus();
  
  std::cout<<"Check firmware "<<std::endl;
  uint32_t fwver = fpga.ReadReg(16);
  std::cout<<"Firmware version: "<<fwver<<std::endl;

  // here a start on loops ...
  bool done = false;
  while(!done ){
	  std::cout<<std::endl<<"  --- Main Menu' ---"<<std::endl<<std::endl;
      std::cout<<"[1] Print monitoring regs"<<std::endl; 
      std::cout<<"[2] Print config regs"<<std::endl; 
      std::cout<<"[3] Write config reg"<<std::endl; 
      std::cout<<"[4] Change state"<<std::endl; 
      std::cout<<"[5] Read one event"<<std::endl; 
      std::cout<<"[6] Read up to 10 events"<<std::endl; 
      std::cout<<"[7] Read up to 1000 events"<<std::endl; 
      std::cout<<""<<std::endl; 
      std::cout<<"[0] Exit "<<std::endl; 
      int val=getOption(0,7);
	  
	  if( val==0 ){
		  done=true;
	  }
	  else if( val>0 && val<8 ){
		switch(val){
		case 1:
		  fpga.printMonitorRegs();
		  break;
		case 2:
		  fpga.printConfigRegs();
		  break;
		case 3:
		  writeConfig(fpga);
		  break;
		case 4:
		  changeState(fpga);
		  break;
		case 5:
		  readEvents(fpga,1);
		  break;
		case 6:
		  readEvents(fpga,10);
		  break;
		case 7:
		  readEvents(fpga,1000);
		  break;
		}		  
	  }
  }
  return 0;
}