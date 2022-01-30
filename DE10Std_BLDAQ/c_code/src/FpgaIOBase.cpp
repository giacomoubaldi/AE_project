//  
//  FPGA IO BASE implementation
//
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <iostream>
#include <iomanip>
#include "FpgaIOBase.h"
#include "stdlib.h"

//----------------------------------------------------------------------------------

FpgaIOBase::FpgaIOBase(){
  fd = -1;  // set file descriptor to an invalid file number (ie: not opened).
  verbosity = 0;
}

FpgaIOBase::~FpgaIOBase(){
  if( fd>-1 ) 
    closeMem();
  fd = -1;
}

// Open memory device and assign register pointers
int FpgaIOBase::openMem(){

  // Access physical memory
  //std::cout<<"open /dev/mem"<<std::endl;
  if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
    std::cout<< "ERROR: could not open \"/dev/mem\"..."<<std::endl;
    fd = -1;
    return( -1 );
  }

  // Map physical memory from HW_REGS_BASE to HW_REGS_SPAN-1 in process virtual memory. Peripherals are mapped here.
  //std::cout<<"mmap  "<<std::endl;
  virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );
  if( virtual_base == MAP_FAILED ) {
    std::cout<< "ERROR: mmap() failed..."<<std::endl;
    close( fd );
    fd = -1;
    return( -1 );
  }

  // std::cout<<"define pointers  "<<std::endl;
  // HERE ALL THE DEFINITION OF POINTERS to peripherals
  sysid_p =  (uint32_t *)(((char *)virtual_base) + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + SYSID_QSYS_BASE ) & ( unsigned long)( HW_REGS_MASK ) ));
  fwvers_p = (uint32_t *)(((char *)virtual_base) + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + VERSION_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) ));
  led_p =    (uint32_t *)(((char *)virtual_base) + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + LED_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) ));
  key_p =    (uint32_t *)(((char *)virtual_base) + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + BUTTON_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) ));
  sw_p  =    (uint32_t *)(((char *)virtual_base) + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + DIPSW_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) ));
  iofifo_p = (uint32_t *)(((char *)virtual_base) + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + IOFIFOCONTROL_0_BASE ) & ( unsigned long)( HW_REGS_MASK ) ));
  
  // std::cout<<"End openMem  "<<std::endl;

  return fd;
}

int FpgaIOBase::closeMem(){

  // clean up our memory mapping
  if( munmap( virtual_base, HW_REGS_SPAN ) != 0 ) {
    std::cout<< "ERROR: munmap() failed..."<<std::endl;
    close( fd );
    return 1 ;
  }

  close( fd );
  fd = -1;
  return 0 ;
}

