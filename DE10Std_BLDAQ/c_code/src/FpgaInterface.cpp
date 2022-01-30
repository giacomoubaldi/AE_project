//
//
//
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <iostream>
#include <iomanip>
#include "FpgaInterface.h"

//------------------------------------┐
#define MON_REGS_OFFSET 16
#define ADC_CH0_reg 10
#define ADC_CH1_reg 10
#define ADC_CH2_reg 11
#define ADC_CH3_reg 11
#define ADC_CH4_reg 12
#define ADC_CH5_reg 12
#define ADC_CH6_reg 13
#define ADC_CH7_reg 13
//------------------------------------┘

FpgaInterface::FpgaInterface() : FpgaIOBase() {
  for(int i=0; i<32; i++) regs[i]= 0xffffffff; // an invalid initialization number
}

FpgaInterface::~FpgaInterface(){
}

//          COMMANDS

void FpgaInterface::sendReadAllRegs(){
  checkIFifo();
  unsigned int nreg = 16+32; // number of registers to read
  setIFifoData(nreg+3);   // number of data to transfer
  setIFifoData(Header_ReadRegs);
  for(unsigned int i=0; i<nreg; i++)
    setIFifoData(i);
  setIFifoData(Instruction_Footer); 
}

void FpgaInterface::sendReadReg(uint32_t reg){
  checkIFifo();	
  setIFifoData(4);
  setIFifoData(Header_ReadRegs);
  setIFifoData(reg);     // reading out a specific register
  setIFifoData(Instruction_Footer); 
}

void FpgaInterface::sendWriteReg(uint32_t reg, uint32_t value){
  checkIFifo();	
  setIFifoData(5);
  setIFifoData(Header_WriteRegs);
  setIFifoData(reg);     // writing register "reg" with 
  setIFifoData(value);   // this "value"
  setIFifoData(Instruction_Footer);    
}

void FpgaInterface::sendGoTo(int stat){
  checkIFifo();
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
      if( verbosity>2 ) std::cout<<(std::hex)<<std::setw(9)<<dat;			
      nmax--;
      data.push_back(dat);
    }
    if( verbosity>2 ) std::cout<<std::endl;			
  }
  if( verbosity>1 ) printFifoStatus();					
}

// be sure that the input fifo is ready to get new data
void FpgaInterface::checkIFifo(){
  int timeout = 0;
  while( (getIFifoStatus()&0x80000)==0x80000 && timeout<100){ // test for busy
	usleep(1000);
	timeout++;
  }
  if( timeout>2 ){
	  std::cout<<" Input Fifo busy for part of the time...:"<<timeout<<"timeout counter."<<std::endl;
  }
  timeout=0;
  while( (getIFifoStatus()&0x10000)==0x00000 && timeout<100){ // test for not empty
	usleep(1000);
	timeout++;
  }
  if( timeout>2 ){
	  std::cout<<" Input Fifo not empty for part of the time...:"<<timeout<<"timeout counter."<<std::endl;
  }
}


//-------------------------------------------------------------------------------------------------------------------------------------┐
// Empty the RAM fifo
void FpgaInterface::cleanUpRAM(){
//  printf("FPGA offset: %d   HPS offset: %d,   FPGA Wrap: %d,  HPS Wrap: %d \n", fpga_offset_RAM, hps_offset_RAM, fpga_wrapped_around, hps_wrapped_around);
  Read_FPGA_RAM_control(&fpga_offset_RAM, &fpga_wrapped_around);// Check how's the situation on the FPGA side
  hps_wrapped_around = fpga_wrapped_around;
  hps_offset_RAM = fpga_offset_RAM;
  Write_HPS_RAM_control(hps_offset_RAM, hps_wrapped_around);// Send to FPGA new HPS status
  usleep(1000); // wait 1 ms for the FPGA to write there...
  Read_FPGA_RAM_control(&fpga_offset_RAM, &fpga_wrapped_around);// Check how's the situation on the FPGA side
  hps_wrapped_around = fpga_wrapped_around;
  hps_offset_RAM = fpga_offset_RAM;
  Write_HPS_RAM_control(hps_offset_RAM, hps_wrapped_around);// Send to FPGA new HPS status  
}


uint32_t FpgaInterface::wordsAvailable_on_RAM() {
  uint32_t num_words = 0;// Init number of read words
  Read_FPGA_RAM_control(&fpga_offset_RAM, &fpga_wrapped_around);// Check how's the situation on the FPGA side
  p_in_different_page = hps_wrapped_around ^ fpga_wrapped_around;// Check if fpga_pointer has wrapped around
  //printf("fpga_offset: %d  hps_offset: %d   wrap fpga:%d wrap hps:%d   wrap around: %d \n", 
  //       fpga_offset_RAM,hps_offset_RAM ,fpga_wrapped_around, hps_wrapped_around,p_in_different_page);
  // sanity chech
  if( fpga_offset_RAM>FPGA_reserved_memory_MAX_OFFSET )
	  std::cout<<"words() sanity check on fpga offset: value: "<<fpga_offset_RAM<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET<<std::endl;
  // sanity chech
  if( hps_offset_RAM>FPGA_reserved_memory_MAX_OFFSET )
	  std::cout<<"words() sanity check on hps  offset: value: "<<hps_offset_RAM<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET<<std::endl;

  if ( (hps_offset_RAM == fpga_offset_RAM) ){  // the two pointers are equal.....
	if( p_in_different_page )	
	  num_words = FPGA_reserved_memory_MAX_OFFSET+1;		// the ram is full
	else 
      num_words = 0;									// the ram is empty
  } else {
	if( (!p_in_different_page) ){	// the two pointers are different, but in the same page
	  num_words = fpga_offset_RAM-hps_offset_RAM;
    }	else {
	  num_words = (FPGA_reserved_memory_MAX_OFFSET+1+fpga_offset_RAM)-hps_offset_RAM;
	}
  }
  return num_words;
}

