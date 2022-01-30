
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

  if(argc<4 || (argv[1][0]!='r' && argv[1][0]!='w' ) ){
	std::cout<<argv[0]<<"  read/write registro  [valore]"<<std::endl;
  }
  bool readf= (argv[1][0]=='r' );


  FpgaInterface fpga;
  fpga.setVerbosity(0);

  std::cout<<"Opening memory ....";
  int rc = fpga.openMem();
  if( rc<0 ){
    std::cout<<"error, code="<<rc<<std::endl;
    return 1; // Not OK
  }
  std::cout<<"Opening RAM memory ....";
  rc = fpga.openMem_RAM();
  if( rc<0 ){
    std::cout<<"error, code="<<rc<<std::endl;
    return 1; // Not OK
  }
  
  if( readf ){
	int reg = atoi(argv[2]);
	std::cout<<" Reading reg "<<std::dec<<reg<<std::endl;
	uint32_t regvalue = fpga.ReadReg(reg);	
	std::cout<<"Registro "<<reg<<" valore: "<<regvalue<< "  hex: "<<std::hex<<regvalue<<std::endl;
  } else {
	int reg = atoi(argv[2]);
	uint32_t regvalue = atoi(argv[3]);
	std::cout<<" Writing reg "<<std::dec<<reg<<" with "<<regvalue<<std::endl;
	fpga.sendWriteReg(reg, regvalue);	
	uint32_t regvalue2 = fpga.ReadReg(reg);	
	std::cout<<"Registro "<<reg<<" valore: "<<regvalue2<< "  hex"<<std::hex<<regvalue2<<std::endl;
  }
  return 0; // OK
}