//----------------------------------------------------------------------------------------------------------------------------------┐
// New openmem and closemem to access reserved memory on RAM and other config stuff
int FpgaIOBase::openMem_RAM() {

  // Access physical memory
  // std::cout<<"open /dev/mem for RAM access"<<std::endl;
  if( ( fd_ram = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
    std::cerr<<"ERROR: could not open \"/dev/mem\" for accessing RAM..."<<std::endl;
    fd_ram = -1;
    return( -1 );
  }

  // Map physical memory from FPGA_reserved_memory_BASE_ADDRESS to FPGA_reserved_memory_AMOUNT-1 in virtual memory of process. Reserved memory for FPGA is mapped here.
  std::cout<<"mmap ram from "<<(std::hex)<<FPGA_reserved_memory_BASE_ADDRESS<<" to "<<FPGA_reserved_memory_BASE_ADDRESS + FPGA_reserved_memory_AMOUNT - 1<<(std::dec)<<std::endl;
  virtual_base_ram = mmap( NULL, FPGA_reserved_memory_AMOUNT, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd_ram, FPGA_reserved_memory_BASE_ADDRESS );
  if( virtual_base_ram == MAP_FAILED ) {
    std::cerr<<"ERROR: mmap() for RAM failed... "<<std::endl;
    close( fd_ram );
    fd_ram = -1;
    return( -1 );
  }

  // Map physical memory to access the GPIO of HPS. We use them as pointers and control signals to control the RAM used as a circular buffer
  //std::cout<<"mmap RAM buffer pointers "<<std::endl;
  virtual_address_GPIO_HPS = mmap( NULL, sysconf(_SC_PAGE_SIZE), ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd_ram, HPS_GPIO_BASE_ADDRESS );// page size is the minimum
  if( virtual_address_GPIO_HPS == MAP_FAILED ) {
    std::cerr<<"ERROR: mmap() for RAM pointers failed..."<<std::endl;
    close( fd_ram );
    fd_ram = -1;
    return( -1 );
  }

  // Define pointers
  // std::cout<<"Define pointer to reserved memory and ctrl regs"<<std::endl;
  ram_32bit_p = (uint32_t *)virtual_base_ram;// Base address of the reserved to the FPGA part of RAM (word addressable)
  ram_byte_p = (uint8_t *)virtual_base_ram;// Base address of the reserved to the FPGA part of RAM (byte addressable)
  GPO_HPS_p = (uint32_t *)( (uint8_t*)virtual_address_GPIO_HPS + (uint32_t)HPS_GPO_OFFSET );// Address of crtl reg HPS side (output)
  GPI_HPS_p = (uint32_t *)( (uint8_t*)virtual_address_GPIO_HPS + (uint32_t)HPS_GPI_OFFSET );// Address of crtl reg FPGA side (input)

  // std::cout<<"End openMem for RAM"<<std::endl;

  // std::cout<<"Resetting bridges and checking calibration of SDRAM\n mmap SDRAM_controller "<<std::endl);
  virtual_address_SDRAM_controller = mmap( NULL, SDRREGS_LENGTH, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd_ram, SDRREGS_BASE );
  if( virtual_address_SDRAM_controller == MAP_FAILED ) {
    std::cerr<<"ERROR: mmap() for SDRAM_controller failed... "<<std::endl;
    close( fd_ram );
    fd_ram = -1;
    return( -1 );
  }
  SDRAM_fpgaportrst_p = (uint32_t *)( (uint8_t*)virtual_address_SDRAM_controller + (uint32_t)FPGAPORTRST_OFFSET);
  SDRAM_calib_p       = (uint32_t *)( (uint8_t*)virtual_address_SDRAM_controller + (uint32_t)DRAMSTS_OFFSET);

  *(SDRAM_fpgaportrst_p) = 0;
  usleep(100000);
  *(SDRAM_fpgaportrst_p) = 0xFFFF;
  usleep(2000000);
  // std::cout<<"SDRAM calibration successful: "<< *(SDRAM_calib_p)<<std::endl;

  if( munmap( virtual_address_SDRAM_controller, SDRREGS_LENGTH ) != 0 ) {
    std::cout<< "ERROR: munmap() of SDRAM_controller failed..." <<std::endl;
    close( fd_ram );
    fd_ram = -1;
    return (-1) ;
  }

  // std::cout<<"\nDone \n");

  return fd_ram;
}

int FpgaIOBase::closeMem_RAM(){
  // clean up our memory mapping

  if( munmap( virtual_base_ram, FPGA_reserved_memory_AMOUNT ) != 0 ) {
    std::cout<< "ERROR: munmap() of RAM failed..." <<std::endl;
    close( fd_ram );
    return 1 ;
  }

  if( munmap( virtual_address_GPIO_HPS, sysconf(_SC_PAGE_SIZE) ) != 0 ) {
    std::cout<< "ERROR: munmap() of RAM failed..."<<std::endl;
    close( fd_ram );
    return 1 ;
  }

  close( fd_ram );
  fd_ram = -1;
  return 0 ;
}
//----------------------------------------------------------------------------------------------------------------------------------┘

void FpgaIOBase::printStatus(){
  static uint32_t oldsw=-1, oldkey=-1;
  char p[11];
  uint32_t sw, key;
  std::cout<<"printStatus  "<<std::endl;
  sw =getSwitches();
  key = getKeys();
  if( sw!=oldsw || key!=oldkey){
    strncpy(p," ******* ",10); 
  } else {
    strncpy(p, "\0",10);
  }
  oldsw = sw;
  oldkey = key;
  std::cout<<"Switches: S0="<<(sw&1)<<", S1="<<((sw>>1)&1)<<", S2="<<((sw>>2)&1)<<", S3="<<((sw>>3)&1)
	<<" -- Keys= K0="<<(key&1)<<", K1="<<((key>>1)&1)<<"   - "<< p<<std::endl;
}

void FpgaIOBase::printFifoStatus(){
  int fifreg;
  fifreg=getIFifoStatus();
  std::cout<<"Input    Fifo reg: "<<(std::hex)<<std::setw(8)<<fifreg<<",  "<<(std::dec)<<(fifreg&0xffff)<<" stored words,  empty="
		   <<((fifreg>>16)&1)<<", almost full="<<((fifreg>>17)&1)<<", full="<< ((fifreg>>18)&1)  <<std::endl;
  fifreg=getOFifoStatus();
  std::cout<<"Output   Fifo reg: "<<(std::hex)<<std::setw(8)<<fifreg<<",  "<<(std::dec)<<(fifreg&0xffff)<<" stored words,  empty="
		   <<((fifreg>>16)&1) <<", almost full="<< ((fifreg>>17)&1) <<", full="<< ((fifreg>>18)&1)  <<std::endl;
  fifreg=getOFifoRegStatus();
  std::cout<<"Register Fifo reg: "<<(std::hex)<<std::setw(8)<<fifreg<<",  "<<(std::dec)<<(fifreg&0xffff)<<" stored words,  empty="
		   <<((fifreg>>16)&1)<<", almost full="<<((fifreg>>17)&1)<<", full="<< ((fifreg>>18)&1)  <<std::endl;
}

//---------------------------------------------------------------------------------------------------------------------------------┐
/* uint32_t FpgaIOBase::Read_32bit_RAM(uint32_t addr_offset) {
  data32_from_ram = *( (uint32_t*)( ram_32bit_p + addr_offset ) );
  return data32_from_ram;
} */

void FpgaIOBase::Write_32bit_RAM(uint32_t data, uint32_t addr_offset) {
  *( (uint32_t*)( ram_32bit_p + addr_offset ) ) = data;
}

uint8_t FpgaIOBase::Read_byte_RAM(uint32_t addr_offset) {// Untested
  data8_from_ram = *( (uint8_t*)( ram_byte_p + addr_offset ));
  return data8_from_ram;
}

void FpgaIOBase::init_RAM(uint32_t data) {
  uint32_t i = 0;
  for (i=0; i<(FPGA_reserved_memory_AMOUNT/sizeof(uint32_t)); ++i) {
    *( (uint32_t*)( ram_32bit_p + i ) ) = data;
  }
}

void FpgaIOBase::Write_HPS_RAM_control(uint32_t addr_offset, uint32_t wrapped_around) {
  temp_RAM_control = 0;
  temp_RAM_control = (wrapped_around<<30) | ( (uint32_t)( (uint32_t*)FPGA_reserved_memory_BASE_ADDRESS + addr_offset ) & 0b111111111111111111111111111111 );
  *GPO_HPS_p = temp_RAM_control;
}

void FpgaIOBase::Read_FPGA_RAM_control(volatile uint32_t* addr_offset, volatile uint32_t* wrapped_around) {
  // printf("GPI: %p   GPO: %p  FPGA Res memory address: %x \n", GPI_HPS_p,GPO_HPS_p, FPGA_reserved_memory_BASE_ADDRESS);
  temp_RAM_control = *GPI_HPS_p;
  // printf("addr_offset: %d   wrap around: %d \n",( (temp_RAM_control & 0b111111111111111111111111111111) - (uint32_t)FPGA_reserved_memory_BASE_ADDRESS ) / sizeof(uint32_t), (temp_RAM_control>>30) & 0b1 );
  *(addr_offset) = ( (temp_RAM_control & 0b111111111111111111111111111111) - (uint32_t)FPGA_reserved_memory_BASE_ADDRESS ) / sizeof(uint32_t);
  *(wrapped_around) = (temp_RAM_control>>30) & 0b1;
}

void FpgaIOBase::printRAMStatus(){ // status of RAM Block
  std::cout<<std::endl<<"RAM status"<<std::endl;
  std::cout<<"RAM base address "<<ram_32bit_p<<std::endl;
  std::cout<<"RAM HPS  read  pointer GPO address "<<GPO_HPS_p<<",   value="<<(std::hex)<< *GPO_HPS_p<<(std::dec)<<std::endl;
  std::cout<<"RAM FPGA write pointer GPI address "<<GPI_HPS_p<<",   value="<<(std::hex)<< *GPI_HPS_p<<(std::dec)<<std::endl;
}
//---------------------------------------------------------------------------------------------------------------------------------┘