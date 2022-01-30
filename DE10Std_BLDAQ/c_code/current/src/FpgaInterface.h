//
//  Upper level class to deal with FPGA bridges
//
#ifndef FPGAINTERFACE
#define FPGAINTERFACE

#include "FpgaIOBase.h"
#include <vector>

// State values of the FSM
const int IdleToConfig=0;
const int ConfigToRun=1;
const int RunToConfig=2;
const int ConfigToIdle=3;

// Header and footers in the FIFO comunication with FPGA
const uint32_t Header_ChangeState = 0xEADE0080;
const uint32_t Header_ReadRegs = 0xEADE0081;    //-- Read registers from register file --
const uint32_t Header_WriteRegs = 0xEADE0082;   //-- Write registers from register file --
const uint32_t Header_ResetRegs = 0xEADE0083;   //-- Reset Registers of the register file --
const uint32_t Instruction_Footer = 0xF00E0099; //-- Footer for the Ethernet instructions --
const uint32_t Header1_regout = 0xEADE2020;     //-- First header in reg responce --
const uint32_t Header2_regout = 0xEADE2121;     //-- Second header in reg responce --
const uint32_t Header_event = 0xEADEBABA;       //-- First header in event --
const uint32_t Footer_event = 0xBACCA000;       //-- Last word in event --

class FpgaInterface : public FpgaIOBase {

public:

  FpgaInterface();
  ~FpgaInterface();
  
  //
  // commands to deal with FPGA generic DAQ Module;
  // low level FIFO functions are in the base class
  //
	
  // send a "read all registers" command
  void sendReadAllRegs();

  // send a single register read command
  void sendReadReg(uint32_t reg);
	
  // write a sigle register
  void sendWriteReg(uint32_t reg, uint32_t value);

  // ask for a given FSM transition (no check it is allowed!)
  void sendGoTo(int stat);

  // Empty the output fifo
  void cleanUpOFifo();
	
  // Empty the register output fifo
  void cleanUpORegFifo();
	
  // read all data present in the event (output) fifo
  void readOfifo(std::vector<uint32_t> & data, int nmax=2000);
	
  // real all data in the output register fifo
  void readORegfifo(std::vector<uint32_t> & data);
	
  // read a single register (send a read command and return its output)
  uint32_t ReadReg(uint32_t reg);
  
  // read all registers (send a read command and return its output)
  void ReadAllRegs();
	
  // read all registers and print them!
  void PrintAllRegs();
	
private:
  
  // a local copy of the FPGA registers	
  // they are updated at each known change
  uint32_t regs[32]; 
};

#endif