void FpgaInterface::readOfifo_on_RAM(std::vector<uint32_t> & data, uint32_t nmax) {
  num_words_read = 0;// Init number of read words
  Read_FPGA_RAM_control(&fpga_offset_RAM, &fpga_wrapped_around);// Check how's the situation on the FPGA side
  p_in_different_page = hps_wrapped_around ^ fpga_wrapped_around;// Check if fpga_pointer has wrapped around

  if ( (hps_offset_RAM == fpga_offset_RAM) && (p_in_different_page) ) {// The whole memory has been written. HPS has to read the whole memory
    do {                                                               // (don't do that in a single call of this function or the OS will crash)
      data.push_back(Read_32bit_RAM(hps_offset_RAM));
      num_words_read++;
      if (hps_offset_RAM == FPGA_reserved_memory_MAX_OFFSET) {
        hps_offset_RAM = 0;
        hps_wrapped_around = !hps_wrapped_around;
      }
      else {
        hps_offset_RAM++;
      }
    } while ( (hps_offset_RAM != fpga_offset_RAM) && (num_words_read<nmax) );// The Do while is required because hps_offset_RAM is already equal fpga_offset_RAM
  }

  if ( (hps_offset_RAM != fpga_offset_RAM) && (!p_in_different_page) ) {// Normal case
    while( (hps_offset_RAM != fpga_offset_RAM) && (num_words_read<nmax) ) {
      data.push_back(Read_32bit_RAM(hps_offset_RAM));
      num_words_read++;
      hps_offset_RAM++;
    }
  }

  if ( (hps_offset_RAM != fpga_offset_RAM) && (p_in_different_page) ) {// Normal case but a pointer has wrapped around
    while( (hps_offset_RAM != fpga_offset_RAM) && (num_words_read<nmax) ) {
      data.push_back(Read_32bit_RAM(hps_offset_RAM));
      num_words_read++;
      if (hps_offset_RAM == FPGA_reserved_memory_MAX_OFFSET) {
        hps_offset_RAM = 0;
        hps_wrapped_around = !hps_wrapped_around;
      }
      else {
        hps_offset_RAM++;
      }
    }
  }

  if ( (hps_offset_RAM == fpga_offset_RAM) && (!p_in_different_page) ) {// FPGA hasn't written anything, so don't do anything
    ;
  }

  Write_HPS_RAM_control(hps_offset_RAM, hps_wrapped_around);// Send to FPGA new HPS status
}



void FpgaInterface::readOfifo_on_RAM_fast(std::vector<uint32_t> & data, uint32_t nmax) {
  num_words_read = 0;// Init number of read words
  Read_FPGA_RAM_control(&fpga_offset_RAM, &fpga_wrapped_around);// Check how's the situation on the FPGA side

  // sanity chech
  if( fpga_offset_RAM>FPGA_reserved_memory_MAX_OFFSET )
	  std::cout<<"words() sanity check on fpga offset: value: "<<fpga_offset_RAM<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET<<std::endl;


  p_in_different_page = hps_wrapped_around ^ fpga_wrapped_around;// Check if fpga_pointer has wrapped around
  uint32_t hps_offset_RAM_orig = hps_offset_RAM;
  uint32_t hps_wrapped_around_orig = hps_wrapped_around;
  uint32_t *  begin1, *end1, *begin2,*end2;
  bool twocopies = false;
  begin1  = (uint32_t*)( ram_32bit_p + hps_offset_RAM);  // the first begin is always easy....
  begin2  = (uint32_t*)( ram_32bit_p + 0);  // but also the second begin is easy....
  
  if( (hps_offset_RAM &0x3fffffff)> FPGA_reserved_memory_MAX_OFFSET)
	  std::cout<<"Error on hps offset :  "<<(std::hex)<<std::setw(8)<<hps_offset_RAM<<(std::dec)<<std::endl;
  if( (fpga_offset_RAM &0x3fffffff)> FPGA_reserved_memory_MAX_OFFSET)
	  std::cout<<"Error on fpga offset :  "<<(std::hex)<<std::setw(8)<<fpga_offset_RAM<<(std::dec)<<std::endl;
  
  // now assume that the first byte contains always the event size_type
  uint32_t evsize =*begin1;
  if( evsize<5000 ){
	  nmax = evsize;	  
  }
  else{
	  std::cout<<"Event size: "<<evsize<<std::endl;
	  std::cout<<"Sanity check on fpga offset: value: "<<(std::hex)<<fpga_offset_RAM<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET <<std::endl;
	  std::cout<<"Sanity check on hps  offset: value: "<<(std::hex)<< hps_offset_RAM<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET <<std::endl;
  }
  uint32_t hps_offset_RAM_old = hps_offset_RAM;
  
  if( (p_in_different_page) ){
	if( FPGA_reserved_memory_MAX_OFFSET+1> hps_offset_RAM+nmax ){  // the reading will not wrap
	  twocopies = false;
	  num_words_read = nmax;
	  end1 = (uint32_t*)( ram_32bit_p + hps_offset_RAM+nmax);
	  hps_offset_RAM += nmax;
	  if( hps_offset_RAM==FPGA_reserved_memory_MAX_OFFSET+1 ){
		hps_offset_RAM= 0;
		hps_wrapped_around = !hps_wrapped_around;
	  }
	} else {  // reading will wrap
	  twocopies =  true;
	  end1 = (uint32_t*)( ram_32bit_p + FPGA_reserved_memory_MAX_OFFSET+1);
	  num_words_read = FPGA_reserved_memory_MAX_OFFSET-hps_offset_RAM+1;
      if( fpga_offset_RAM==0){
  	    twocopies = false;
	    hps_offset_RAM = 0;
	  } else if( fpga_offset_RAM+num_words_read > nmax ){  // read only a part of the available data
	    end2 = (uint32_t*)( (ram_32bit_p + nmax)-num_words_read);
		hps_offset_RAM = nmax-num_words_read;
		num_words_read = nmax;
	  } else {									  // read all the available data
		end2 = (uint32_t*)( ram_32bit_p +fpga_offset_RAM);
		num_words_read += fpga_offset_RAM;
		hps_offset_RAM = fpga_offset_RAM;
	  }
	  hps_wrapped_around = !hps_wrapped_around;
	}
  } else {
	twocopies = false;
	if( fpga_offset_RAM==hps_offset_RAM ){ // the RAM is empty
	  end1 = begin1+1;
	  num_words_read=0;
	} else if( fpga_offset_RAM < hps_offset_RAM+nmax ){  // read all the available data
      end1 = (uint32_t*)( ram_32bit_p + fpga_offset_RAM+1);
	  num_words_read = fpga_offset_RAM-hps_offset_RAM-1;
	  hps_offset_RAM = fpga_offset_RAM;
	} else {  // read only part of the available data
      end1 = (uint32_t*)( ram_32bit_p + hps_offset_RAM+nmax);
	  num_words_read = nmax;
	  hps_offset_RAM = hps_offset_RAM+nmax;	
	}	
  }
  
  // this is a fast insertion method
  if( num_words_read>0 ){
	if( end1<=begin1 || ((end1-begin1)+data.size()>250000) ){
      std::cout<<"fpga_offset: "<<fpga_offset_RAM<<"  hps_offset: "<<hps_offset_RAM_orig<<" (now:"<<hps_offset_RAM
			    <<")  wrap fpga:"<<fpga_wrapped_around<<" wrap hps:"<<hps_wrapped_around_orig<<" (now:"<<hps_wrapped_around
				<<")  wrap around: "<<p_in_different_page<<std::endl;		
 	  std::cout<<"begin1 "<<begin1<<"   end1 "<<end1<<"  to be inserted: "<<(std::dec)<<end1-begin1<<",  vector size: "<<data.size()<<std::endl;
	}
    data.insert(data.end(), begin1, end1);
	if( twocopies ){
	  if( end2<=begin2 || ((end2-begin2)+data.size()>250000) ){
        printf("fpga_offset: %d  hps_offset: %d (now:%d)  wrap fpga:%d wrap hps:%d (now:%d)  wrap around: %d \n", 
			   fpga_offset_RAM,hps_offset_RAM_orig ,hps_offset_RAM,fpga_wrapped_around, hps_wrapped_around_orig,hps_wrapped_around,p_in_different_page);		
 	    printf("begin2 %p   end2 %p  to be inserted: %d,  vector size: %d\n",begin2, end2, end2-begin2,data.size());
	  }
      data.insert(data.end(), begin2, end2);
	}
  }
  // sanity chech
  if( hps_offset_RAM>FPGA_reserved_memory_MAX_OFFSET )
	  std::cout<<"Sanity check on hps offset: value: "<<hps_offset_RAM<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET<<std::endl;
  bool ramIsEmpty = (hps_offset_RAM==fpga_offset_RAM) && ((hps_wrapped_around^fpga_wrapped_around)==0);
  if( (!ramIsEmpty) && (*(ram_32bit_p + hps_offset_RAM)>1000 || *(ram_32bit_p + hps_offset_RAM)<0) ){
	  std::cout<<"Sanity check on fpga offset: value: "<<fpga_offset_RAM<<",  wrapped: "<<p_in_different_page<<std::endl;
	  std::cout<<"Sanity check on hps offset old: value: "<<hps_offset_RAM_old<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET<<std::endl;
	  std::cout<<"Sanity check on hps offset new: value: "<<hps_offset_RAM<<",  wrap: "<<hps_wrapped_around<<std::endl;
	  std::cout<<"Sanity event size: "<<(std::dec)<<*(ram_32bit_p + hps_offset_RAM_old)<<", readout:"<<data.size()<<"  next size: "<<*(ram_32bit_p + hps_offset_RAM)<<std::endl;
 }
  
  Write_HPS_RAM_control(hps_offset_RAM, hps_wrapped_around);// Send to FPGA new HPS status
}




