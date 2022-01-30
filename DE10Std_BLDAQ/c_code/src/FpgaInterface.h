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

struct EVHeader {
  uint32_t header;
  uint32_t timeStamp;
  uint32_t acceptedTriggers;
  uint32_t receivedTriggers;
  uint32_t MSBClockCounter;
  uint32_t LSBClockCounter;
  uint32_t DDR3PointerTail;
  uint32_t DDR3PointerHead;
};

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
  
  EVHeader & getCurrentEvent() { return currEvent;};
  EVHeader & readCurrentEvent();

  // Empty the ram
  void cleanUpRAM();
  
  // read all data present in the event (output) fifo
  void readOfifo(std::vector<uint32_t> & data, int nmax=2000);
	
  // get the numbers of words available on RAM
  uint32_t wordsAvailable_on_RAM();
	
  // read all data present in the event (output) fifo fast via shared RAM buffer-------------------------------------------------------------------------------
  void readOfifo_on_RAM(std::vector<uint32_t> & data, uint32_t nmax=2000);
  void readOfifo_on_RAM_fast(std::vector<uint32_t> & data, uint32_t nmax=2000);
  void readOfifo_on_RAM_fast2(std::vector<uint32_t> & data, uint32_t nmax=2000);
  void readOfifo_on_RAM_fast3(uint32_t tail, uint32_t head, std::vector<uint32_t> & data, uint32_t nmax=2000);

  // real all data in the output register fifo
  void readORegfifo(std::vector<uint32_t> & data);
	
  // read a single register (send a read command and return its output)
  uint32_t ReadReg(uint32_t reg);
  
  // read all registers (send a read command and return its output)
  void ReadAllRegs();
	
  // read all registers and print them!
  void PrintAllRegs();
  
  // read all registers and print them!
  void printMonitorRegs();

  // read all registers and print them!
  void printConfigRegs();

  // read a specified channel of the ADC-------------------------------------------------------------------------------------------------------
  uint16_t read_ADC_channel(uint8_t ch_index);
	
  // getters for RAM handling
  uint32_t getHPSofs() const { return hps_offset_RAM;};
  uint32_t getFPGAofs() const { return fpga_offset_RAM;};
	
private:
  
  // a local copy of the FPGA registers	
  // they are updated at each known change
  uint32_t regs[48]; 

  // check and wait for the input FIFO to be ready to get new commands.
  void checkIFifo();

  EVHeader currEvent;
 
public:
  // RAM management variables----------------------┐
  uint32_t num_words_read = 0;
  uint32_t hps_offset_RAM = 0;
  uint32_t hps_wrapped_around = 0;
  volatile uint32_t fpga_offset_RAM = 0;
  volatile uint32_t fpga_wrapped_around = 0;
  bool p_in_different_page = false;
  //-----------------------------------------------┘
};

#endif
