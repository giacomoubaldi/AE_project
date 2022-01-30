
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
  fpga.printRAMStatus();
  std::cout<<"System ID: "<<(std::hex)<<fpga.getSysID()<<std::endl;
  std::cout<<"Firmware version : "<<(std::hex)<<fpga.getFirmwareVersion()<<std::endl;
  std::cout<<"Switches position: "<<(std::hex)<<fpga.getSwitches()<<std::endl;
  std::cout<<"Keys position    : "<<(std::hex)<<fpga.getKeys()<<std::endl;
  
  fpga.printStatus();  
  fpga.printFifoStatus();
  fpga.printRAMStatus();
  
  std::cout<<"Check firmware "<<std::endl;
  uint32_t fwver = fpga.ReadReg(16);
  std::cout<<"Firmware version: "<<fwver<<std::endl;

  fpga.printFifoStatus();
  std::cout<<"Printing all registers"<<std::endl;
  fpga.PrintAllRegs();
  fpga.printStatus();
  fpga.printRAMStatus();
  std::cout<<std::endl<<"RAM Offset 1:  "<<fpga.getHPSofs()<<std::endl;
  std::cout<<           "RAM Offset 2:  "<<fpga.getFPGAofs()<<std::endl;
  return 0; // OK
}