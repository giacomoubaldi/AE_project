//
//
#ifndef FPGAIOBASE
#define FPGAIOBASE

#include <fcntl.h>
#include <sys/mman.h>
#define soc_cv_av
#include "hwlib.h"
//#include "socal/socal.h"
//#include "socal/hps.h"
//#include "socal/alt_gpio.h"
//#include "hps.h"
#include "hps_0.h"

//
// Definitions needed for the FPGA-CPU Bridge
//
#define ALT_STM_OFST 0xfc000000
#define ALT_LWFPGASLVS_OFST 0xff200000LL


//#define ALT_STM_OFST ALT_CS_STM_OFST 
//#define ALT_LWFPGASLVS_OFST ALT_FPGA_BRIDGE_LWH2F_OFST

#define HW_REGS_BASE ( ALT_STM_OFST )
#define HW_REGS_SPAN ( 0x04000000 )
#define HW_REGS_MASK ( HW_REGS_SPAN - 1 )
#define IOFIFOSCONTROL_BASE ( IOFIFOSCONTROL_0_BASE )

//------------------------------------------------------------------------------------------------------------------------------------┐
// These two parameters MUST agree with the configuration of the device tree and FPGA
#define FPGA_reserved_memory_BASE_ADDRESS 0x00C00000 //I reserved 16 MiB for the FPGA, from 0x00C0_0000 to 0x01BF_FFFF
#define FPGA_reserved_memory_AMOUNT 0x01000000 // that is 0x100_0000 == 16 MiB
#define FPGA_reserved_memory_MAX_OFFSET (FPGA_reserved_memory_AMOUNT/sizeof(uint32_t) - 1)
// RAM pointers register location
#define HPS_GPIO_BASE_ADDRESS 0xFF706000
#define HPS_GPO_OFFSET 0x10// Register for the general purpose output of hps
#define HPS_GPI_OFFSET 0x14// Register for the general purpose input of hps
// Bridges resets and calibration flag
#define SDRREGS_BASE 0xFFC20000// SDRAM controller subsystem registers
#define SDRREGS_LENGTH 131072// (128 KiB)
#define FPGAPORTRST_OFFSET 0x5080// This register should be written to with a 1 to enable the selected FPGA port to exit reset
#define DRAMSTS_OFFSET 0x5038// DRAM Status Register: this register provides the status of the calibration and ECC logic
//------------------------------------------------------------------------------------------------------------------------------------┘

class FpgaIOBase {

public:
  FpgaIOBase();
  ~FpgaIOBase();
  
  int openMem();  // open memory device (needed to talk to FPGA)
  int closeMem(); // clean up our memory mapping (do it at the end!!)
  //----------------------------------------------------------------------------------------┐
  int openMem_RAM();// open memory device (needed to access reserved memory to FPGA)
  int closeMem_RAM();// clean up our memory mapping (do it at the end!!)
  //----------------------------------------------------------------------------------------┘
  
  // getters
  uint32_t getSysID() {return *sysid_p;};
  uint32_t getFirmwareVersion() {return *fwvers_p;};
  uint32_t getSwitches() {return *sw_p;};
  uint32_t getKeys() {return *key_p;};
  uint32_t getLEDs() {return *led_p;};

  // getters fifo
  uint32_t getIFifoStatus() {return *(iofifo_p+1);};
  uint32_t getOFifoData() {return *(iofifo_p+2);};
  uint32_t getOFifoStatus() {return *(iofifo_p+3);};
  uint32_t getOFifoRegData() {return *(iofifo_p+4);};
  uint32_t getOFifoRegStatus() {return *(iofifo_p+5);};

  //-----------------------------------------------------------------------------------------------------------┐
  // Management of RAM DDR3
  inline uint32_t Read_32bit_RAM(uint32_t addr_offset){
                    return  *( (uint32_t*)( ram_32bit_p + addr_offset ) ); 
	              }
  
  uint8_t Read_byte_RAM(uint32_t addr_offset);
  void Write_32bit_RAM(uint32_t data, uint32_t addr_offset);
  
  // Initialize RAM
  void init_RAM(uint32_t data = 0);

  // Write and read pointers from and to the GPIO of HPS
  void Write_HPS_RAM_control(uint32_t addr_offset, uint32_t wrapped_around);
  void Read_FPGA_RAM_control(volatile uint32_t* addr_offset, volatile uint32_t* wrapped_around);
  //-----------------------------------------------------------------------------------------------------------┘

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
  void printRAMStatus(); // status of RAM Block
  
protected:
  
  int fd;
  int fd_ram;//--------------------------------------------------------------------------------
  int verbosity;
    
private:
  
  void *virtual_base;// mapping of the FPGA memory space
  //----------------------------------------------------------------------------------------┐
  void *virtual_base_ram;// mapping of the FPGA memory space
  void *virtual_address_GPIO_HPS;// mapping of the GPIOs of HPS
  void *virtual_address_SDRAM_controller;// mapping of the SDRAM controller
  //----------------------------------------------------------------------------------------┘

  volatile uint32_t * sysid_p;// Address of system ID: read only
  volatile uint32_t * fwvers_p;// Address of firmware version: read only
  volatile uint32_t * led_p;// Address of LEDs: write only
  volatile uint32_t * key_p;// Address of KEYs: read only
  volatile uint32_t * sw_p;// Address of SWs: read only
  volatile uint32_t * iofifo_p;// Address of custom FIFOs structure: 6 words: 1 write only, 5 read only
  
  //--------------------------------------------------------------------------------------------------------------------┐
protected:
  volatile uint32_t * ram_32bit_p;// Base address of the reserved to the FPGA part of RAM
  volatile uint8_t  * ram_byte_p;// Base address of the reserved to the FPGA part of RAM
  volatile uint32_t * GPO_HPS_p;// Address of the GPO of HPS
  volatile uint32_t * GPI_HPS_p;// Address of the GPI of HPS
  volatile uint32_t * SDRAM_fpgaportrst_p;// Address of the SDRAM controller reset ports
  volatile uint32_t * SDRAM_calib_p;// Address of the SDRAM controller calibration flag

  volatile uint32_t data32_from_ram;
  volatile uint8_t data8_from_ram;
  uint32_t temp_RAM_control;
  //--------------------------------------------------------------------------------------------------------------------┘

};

#endif
