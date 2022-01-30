//
//
//
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include "FpgaInterface.h"


FpgaInterface::FpgaInterface() : FpgaIOBase() {
  for(int i=0; i<32; i++) regs[i]= 0xffffffff; // an invalid initialization number
}

FpgaInterface::~FpgaInterface(){
}

//          COMMANDS

void FpgaInterface::sendReadAllRegs(){
  unsigned int nreg = 27; // number of registers to read
  setIFifoData(nreg+3);   // number of data to transfer
  setIFifoData(Header_ReadRegs);
  for(unsigned int i=0; i<nreg; i++)
    setIFifoData(i);
  setIFifoData(Instruction_Footer);  
}

void FpgaInterface::sendReadReg(uint32_t reg){
  setIFifoData(4);
  setIFifoData(Header_ReadRegs);
  setIFifoData(reg);     // reading out a specific register
  setIFifoData(Instruction_Footer); 
}

void FpgaInterface::sendWriteReg(uint32_t reg, uint32_t value){
  setIFifoData(5);
  setIFifoData(Header_WriteRegs);
  setIFifoData(reg);     // writing register "reg" with 
  setIFifoData(value);   // this "value"
  setIFifoData(Instruction_Footer);    
}

void FpgaInterface::sendGoTo(int stat){
  setIFifoData(4);
  setIFifoData(Header_ChangeState);
  setIFifoData(stat&3);  
  setIFifoData(Instruction_Footer);  
}

// clean up EVENT fifo  (OFifo)
void FpgaInterface::cleanUpOFifo(){
  while( !OFifoEmpty() ){
    int n=OFifoWords();
    for(int idx=0; idx<n; idx++){
      getOFifoData();
    }
    usleep(20);
  }
}

// clean up output REGISTER fifo  (ORegFifo)
void FpgaInterface::cleanUpORegFifo(){
  while( !OFifoRegEmpty() ){
    int n=OFifoRegWords();
    for(int idx=0; idx<n; idx++){
      getOFifoRegData();
    }
    usleep(20);
  }
}

void FpgaInterface::readOfifo(std::vector<uint32_t> & data, int nmax){
  if( nmax>2000) nmax = 2000;                // data are read out in chunks of 2000 ints
  if( verbosity>1 ) printFifoStatus();		
  while( !OFifoEmpty() ){
    int n=getOFifoStatus()&0xffff;
    if( n==0 || nmax <0 ) break;
    if( n>10 ) n=10;
    for(int idx=0; idx<n; idx++){
      uint32_t dat= getOFifoData();
      if( verbosity>0 ) printf("%08x ", dat);			
      nmax--;
      data.push_back(dat);
    }
    if( verbosity>0 ) printf("\n");			
  }
  if( verbosity>0 ) printFifoStatus();					
}

void FpgaInterface::readORegfifo(std::vector<uint32_t> & data){
  int nmax = 2000;
  if( verbosity>1 ) printFifoStatus();		
  usleep(1000);    // just for this moment 1ms delay!
  while( !OFifoRegEmpty() ){
    int n=getOFifoRegStatus()&0xffff;
    if( n==0 || nmax <0 ) break;
    if( n>10 ) n=10;
    for(int idx=0; idx<n; idx++){
      uint32_t dat= getOFifoRegData();
      if( verbosity>0 ) printf("%08x ", dat);			
      nmax--;
      data.push_back(dat);
    }
    if( verbosity>0 ) printf("\n");			
  }
  if( verbosity>0 ) printFifoStatus();					
}

