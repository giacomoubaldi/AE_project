//
//
#ifndef FPGAIOBASE
#define FPGAIOBASE

#include <fcntl.h>
#include <sys/mman.h>
#include "hwlib.h"
#include "socal/socal.h"
#include "socal/hps.h"
#include "socal/alt_gpio.h"
#include "hps_0.h"

//
// Definitions needed for the FPGA-CPU Bridge
//

#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )
#define IOFIFOSCONTROL_BASE ( IOFIFOSCONTROL_0_BASE )

class FpgaIOBase {

public:
  FpgaIOBase();
  ~FpgaIOBase();
  
  int openMem();  // open memory device (needed to talk to FPGA)
  int closeMem(); // clean up our memory mapping (do it at the end!!)
  
  // getters
  uint32_t getSwitches() {return *sw_p;};
  uint32_t getKeys() {return *key_p;};
  uint32_t getLEDs() {return *led_p;};

  // getters fifo
  uint32_t getIFifoStatus() {return *(iofifo_p+1);};
  uint32_t getOFifoData() {return *(iofifo_p+2);};
  uint32_t getOFifoStatus() {return *(iofifo_p+3);};
  uint32_t getOFifoRegData() {return *(iofifo_p+4);};
  uint32_t getOFifoRegStatus() {return *(iofifo_p+5);};

  // get file descriptor (-1 = error)
  uint32_t getFileDesc() const {return fd;};
  int getVerbosity() const { return verbosity;};

  // setters 
  void setLEDs(uint32_t value) { *led_p = value;};
  void setIFifoData(uint32_t value) { *iofifo_p = value;}
  void setVerbosity(int verb) { verbosity=verb;};
	
  // getters on FIFOs status
  bool OFifoEmpty(){ return (getOFifoStatus()>>16)&1;};
  bool OFifoRegEmpty(){ return (getOFifoRegStatus()>>16)&1;};
  bool FifoEmpty(uint32_t fifostatus){ return (fifostatus>>16)&1;};
  unsigned int OFifoWords(){ return (getOFifoStatus()&0xffff);};
  unsigned int OFifoRegWords(){ return (getOFifoRegStatus()&0xffff);};
  unsigned int IFifoWords(){ return (getIFifoStatus()&0xffff);};
  unsigned int FifoWords(uint32_t fifostatus){ return (fifostatus&0xffff);};

  // other
  void printStatus(); // status of SW, KEYs, FIFO
  void printFifoStatus();  // print only FIFO status
	
protected:
  
  int fd;
  int verbosity;
    
private:
  
  void *virtual_base;		// mapping of the FPGA memory space
  volatile uint32_t * led_p;	// Address of LEDs: write only
  volatile uint32_t * key_p;	// Address of KEYs: read only
  volatile uint32_t * sw_p;	// Address of SWs: read only
  volatile uint32_t * iofifo_p;	// Address of custom FIFOs structure: 6 words: 1 write only, 5 read only

};

#endif