void FpgaInterface::readOfifo_on_RAM_fast2(std::vector<uint32_t> & data, uint32_t nmax) {

  Read_FPGA_RAM_control(&fpga_offset_RAM, &fpga_wrapped_around);// Check how's the situation on the FPGA side

  uint32_t wordsAvailable = wordsAvailable_on_RAM();
  if( wordsAvailable==0 ){
	  std::cout<<"No data available on RAM now..."<<std::endl;
	  usleep(200);
	  return;
  }


  uint32_t hps_offset_RAM_orig = hps_offset_RAM;
  uint32_t hps_wrapped_around_orig = hps_wrapped_around;
  uint32_t *  begin1, *end1, *begin2,*end2;
  bool twocopies = false;
  begin1  = (uint32_t*)( ram_32bit_p + hps_offset_RAM);  // the first begin is always easy....
  begin2  = (uint32_t*)( ram_32bit_p + 0);  // but also the second begin is easy....
  
  if( hps_offset_RAM > FPGA_reserved_memory_MAX_OFFSET)
	  std::cout<<"Error on hps offset :  "<<(std::hex)<<std::setw(8)<<hps_offset_RAM<<(std::dec)<<std::endl;
  if( fpga_offset_RAM > FPGA_reserved_memory_MAX_OFFSET)
	  std::cout<<"Error on fpga offset :  "<<(std::hex)<<std::setw(8)<<fpga_offset_RAM<<(std::dec)<<std::endl;
  
  // now assume that the first word contains always the event size_type
  uint32_t evsize =*begin1;
  if( evsize>5000 ){
	  std::cout<<"Error on size. Event size: "<<evsize<<" Words available:"<<wordsAvailable<<std::endl;
	  std::cout<<"Error on size. Sanity check on fpga offset: value: "<<(std::hex)<<fpga_offset_RAM<<", wrap:"<<fpga_wrapped_around<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET <<std::endl;
	  std::cout<<"Error on size. Sanity check on hps  offset: value: "<<(std::hex)<< hps_offset_RAM<<", wrap:"<<hps_wrapped_around<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET <<std::endl;
	  uint32_t *begin0=begin1;
	  for(int i=0; i<20; i++){
		std::cout<<(std::hex)<<std::setw(9)<<*begin0<<" ";
	    begin0++;
	  }
  }
  if( nmax>wordsAvailable ) nmax = wordsAvailable;
  
  uint32_t hps_offset_RAM_old = hps_offset_RAM;
  twocopies = false;

  uint32_t nevents = 0;// number of events read out
  num_words_read = 0;// Init number of read words

  bool done = false;
  while( !done ){
    evsize = (uint32_t) *(ram_32bit_p + hps_offset_RAM);
//	std::cout<<"check on hps offset: value: "<<(std::hex)<< hps_offset_RAM<<",  evsize: "<<(std::dec)<<evsize<<"  events:"<<nevents<<std::endl;
    if( evsize > nmax || evsize+num_words_read>nmax ){ 
	  done = true;
    } else{
	  nevents++;
      num_words_read+=evsize;
	  hps_offset_RAM+=evsize;
	  if( hps_offset_RAM>FPGA_reserved_memory_MAX_OFFSET ){
// 	    std::cout<<"Two copies check on hps offset: value: "<<(std::hex)<< hps_offset_RAM<<",  evsize: "<<(std::dec)<<evsize<<"  events:"<<nevents<<std::endl;
        hps_offset_RAM -= (FPGA_reserved_memory_MAX_OFFSET+1) ;
		hps_wrapped_around = !hps_wrapped_around;
// 	    std::cout<<"Two copies check on hps offset: value: "<<(std::hex)<< hps_offset_RAM<<",  evsize: "<<(std::dec)<<evsize<<"  events:"<<nevents<<std::endl;
// 	    std::cout<<"Next event size: "<<(std::hex)<< (uint32_t) *(ram_32bit_p + hps_offset_RAM)<<std::endl;
        twocopies =  true;
	  }
	}
  }
  
  end1 = (uint32_t*)(ram_32bit_p + hps_offset_RAM);
  end2 = begin2;
  if( twocopies ){
    end2 = end1;
	end1 = (uint32_t*)(ram_32bit_p + FPGA_reserved_memory_MAX_OFFSET+1);
  }

   
  // this is a fast insertion method
  if( num_words_read>0 ){
	if( end1<=begin1 || ((end1-begin1)+data.size()>data.capacity()) ){
      std::cout<<"Error on pointers. fpga_offset: "<<fpga_offset_RAM<<"  hps_offset: "<<hps_offset_RAM_orig<<" (now:"<<hps_offset_RAM
			    <<")  wrap fpga:"<<fpga_wrapped_around<<" wrap hps:"<<hps_wrapped_around_orig<<" (now:"<<hps_wrapped_around
				<<")  wrap around: "<<p_in_different_page<<std::endl;		
 	  std::cout<<"Error on pointers. begin1 "<<begin1<<"   end1 "<<end1<<"  to be inserted: "<<(std::dec)<<end1-begin1<<",  vector size: "<<data.size()<<std::endl;
	}
    data.insert(data.end(), begin1, end1);
	if( twocopies ){
	  if( end2<=begin2 || ((end2-begin2)+data.size()>data.capacity()) ){
      std::cout<<"Error on pointers. fpga_offset: "<<fpga_offset_RAM<<"  hps_offset: "<<hps_offset_RAM_orig<<" (now:"<<hps_offset_RAM
			    <<")  wrap fpga:"<<fpga_wrapped_around<<" wrap hps:"<<hps_wrapped_around_orig<<" (now:"<<hps_wrapped_around
				<<")  wrap around: "<<p_in_different_page<<std::endl;		
 	  std::cout<<"Error on pointers. begin2 "<<begin2<<"   end2 "<<end2<<"  to be inserted: "<<(std::dec)<<end2-begin2<<",  vector size: "<<data.size()<<std::endl;
	  }
      data.insert(data.end(), begin2, end2);
	}
  }
  // sanity chech
  if( hps_offset_RAM>FPGA_reserved_memory_MAX_OFFSET )
	  std::cout<<"Sanity check on hps offset: value: "<<hps_offset_RAM<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET<<std::endl;
  bool Empty = (fpga_offset_RAM==hps_offset_RAM) && (hps_wrapped_around^fpga_wrapped_around)==0;
  if( !Empty && (*(ram_32bit_p + hps_offset_RAM)>5000 || *(ram_32bit_p + hps_offset_RAM)<0) ){
	  std::cout<<"Sanity check on fpga offset: value: "<<fpga_offset_RAM<<",  wrapped: "<<p_in_different_page<<std::endl;
	  std::cout<<"Sanity check on hps offset old: value: "<<hps_offset_RAM_old<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET<<std::endl;
	  std::cout<<"Sanity check on hps offset new: value: "<<hps_offset_RAM<<",  wrap: "<<hps_wrapped_around<<std::endl;
	  std::cout<<"Sanity event size: "<<(std::dec)<<*(ram_32bit_p + hps_offset_RAM_old)<<", readout:"<<data.size()<<"  next size: "<<*(ram_32bit_p + hps_offset_RAM)<<std::endl;
  }
  
  Write_HPS_RAM_control(hps_offset_RAM, hps_wrapped_around);// Send to FPGA new HPS status
}