void FpgaInterface::PrintAllRegs(){
	
  std::vector<uint32_t> data;

  printf("\n\nReading all regs\n\n");
  sendReadAllRegs();
  readORegfifo(data);

  if( verbosity>0 ){
    printf("Message: \n");			
    for( unsigned int i=0;i<data.size(); i++){
      printf("%08x ", data[i]);
    }
    printf("\n");			
  }
  
  if(data.size()>20 && data[1]==Header1_regout && data[2]==Header2_regout){
    printf("Size and headers ok\n");			  
  }
  //  int regs= (data.size()-6)/2;
  for( unsigned int i=3;i<data.size()-3; i+=2){
    int rg = data[i];
    int val = data[i+1];
    switch (rg){
    case 0:
      printf(" 0 - Control reg empty: %08x\n",val);
      break;
    case 1:
      printf(" 1 - Control reg last FSM command: %d",val);
      switch (val&3){
      case 0: printf(" IdleToConfig\n"); break;
      case 1: printf(" ConfigToRun\n"); break;
      case 2: printf(" RunToConfig\n"); break;
      case 3: printf(" ConfigToIdle\n"); break;
      default: printf(" UNKNOWN STATE\n"); break;
      };
      break;
    case 2:
      printf(" 2 - Control reg Event Builder Header: %08x\n",val);
      break;
    case 3:
      printf(" 3 - Control reg Busy Model: %d, %s\n",val,(val? "Busy on 1 event": "Busy on fifo almost full"));
      break;
    case 4:
      printf(" 4 - Control reg Duration of ENDOFRUN status: %08x\n",val);
      break;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      printf("%2d - Control reg not used : %08x\n",rg,val);
      break;
    case 16:
      printf("%2d - Monitor reg FIRMWARE VERSION : %08x\n",rg,val);
      break;
    case 17:
      printf("%2d - Monitor reg FSM : %08x  ",rg,val);
      switch (val&0x7){
      case 0: printf(" IDLE"); break;
      case 1: printf(" CONFIG"); break;
      case 2: printf(" PREPAREFORRUN"); break;
      case 3: printf(" RUN"); break;
      case 4: printf(" ENDOFRUN"); break;
      case 5: printf(" WAITINGEMPTYFIFO"); break;
      default: printf(" UNKNOWN STATE"); break;
      };
      if(val&0x10000000) printf(" DAQIsRunning ");
      if(val&0x8000000) printf(" DAQReset ");
      if(val&0x4000000) printf(" DAQConfig ");
      if(val&0x2000000) printf(" DAQReading ");
      printf("\n");
      break;
    case 18:
      printf("%2d - Monitor reg DAQ errors : %08x\n",rg,val);
      break;
    case 19:
      printf("%2d - Monitor reg Trigger Counter : %8d\n",rg,val);
      break;
    case 20:
      printf("%2d - Monitor reg BCOCounter : %08x\n",rg,val);
      break;
    case 21:
      printf("%2d - Monitor reg Clock counter MSB: %08x\n",rg,val);
      break;
    case 22:
      printf("%2d - Monitor reg Clock counter LSB : %08x\n",rg,val);
      break;
    case 23:
      printf("%2d - Monitor reg Event builder FIFO status %08x\n",rg,val);
      break;
    case 24:
      printf("%2d - Monitor reg TX FIFO status %08x\n",rg,val);
      break;
    case 25:
      printf("%2d - Monitor reg RX FIFO status %08x\n",rg,val);
      break;
    case 26:
      printf("%2d - Monitor reg TX Reg FIFO status %08x\n",rg,val);
      break;
    default:
      printf("%2d - UNKNOWN : %08x\n",rg,val);
      break;
    }
  }
}

// high level function to read all registers
void FpgaInterface::ReadAllRegs(){
  std::vector<uint32_t> data;
  cleanUpOFifo();
  sendReadAllRegs();
  usleep(20);
  readORegfifo(data);

  if(data.size()>20 && data[1]==Header1_regout && data[2]==Header2_regout){
    for( unsigned int i=3;i<data.size()-3; i+=2){
      int rg = data[i];
      regs[rg] = data[i+1];
    }
  } else {
    printf("Error reading all registers \n");
  }
  
}

// high level function to read a given register
uint32_t FpgaInterface::ReadReg(uint32_t reg){
  std::vector<uint32_t> data;
  cleanUpORegFifo();
  sendReadReg(reg);
  usleep(20);
  readORegfifo(data);
  if(data.size()>4 && data[1]==Header1_regout && data[2]==Header2_regout && data[3]==(uint32_t)reg){
    regs[reg]= data[4];
  } else {
    printf("Error reading reg %d\n",reg);
    regs[reg]=0xffffffff;
  }
  return regs[reg];
}
