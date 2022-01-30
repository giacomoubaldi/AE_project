//  
//  FPGA IO BASE implementation
//
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include "FpgaIOBase.h"


FpgaIOBase::FpgaIOBase(){
  fd = -1;  // set file descriptor to an invalid file number (ie: not opened).
}

FpgaIOBase::~FpgaIOBase(){
  if( fd>-1 ) 
    closeMem();
  fd = -1;
}

// open memory device and assign register pointers
// return -1 in case of errors
int FpgaIOBase::openMem(){
  // map the address space for the LED registers into user space so we can interact with them.
  // we'll actually map in the entire CSR span of the HPS since we want to access various registers within that span

  printf("open /dev/mem \n");
  if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) {
    printf( "ERROR: could not open \"/dev/mem\"...\n" );
    fd = -1;  // file not opened
    return( -1 );
  }

  printf("mmap  \n");
  virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );

  if( virtual_base == MAP_FAILED ) {
    printf( "ERROR: mmap() failed...\n" );
    close( fd );
    fd = -1;
    return( -1 );
  }

  printf("define pointers  \n");
  //  HERE ALL THE DEFINITION OF POINTERS. To be checked!!
  led_p = (uint32_t *)(((char *)virtual_base) + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + LED_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) ));
  key_p = (uint32_t *)(((char *)virtual_base) + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + BUTTON_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) ));
  sw_p  = (uint32_t *)(((char *)virtual_base) + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + DIPSW_PIO_BASE ) & ( unsigned long)( HW_REGS_MASK ) ));
  iofifo_p = (uint32_t *)(((char *)virtual_base) + ( ( unsigned long  )( ALT_LWFPGASLVS_OFST + IOFIFOSCONTROL_BASE ) & ( unsigned long)( HW_REGS_MASK ) ));
  
  printf("end openMem  \n");

  return fd;
}

int FpgaIOBase::closeMem(){
  // clean up our memory mapping
	
  if( munmap( virtual_base, HW_REGS_SPAN ) != 0 ) {
    printf( "ERROR: munmap() failed...\n" );
    close( fd );
    return 1 ;
  }

  close( fd );
  fd = -1;
  return 0 ;
}

void FpgaIOBase::printStatus(){
  static uint32_t oldsw=-1, oldkey=-1;
  char p[11];
  uint32_t sw, key;
  printf("printStatus  \n");
  sw =getSwitches();
  key = getKeys();
  if( sw!=oldsw || key!=oldkey){
    strncpy(p," ******* ",10); 
  } else {
    strncpy(p, "\0",10);
  }
  oldsw = sw;
  oldkey = key;
  printf("Switches: S0=%d, S1=%d, S2=%d, S3=%d -- Keys= K0=%d, K1=%d   - %s \n", sw&1, (sw>>1)&1, (sw>>2)&1, (sw>>3)&1, key&1, (key>>1)&1, p);
}


void FpgaIOBase::printFifoStatus(){
  int fifreg;
  fifreg=getIFifoStatus();
  printf("Input    Fifo reg: %08x,  %d stored words,  empty=%d, almost full=%d, full=%d \n", fifreg, (fifreg&0xffff), (fifreg>>16)&1, (fifreg>>17)&1, (fifreg>>18)&1);
  fifreg=getOFifoStatus();
  printf("Output   Fifo reg: %08x,  %d stored words,  empty=%d, almost full=%d, full=%d \n", fifreg, (fifreg&0xffff), (fifreg>>16)&1, (fifreg>>17)&1, (fifreg>>18)&1);
  fifreg=getOFifoRegStatus();
  printf("Register Fifo reg: %08x,  %d stored words,  empty=%d, almost full=%d, full=%d \n", fifreg, (fifreg&0xffff), (fifreg>>16)&1, (fifreg>>17)&1, (fifreg>>18)&1);
}