void FpgaInterface::readOfifo_on_RAM_fast3(uint32_t tail, uint32_t head, std::vector<uint32_t> & data, uint32_t nmax) {

  fpga_offset_RAM = head & 0x3fffffff;
  fpga_wrapped_around = (head>>30 ) & 1;
  hps_offset_RAM = tail & 0x3fffffff;
  hps_wrapped_around = (tail>>30 ) & 1;
//  Read_FPGA_RAM_control(&fpga_offset_RAM, &fpga_wrapped_around);// Check how's the situation on the FPGA side

  uint32_t wordsAvailable = wordsAvailable_on_RAM();
  if( wordsAvailable==0 ){
	  std::cout<<"No data available on RAM now..."<<std::endl;
	  usleep(200);
	  return;
  }


  uint32_t hps_offset_RAM_orig = hps_offset_RAM;
  uint32_t hps_wrapped_around_orig = hps_wrapped_around;
  uint32_t *  begin1, *end1, *begin2,*end2;
  bool twocopies = false;
  begin1  = (uint32_t*)( ram_32bit_p + hps_offset_RAM);  // the first begin is always easy....
  begin2  = (uint32_t*)( ram_32bit_p + 0);  // but also the second begin is easy....
  
  if( hps_offset_RAM > FPGA_reserved_memory_MAX_OFFSET)
	  std::cout<<"Error on hps offset :  "<<(std::hex)<<std::setw(8)<<hps_offset_RAM<<(std::dec)<<std::endl;
  if( fpga_offset_RAM > FPGA_reserved_memory_MAX_OFFSET)
	  std::cout<<"Error on fpga offset :  "<<(std::hex)<<std::setw(8)<<fpga_offset_RAM<<(std::dec)<<std::endl;
  
  // now assume that the first word contains always the event size_type
  uint32_t evsize =*begin1;
  if( evsize>5000 ){
	  std::cout<<"Error on size. Event size: "<<evsize<<" Words available:"<<wordsAvailable<<std::endl;
	  std::cout<<"Error on size. Sanity check on fpga offset: value: "<<(std::hex)<<fpga_offset_RAM<<", wrap:"<<fpga_wrapped_around<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET <<std::endl;
	  std::cout<<"Error on size. Sanity check on hps  offset: value: "<<(std::hex)<< hps_offset_RAM<<", wrap:"<<hps_wrapped_around<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET <<std::endl;
	  uint32_t *begin0=begin1;
	  for(int i=0; i<20; i++){
		std::cout<<(std::hex)<<std::setw(9)<<*begin0<<" ";
	    begin0++;
	  }
  }
  if( nmax>wordsAvailable ) nmax = wordsAvailable;
  
  uint32_t hps_offset_RAM_old = hps_offset_RAM;
  twocopies = false;

  uint32_t nevents = 0;// number of events read out
  num_words_read = 0;// Init number of read words

  bool done = false;
  while( !done ){
    evsize = (uint32_t) *(ram_32bit_p + hps_offset_RAM);
//	std::cout<<"check on hps offset: value: "<<(std::hex)<< hps_offset_RAM<<",  evsize: "<<(std::dec)<<evsize<<"  events:"<<nevents<<std::endl;
    if( evsize > nmax || evsize+num_words_read>nmax ){ 
	  done = true;
    } else{
	  nevents++;
      num_words_read+=evsize;
	  hps_offset_RAM+=evsize;
	  if( hps_offset_RAM>FPGA_reserved_memory_MAX_OFFSET ){
// 	    std::cout<<"Two copies check on hps offset: value: "<<(std::hex)<< hps_offset_RAM<<",  evsize: "<<(std::dec)<<evsize<<"  events:"<<nevents<<std::endl;
        hps_offset_RAM -= (FPGA_reserved_memory_MAX_OFFSET+1) ;
		hps_wrapped_around = !hps_wrapped_around;
// 	    std::cout<<"Two copies check on hps offset: value: "<<(std::hex)<< hps_offset_RAM<<",  evsize: "<<(std::dec)<<evsize<<"  events:"<<nevents<<std::endl;
// 	    std::cout<<"Next event size: "<<(std::hex)<< (uint32_t) *(ram_32bit_p + hps_offset_RAM)<<std::endl;
        twocopies =  true;
	  }
	}
  }
  
  end1 = (uint32_t*)(ram_32bit_p + hps_offset_RAM);
  end2 = begin2;
  if( twocopies ){
    end2 = end1;
	end1 = (uint32_t*)(ram_32bit_p + FPGA_reserved_memory_MAX_OFFSET+1);
  }

   
  // this is a fast insertion method
  if( num_words_read>0 ){
	if( end1<=begin1 || ((end1-begin1)+data.size()>data.capacity()) ){
      std::cout<<"Error on pointers. fpga_offset: "<<fpga_offset_RAM<<"  hps_offset: "<<hps_offset_RAM_orig<<" (now:"<<hps_offset_RAM
			    <<")  wrap fpga:"<<fpga_wrapped_around<<" wrap hps:"<<hps_wrapped_around_orig<<" (now:"<<hps_wrapped_around
				<<")  wrap around: "<<p_in_different_page<<std::endl;		
 	  std::cout<<"Error on pointers. begin1 "<<begin1<<"   end1 "<<end1<<"  to be inserted: "<<(std::dec)<<end1-begin1<<",  vector size: "<<data.size()<<std::endl;
	}
    data.insert(data.end(), begin1, end1);
	if( twocopies ){
	  if( end2<=begin2 || ((end2-begin2)+data.size()>data.capacity()) ){
      std::cout<<"Error on pointers. fpga_offset: "<<fpga_offset_RAM<<"  hps_offset: "<<hps_offset_RAM_orig<<" (now:"<<hps_offset_RAM
			    <<")  wrap fpga:"<<fpga_wrapped_around<<" wrap hps:"<<hps_wrapped_around_orig<<" (now:"<<hps_wrapped_around
				<<")  wrap around: "<<p_in_different_page<<std::endl;		
 	  std::cout<<"Error on pointers. begin2 "<<begin2<<"   end2 "<<end2<<"  to be inserted: "<<(std::dec)<<end2-begin2<<",  vector size: "<<data.size()<<std::endl;
	  }
      data.insert(data.end(), begin2, end2);
	}
  }
  // sanity chech
  if( hps_offset_RAM>FPGA_reserved_memory_MAX_OFFSET )
	  std::cout<<"Sanity check on hps offset: value: "<<hps_offset_RAM<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET<<std::endl;
  bool Empty = (fpga_offset_RAM==hps_offset_RAM) && (hps_wrapped_around^fpga_wrapped_around)==0;
  if( !Empty && (*(ram_32bit_p + hps_offset_RAM)>5000 || *(ram_32bit_p + hps_offset_RAM)<0) ){
	  std::cout<<"Sanity check on fpga offset: value: "<<fpga_offset_RAM<<",  wrapped: "<<p_in_different_page<<std::endl;
	  std::cout<<"Sanity check on hps offset old: value: "<<hps_offset_RAM_old<<",  max: "<<FPGA_reserved_memory_MAX_OFFSET<<std::endl;
	  std::cout<<"Sanity check on hps offset new: value: "<<hps_offset_RAM<<",  wrap: "<<hps_wrapped_around<<std::endl;
	  std::cout<<"Sanity event size: "<<(std::dec)<<*(ram_32bit_p + hps_offset_RAM_old)<<", readout:"<<data.size()<<"  next size: "<<*(ram_32bit_p + hps_offset_RAM)<<std::endl;
  }
  
  //Write_HPS_RAM_control(hps_offset_RAM, hps_wrapped_around);// Send to FPGA new HPS status
}

//-------------------------------------------------------------------------------------------------------------------------------------┘



//-------------------------------------------------------------------------------------------------------------------------------------┘

void FpgaInterface::readORegfifo(std::vector<uint32_t> & data){
  int nmax = 2000;
  if( verbosity>1 ) printFifoStatus();		
  usleep(1000);    // just for this moment 1ms delay!
  // wait until the input fifo is ready again!
  checkIFifo();
  std::cout<<(std::hex);
  while( !OFifoRegEmpty() ){
    int n=getOFifoRegStatus()&0xffff;
    if( n==0 || nmax <0 ) break;
    if( n>10 ) n=10;
    for(int idx=0; idx<n; idx++){
      uint32_t dat= getOFifoRegData();
      if( verbosity>0 ) std::cout<<dat<<" ";			
      nmax--;
      data.push_back(dat);
    }
    if( verbosity>0 ) std::cout<<(std::dec)<<std::endl;			
  }
  if( verbosity>0 ) printFifoStatus();					
}

void FpgaInterface::PrintAllRegs(){
	
  std::vector<uint32_t> data;

  std::cout<<std::endl<<"Reading all regs"<<std::endl<<std::endl;
  sendReadAllRegs();
  std::cout<<"Reading OREG all regs"<<std::endl<<std::endl;
  readORegfifo(data);
  std::cout<<"Done"<<std::endl<<std::endl;
//  if( verbosity>0 ){
    std::cout<<"Message: "<<(std::hex)<<"Data size:"<<data.size()<<std::endl;			
    for( unsigned int i=0;i<data.size(); i++){
      std::cout<<std::setw(8)<<data[i]<<" "<<std::endl;
    }
    std::cout<<(std::dec)<<std::endl;			
//  }
  
  if(data.size()>20 && data[1]==Header1_regout && data[2]==Header2_regout){
    std::cout<<"Size and headers ok"<<std::endl;			  
  }
  //  int regs= (data.size()-6)/2;
  
  for( unsigned int i=3;i+3<data.size(); i+=2){
    uint32_t rg = data[i];
    uint32_t val = data[i+1];
    switch (rg){
    case 0:
      std::cout<<" 0 - Control reg empty: "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 1:
      std::cout<<" 1 - Control reg last FSM command: "<<val<<"   ";
      switch (val&3){
      case 0: std::cout<<" IdleToConfig"<<std::endl; break;
      case 1: std::cout<<" ConfigToRun"<<std::endl; break;
      case 2: std::cout<<" RunToConfig"<<std::endl; break;
      case 3: std::cout<<" ConfigToIdle"<<std::endl; break;
      default: std::cout<<" UNKNOWN STATE"<<std::endl; break;
      };
      break;
    case 2:
      std::cout<<" 2 - Control reg Event Builder Header: "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 3:
      std::cout<<" 3 - Control reg Busy Model: "<<val<<", "<<(val? "Busy on 1 event": "Busy on fifo almost full")<<std::endl;
      break;
    case 4:
      std::cout<<" 4 - Control reg Duration of ENDOFRUN status: "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 5:
      std::cout<<" 5 - Control reg Enable RAM usage: "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
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
      std::cout<<rg<<" - Control reg not used : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 16:
      std::cout<<"16 - Monitor reg FIRMWARE VERSION :"<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 17:
      std::cout<<"17 - Monitor reg FSM :"<<(std::hex)<<val<<(std::dec)<<std::endl;
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
      if(val&0x2000000) std::cout<<" DAQReading ";
      std::cout<<std::endl;
      break;
    case 18:
      std::cout<<"18 - Monitor reg DAQ errors : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 19:
      std::cout<<"19 - Monitor reg Trigger Counter : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 20:
      std::cout<<"20 - Monitor reg BCOCounter : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 21:
      std::cout<<"21 - Monitor reg Clock counter MSB : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 22:
      std::cout<<"22 - Monitor reg Clock counter LSB : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 23:
      std::cout<<"23 - Monitor reg Event builder FIFO status "<<(std::hex)<<val<<(std::dec)<<std::endl;
	  if(val&0x80000000) std::cout<<" EB Fifo Full, ";
	  if(val&0x40000000) std::cout<<" EB Fifo Almost Full, ";
	  if(val&0x20000000) std::cout<<" EB Fifo Empty,";
	  if(val&0x10000000) std::cout<<" Metadata Full, ";
	  if(val&0x00100000) std::cout<<" Metadata Full2, ";
	  std::cout<<std::endl;
      break;
    case 24:
      std::cout<<"24 - Monitor reg TX FIFO status "<<(std::hex)<<val<<(std::dec)<<std::endl;
	  if(val&0x80000000) std::cout<<" TX Fifo Full, ";
	  if(val&0x40000000) std::cout<<" TX Fifo Almost Full, ";
	  if(val&0x20000000) std::cout<<" TX Fifo Empty, ";
	  if(val&0x10000000) std::cout<<" Reg Fifo Full, ";
	  if(val&0x08000000) std::cout<<" Reg Almost Full, ";
	  if(val&0x04000000) std::cout<<" Reg Empty ";
	  std::cout<<std::endl;
      break;
    case 25:
      std::cout<<"25 - Monitor reg RX FIFO status "<<(std::hex)<<val<<(std::dec)<<std::endl;
	  if(val&0x80000000) std::cout<<" RX Fifo Full, ";
	  if(val&0x40000000) std::cout<<" RX Fifo Almost Full, ";
	  if(val&0x20000000) std::cout<<" RX Fifo Empty";
	  std::cout<<std::endl;
      break;
    case 26:
      std::cout<<"26 - ADC Ch0="<<(val&0xfff)<<", Ch1="<<(val>>12)<<std::endl;
      break;
    case 27:
      std::cout<<"27 - ADC Ch0="<<(val&0xfff)<<", Ch1="<<(val>>12)<<std::endl;
      break;
    case 28:
      std::cout<<"28 - ADC Ch0="<<(val&0xfff)<<", Ch1="<<(val>>12)<<std::endl;
      break;
    case 29:
      std::cout<<"29 - ADC Ch0="<<(val&0xfff)<<", Ch1="<<(val>>12)<<std::endl;
      break;
    case 30:
      std::cout<<"30 - RAM FPGA pointer "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 31:
      std::cout<<"31 - RAM  HPS pointer "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    default:
      std::cout<<rg<<" - UNKNOWN : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    }
  }
  std::cout<<"30 - RAM FPGA pointer "<<(std::hex)<<ReadReg(30)<<(std::dec)<<std::endl;
  std::cout<<"31 - RAM  HPS pointer "<<(std::hex)<<ReadReg(31)<<(std::dec)<<std::endl;

}

void FpgaInterface::printConfigRegs(){
	
  std::vector<uint32_t> data;

  checkIFifo();
  unsigned int nreg = 7; // number of registers to read
  setIFifoData(nreg+3);   // number of data to transfer
  setIFifoData(Header_ReadRegs);
  for(unsigned int i=0; i<nreg; i++)
    setIFifoData(i);
  setIFifoData(Instruction_Footer); 
//  sendReadAllRegs();
  readORegfifo(data);
  if( verbosity>0 ){
    std::cout<<"Message: "<<(std::hex)<<"Data size:"<<data.size()<<std::endl;			
    for( unsigned int i=0;i<data.size(); i++){
      std::cout<<std::setw(8)<<data[i]<<" "<<std::endl;
    }
    std::cout<<(std::dec)<<std::endl;			
  }
  
  if(data.size()>2*nreg && data[1]==Header1_regout && data[2]==Header2_regout){
    std::cout<<"Size and headers ok"<<std::endl;			  
  }
  //  int regs= (data.size()-6)/2;
  
  for( unsigned int i=3;i<nreg*2+3; i+=2){ // only first 16 regs
    uint32_t rg = data[i];
    uint32_t val = data[i+1];
    switch (rg){
    case 0:
      std::cout<<" 0 - Control reg empty: "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 1:
      std::cout<<" 1 - Control reg last FSM command: "<<val<<"    ";
      switch (val&3){
      case 0: std::cout<<" IdleToConfig"<<std::endl; break;
      case 1: std::cout<<" ConfigToRun"<<std::endl; break;
      case 2: std::cout<<" RunToConfig"<<std::endl; break;
      case 3: std::cout<<" ConfigToIdle"<<std::endl; break;
      default: std::cout<<" UNKNOWN STATE"<<std::endl; break;
      };
      break;
    case 2:
      std::cout<<" 2 - Control reg Event Builder Header: "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 3:
      std::cout<<" 3 - Control reg Configuration: "<<(std::hex)<<val<<", "
      <<" Active Streams: "<<(val&0xFF)<<"; flags: "
	  <<(((val>>8) & 1) ? "Do store" : "No storing")<<", "
	  <<(((val>>9) & 1) ? "Busy on 1 event": "Busy on fifo almost full")<<", "
	  <<(((val>>10) & 1) ? "Simulated data" : "External data")<<"."<<std::endl;
      break;
    case 4:
      std::cout<<" 4 - Control reg Pre-Trigger samples : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 5:
      std::cout<<" 5 - Control reg Post Trigger samples: "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 6:
      std::cout<<" 6 - Control reg Waiting cycles : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      std::cout<<rg<<" - Control reg not used : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    default:
      std::cout<<rg<<" - UNKNOWN : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    }
  }
}

void FpgaInterface::printMonitorRegs(){
	
  std::vector<uint32_t> data;

  checkIFifo();
  unsigned int nreg = 24; // number of registers to read
  setIFifoData(nreg+3);   // number of data to transfer
  setIFifoData(Header_ReadRegs);
  for(unsigned int i=0; i<nreg; i++)
    setIFifoData(i+16);
  setIFifoData(Instruction_Footer); 

  readORegfifo(data);
  if( verbosity>0 ){
    std::cout<<"Message: "<<(std::hex)<<"Data size:"<<data.size()<<std::endl;			
    for( unsigned int i=0;i<data.size(); i++){
      std::cout<<std::setw(8)<<data[i]<<" "<<std::endl;
    }
    std::cout<<(std::dec)<<std::endl;			
  }
  
  if(data.size()>20 && data[1]==Header1_regout && data[2]==Header2_regout){
    std::cout<<"Size and headers ok"<<std::endl;			  
  }
  //  int regs= (data.size()-6)/2;
  
  for( unsigned int i=3;i+3<data.size(); i+=2){ // only first 16 regs
    uint32_t rg = data[i];
    uint32_t val = data[i+1];
    switch (rg){
    case 16:
      std::cout<<"16 - Monitor reg FIRMWARE VERSION :"<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 17:
      std::cout<<"17 - Monitor reg FSM :"<<(std::hex)<<val<<(std::dec)<<"   ";
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
      std::cout<<std::endl;
      break;
    case 18:
      std::cout<<"18 - Monitor reg DAQ errors : "<<(std::hex)<<val;
      if(val&0x2) std::cout<<" Trigger_Lost ";
      if(val&0x4) std::cout<<" InvalidAddress ";
      if(val&0x8) std::cout<<" InOutBothActive ";
      std::cout<<std::endl;
      break;
    case 19:
      std::cout<<"19 - Monitor reg Trigger Counter : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 20:
      std::cout<<"20 - Monitor reg BCOCounter : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 21:
      std::cout<<"21 - Monitor reg Clock counter MSB : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 22:
      std::cout<<"22 - Monitor reg Clock counter LSB : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 23:
      std::cout<<"23 - Monitor reg EB+TRG FIFO status "<<(std::hex)<<val<<(std::dec)<<std::endl;
	  std::cout<<"     EB:  "<<(val&0x7ff)<<"  entries, ";
	  if(val&0x8000) std::cout<<" EB Fifo Full, ";
	  if(val&0x4000) std::cout<<" EB Fifo Almost Full. ";
	  if(val&0x2000) std::cout<<" EB Fifo Empty.";
	  std::cout<<std::endl;
      val = val>>16;
	  std::cout<<"     Trg: "<<(val&0x7ff)<<"  entries, ";
	  if(val&0x8000) std::cout<<" Trg Fifo Full, ";
	  if(val&0x4000) std::cout<<" Trg Fifo Almost Full. ";
	  if(val&0x2000) std::cout<<" Trg Fifo Empty.";
	  std::cout<<std::endl;
      break;
    case 24:
      std::cout<<"24 - Monitor reg RX+TX FIFO status "<<(std::hex)<<val<<(std::dec)<<std::endl;
	  std::cout<<"     RX: "<<(val&0xfff)<<"  entries, ";
	  if(val&0x8000) std::cout<<" RX Fifo Full, ";
	  if(val&0x4000) std::cout<<" RX Fifo Almost Full. ";
	  if(val&0x2000) std::cout<<" RX Fifo Empty.";
	  std::cout<<std::endl;
      val = val>>16;
	  std::cout<<"     TX: "<<(val&0xfff)<<"  entries, ";
	  if(val&0x8000) std::cout<<" TX Fifo Full, ";
	  if(val&0x4000) std::cout<<" TX Fifo Almost Full. ";
	  if(val&0x2000) std::cout<<" TX Fifo Empty.";
	  std::cout<<std::endl;
      break;
    case 25:
      std::cout<<"25 - Monitor EBDebug "<<(std::hex)<<val<<(std::dec)<<std::endl;
	  std::cout<<"     EB: Status: "<<(val&7)<<" Range ready:"<<((val>>3)&1)
			   <<" End of Wait:"<<((val>>4)&1)
			   <<" FifoData ready:"<<((val>>5)&1)
			   <<" Evaluate Range: "<<((val>>6)&1)<<std::endl
			   <<"     EvFifo Empty: "<<((val>>7)&1)
			   <<" ORStream Full: "<<((val>>8)&1)
			   <<" Memory Full: "<<((val>>9)&1)
			   <<" StreamFull: "<<((val>>12)&15)<<std::endl;
	  std::cout<<"     PFM: Status: "<<((val>>16)&7)<<" Eval Range:"<<((val>>19)&1)
			   <<" Min found:"<<((val>>20)&1)
			   <<" Max found:"<<((val>>21)&1)
			   <<" Mem empty: "<<((val>>22)&1)
			   <<" Mem full: "<<((val>>23)&1)<<std::endl
			   <<"     Do Read: "<<((val>>24)&1)
			   <<" No more data: "<<((val>>25)&1)
			   <<" Tail wrap: "<<((val>>26)&1)
			   <<" Read wrap: "<<((val>>27)&1)
			   <<" Write wrap: "<<((val>>28)&1)
			   <<" Err1: "<<((val>>29)&1)
			   <<" Err2: "<<((val>>30)&1)
			   <<" Err3: "<<((val>>31)&1)<<std::endl;
			   
      break;
    case 26:
      std::cout<<"26 - ADC Ch0="<<(val&0xfff)<<", Ch1="<<(val>>12)<<std::endl;
      break;
    case 27:
      std::cout<<"27 - ADC Ch0="<<(val&0xfff)<<", Ch1="<<(val>>12)<<std::endl;
      break;
    case 28:
      std::cout<<"28 - ADC Ch0="<<(val&0xfff)<<", Ch1="<<(val>>12)<<std::endl;
      break;
    case 29:
      std::cout<<"29 - ADC Ch0="<<(val&0xfff)<<", Ch1="<<(val>>12)<<std::endl;
      break;
	case 30:
      std::cout<<"30 - EVT Header="<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
	case 31:
      std::cout<<"31 - EVT Time stamp="<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
	case 32:
      std::cout<<"32 - EVT Trg accepted="<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
	case 33:
      std::cout<<"33 - EVT Trg received="<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
	case 34:
      std::cout<<"34 - EVT clock ticks MSB="<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
	case 35:
      std::cout<<"35 - EVT clock ticks LSB="<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
	case 36:
      std::cout<<"36 - EVT DDR3 Tail="<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
	case 37:
      std::cout<<"37 - EVT DDR3 Head="<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 38:
      std::cout<<"38 - RAM DDR3 Tail pointer "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    case 39:
      std::cout<<"39 - RAM DDR3 Head pointer "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    default:
      std::cout<<rg<<" - UNKNOWN : "<<(std::hex)<<val<<(std::dec)<<std::endl;
      break;
    }
  }
  std::cout<<std::endl;
}


// high level function to read all registers
void FpgaInterface::ReadAllRegs(){
  std::vector<uint32_t> data;
  cleanUpOFifo();
  sendReadAllRegs();
  usleep(1000);
  checkIFifo();
  readORegfifo(data);

  if(data.size()>20 && data[1]==Header1_regout && data[2]==Header2_regout){
    for( unsigned int i=3;i<data.size()-3; i+=2){
      int rg = data[i];
      regs[rg] = data[i+1];
    }
  } else {
    std::cout<<"Error reading all registers"<<std::endl;
  }
  
}

// high level function to read a given register
uint32_t FpgaInterface::ReadReg(uint32_t reg){
  std::vector<uint32_t> data;
  cleanUpORegFifo();
  sendReadReg(reg);
  usleep(1000);
  checkIFifo();
  readORegfifo(data);
  if( verbosity>0 ){
    std::cout<<"Reg "<<reg<<", data size = "<<data.size()<<std::setw(9)<<(std::hex)<<std::endl;
    for(unsigned int i=0; i<data.size(); ++i) { std::cout<<std::setw(9)<<data[i];}
    std::cout<<(std::dec)<<std::endl;	
  }
  
  
  if(data.size()>4 && data[1]==Header1_regout && data[2]==Header2_regout && data[3]==(uint32_t)reg){
    regs[reg]= data[4];
  } else {
    std::cout<<"Error reading reg "<<reg<<std::endl;
    std::cout<<"Reg "<<reg<<", data size = "<<data.size()<<std::setw(9)<<(std::hex)<<std::endl;
    for(unsigned int i=0; i<data.size(); ++i) { std::cout<<std::setw(9)<<data[i];}
    std::cout<<(std::dec)<<std::endl;
	std::cout<<"Retrying...."<<std::endl;
    cleanUpORegFifo();
    usleep(1000);
    sendReadReg(reg);
    usleep(1000);
    checkIFifo();
    readORegfifo(data);
    if(data.size()>4 && data[1]==Header1_regout && data[2]==Header2_regout && data[3]==(uint32_t)reg){
      regs[reg]= data[4];
    } else {
      std::cout<<"Error reading reg "<<reg<<std::endl;
      std::cout<<"Reg "<<reg<<", data size = "<<data.size()<<std::setw(9)<<(std::hex)<<std::endl;
      for(unsigned int i=0; i<data.size(); ++i) { std::cout<<std::setw(9)<<data[i];}
      std::cout<<(std::dec)<<std::endl;
	  std::cout<<"No more trys...."<<std::endl;	
      regs[reg]=0xffffffff;
	}
  }
  return regs[reg];
}

EVHeader & FpgaInterface::readCurrentEvent(){
  // sending request
  cleanUpORegFifo();
  usleep(1000);
  checkIFifo();
  unsigned int nreg = 8; // number of registers to read
  setIFifoData(nreg+3);   // number of data to transfer
  setIFifoData(Header_ReadRegs);
  for(unsigned int i=0; i<nreg; i++)
    setIFifoData(i+14+16);
  setIFifoData(Instruction_Footer); 
  usleep(1000);
  // reading data in return
  std::vector<uint32_t> data;
  readORegfifo(data);
  if(data.size()>4 && data[1]==Header1_regout && data[2]==Header2_regout){
    currEvent.header = data[4];
    currEvent.timeStamp = data[6];
    currEvent.acceptedTriggers = data[8];
    currEvent.receivedTriggers = data[10];
    currEvent.MSBClockCounter = data[12];
    currEvent.LSBClockCounter = data[14];
    currEvent.DDR3PointerTail = data[16];
    currEvent.DDR3PointerHead = data[18];
  } else {
    currEvent.header = 0;
    currEvent.timeStamp = 0;
    currEvent.acceptedTriggers = 0;
    currEvent.receivedTriggers = 0;
    currEvent.MSBClockCounter = 0;
    currEvent.LSBClockCounter = 0;
    currEvent.DDR3PointerTail = 0;
    currEvent.DDR3PointerHead = 0;
	  
  }
  return currEvent;
}

//----------------------------------------------------------------------------------------------------------┐
// high level function to read a specified channel of the on board ADC
uint16_t FpgaInterface::read_ADC_channel(uint8_t ch_index) {

  uint8_t realtive_reg_offset = 0b11111111;

  // Convert the channel index to the real register offset value
  switch (ch_index) {
    case 0: realtive_reg_offset = MON_REGS_OFFSET + ADC_CH0_reg; break;
    case 1: realtive_reg_offset = MON_REGS_OFFSET + ADC_CH1_reg; break;
    case 2: realtive_reg_offset = MON_REGS_OFFSET + ADC_CH2_reg; break;
    case 3: realtive_reg_offset = MON_REGS_OFFSET + ADC_CH3_reg; break;
    case 4: realtive_reg_offset = MON_REGS_OFFSET + ADC_CH4_reg; break;
    case 5: realtive_reg_offset = MON_REGS_OFFSET + ADC_CH5_reg; break;
    case 6: realtive_reg_offset = MON_REGS_OFFSET + ADC_CH6_reg; break;
    case 7: realtive_reg_offset = MON_REGS_OFFSET + ADC_CH7_reg; break;
    default: return 0xffff; break;// 0xffff is an impossible value as the ADC is 12 bit. This means there's an error
  }

  // Every register contains two values, so two different ways to read
  if (ch_index%2 == 0) {
    return ( (ReadReg(realtive_reg_offset)) & 0b111111111111 );
  }
  else {
    return ( ( (ReadReg(realtive_reg_offset))>>12 ) & 0b111111111111 );
  }

}
//----------------------------------------------------------------------------------------------------------┘