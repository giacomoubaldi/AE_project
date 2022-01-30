/************************************************************************/
/*									*/
/* This is a simple prototyping program for the CAEN TDC module	1190	*/
/*									*/
/* Date:   12/12/2012    						*/
/* Author: C. Sbarra	       					*/
/*									*/
/**************** C 2006 - A nickel program worth a dime ****************/
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <iomanip>
#include <vector>
#include <math.h>
#include "rcc_error/rcc_error.h"
#include "vme_rcc/vme_rcc.h"
#include "cmem_rcc/cmem_rcc.h"
#include "DFDebug/DFDebug.h"
#include "ROSGetInput/get_input.h"
#include "rcd_v1190/v1190.h"
#include "rcc_time_stamp/tstamp.h"


/**************/
/* Prototypes */
/**************/

void readConfigurationROM();
void decodeControlRegister();
void decodeStatusRegister();
void setECLoutput();
void setContinuousStorageMode();
void setTriggerMatchingMode();
void enableHeaderAndTrailer();
void disableHeaderAndTrailer();
void enableTestMode();
void disableTestMode();
unsigned short buildOPCODE(unsigned short command, unsigned short data);
bool microReady4Write();
bool microReady2Read();
void setMaxHitsInEvent(unsigned short opt);
void readOneEventfromOutputBuffer();
void bltAcquisition(int* handle, int address);
void acquisitionModeMenu();
void triggerSettingsMenu();
void channelMenu();
void debugAndTestMenu();
void TDCReadoutMenu();
void EdgeDetectionAndResolutionMenu();
void FIFOMenu();


using namespace std;

#define FILLER 0xc0000000
#define MAXEVENTSFIFO 5

volatile v1190_regs_t *v1190;
unsigned long m_cmem_desc_paddr;
unsigned long m_cmem_desc_uaddr;
std::vector<unsigned int>m_wordCountFIFO;
unsigned int* m_ptrFIFO, *m_ptrEndFIFO, *m_rptr;

/*****************************/
int main(int argc, char **argv)
/*****************************/
{
  unsigned int ret = ts_open(1, TS_DUMMY);       //Open the library that we will use to measure time inervals
  if (ret)
  {
    rcc_error_print(stdout, ret);   //This is a function of the ATLAS rcc_error package. 
    exit(-2);
  }

  // first of all, open the library that handles memory allocation to
  // prepare for VME block transfer

  ret = CMEM_Open();                
  if (ret) 
  {
    rcc_error_print(stdout, ret);
    exit(-3);
  } 

 //The next few functions allocate memory that we will only need 
  //when we do block transfer.
  //Therefore we could move these instructions down. However, it is 
  //good practice to perform (potentially) time consuming functions 
  //in the initialization part of a program and keep them out of the main 
  //part of the code. This way S/W delays cannot interfere with data taking

  //Get a contiguous buffer for block write transfers
  //0x100000 is the size in bytes (actually more than we need but memory is cheap....)
  //"DMA_BUFFER" is a random string (but it should be unique)
  //cmem_desc_out is a descriptor that we will need for other CMEM functions


 //Get a buffer for block read transfers
  int cmem_desc_in;
  char name[]= "DMA_BUFFER2";
  ret = CMEM_SegmentAllocate(0x20000, name, &cmem_desc_in);     
  if (ret) 
  {
    rcc_error_print(stdout, ret);
    exit(-4);
  }

  ret = CMEM_SegmentPhysicalAddress(cmem_desc_in, &m_cmem_desc_paddr);
  if (ret) 
  {
    rcc_error_print(stdout, ret);
    exit(-4);
  }

  ret = CMEM_SegmentVirtualAddress(cmem_desc_in, &m_cmem_desc_uaddr);
  if (ret) 
  {
    rcc_error_print(stdout, ret);
    exit(-4);
  }


  m_wordCountFIFO.reserve( MAXEVENTSFIFO );
  if( m_wordCountFIFO.size() == 0) {
    for(unsigned int i = 0; i < MAXEVENTSFIFO; i++)
      m_wordCountFIFO.push_back(0);
    } else { 
    for(unsigned int i=0; i < MAXEVENTSFIFO; i++)
      m_wordCountFIFO[i] = 0;
  }
  m_ptrFIFO = m_wordCountFIFO.data();




  
  int handle;
  unsigned long value;
  u_int havebase = 0, v1190_base = 0xe00000, fun = 1;
  char c;
  static VME_MasterMap_t master_map = {0xe00000, 0x10000, VME_A32, 0};
  while ((c = getopt(argc, argv, "db:")) != -1)
    {
      switch (c) 
	{
	case 'b':	
	  {
	    v1190_base  = strtol(optarg, 0, 16); 
	    havebase = 1;
	  }
	  break;
	default:
	  printf("Invalid option %c\n", c);
	  printf("Usage: %s  [options]: \n", argv[0]);
	  printf("Valid options are ...\n");
	  printf("-b <VME A32 base address>: The hexadecimal A32 base address of the v1190\n");
	  printf("-d:                        Dump registers and exit\n");
	  printf("\n");
	  return -1;
	}
    }  
  if (!havebase)
    {
      printf("Enter the VMEbus A32 base address of the v1190\n");
      v1190_base = gethexd(v1190_base);
    }
  master_map.vmebus_address = v1190_base;
  ret = VME_Open();
  if (ret != VME_SUCCESS)
    {
      VME_ErrorPrint(ret);
      return -1;
    }
  ret = VME_MasterMap(&master_map, &handle);
  if (ret != VME_SUCCESS)
    {
      VME_ErrorPrint(ret);
      return -1;
    }
  ret = VME_MasterMapVirtualLongAddress(handle, &value);
  if (ret != VME_SUCCESS)
    {
      VME_ErrorPrint(ret);
      return -1;
    }

  v1190 = (v1190_regs_t *) value;  


  while (fun != 0){
    printf("\n");
    printf("MAIN MENU\n");
    printf("Select an option:\n"); 
    printf("   1  Read configuration ROM\n");
    printf("   2  GetFirmwareVersion\n");
    printf("   3  decodeControlRegister\n");
    printf("   4  decodeStatusRegister\n");
    printf("   5  Reset Module\n");
    printf("   6  Clear Module\n");
    printf("   7  SW event reset\n");
    printf("  10  Set AlmostFullLevel of Output Buffer\n");
    printf("  11  Acquisition Mode Menu\n");
    printf("  12  TriggerSetting Menu\n");
    printf("  13  TDC Readout menu\n");
    printf("  14  Channel menu\n");
    printf("  15  Debug-and-Test menu\n");
    printf("  16  FIFO menu\n");
    printf("  17  Edge detection menu\n");
    printf("  18  Read Output Buffer\n");
    printf("  19  Block Transfer Acquisition\n");
    printf("Your choice ");
    fun = getdecd(fun);
    if (fun == 1) readConfigurationROM();
    if (fun == 2) cout <<" Firmware Revision is 0x"<<HEX(v1190->firmwareRevision)<<endl; 
    if (fun == 3) decodeControlRegister();
    if (fun == 4) decodeStatusRegister();
    if (fun == 5){
      v1190->moduleReset = 0;
      usleep(20000);}
    if (fun == 6){
      v1190->softwareClear = 0;
      usleep(20000);}
    if (fun == 7){
      v1190->softwareEventReset = 0;
      usleep(20000);}
    if (fun == 10){
      cout<<"Enter minimum # of words in output buffer to decide it is almost full (default 64)"<<endl;
      unsigned short value = 0;
      value = getdecd(value);
      v1190->almostFullLevel = value;
      usleep(20000);
    }
    if (fun == 11)
      acquisitionModeMenu();
    if (fun == 12)
      triggerSettingsMenu();
    if (fun == 13)
      TDCReadoutMenu();
    if (fun == 14)
      channelMenu();
    if (fun == 15)
      debugAndTestMenu();
    if (fun == 16)
      FIFOMenu();
    if (fun == 17)
      EdgeDetectionAndResolutionMenu();
    if (fun == 18)
      readOneEventfromOutputBuffer();
    if (fun == 19)
      bltAcquisition(&handle, v1190_base);
  }      
  ret = VME_MasterUnmap(handle);
  if (ret != VME_SUCCESS)
    VME_ErrorPrint(ret);  
  ret = VME_Close();
  if (ret != VME_SUCCESS)
    VME_ErrorPrint(ret);

  ret = CMEM_SegmentFree(cmem_desc_in);    //Return the first buffer
  if (ret) 
    rcc_error_print(stdout, ret);
  
//  ret = CMEM_SegmentFree(cmem_desc_out);   //Return the second buffer
//  if (ret) 
//    rcc_error_print(stdout, ret);
//

  ret = CMEM_Close();                      //Close the memory allocation library
  if (ret)  
    rcc_error_print(stdout, ret);
     
  ret = ts_close(TS_DUMMY);                //Close the time stamping library  
  if (ret)
    rcc_error_print(stdout, ret);

  return 0;
}

/****************************************/
void EdgeDetectionAndResolutionMenu()
/****************************************/
{
  int fun = 1;
  while (fun != 0){
    cout<<""<<endl;
    printf("   1  Read Edge detection configuration\n");
    printf("   2  Set Edge Detection configuration\n");
    printf("   3  Set LSB of leding/trailing edge\n");
    printf("   4  Set channel dead time between hits\n");
    printf("   5  Read channel dead time\n");

    fun = getdecd(fun);

    if (fun == 1){
      unsigned short opCode = buildOPCODE(0x23,0);
      v1190->micro = opCode;
      while (!microReady2Read()){
	usleep(1); 
      }
      unsigned short value = v1190->micro;
      switch (value)
	{
	case 0 : 
	  cout<<"Edge detection set to pair mode"<< endl;  
	  break;
	case 1 :
	  cout<<"Detecting trailing edge only"<< endl;  
	  break;
	case 2 :
	  cout<<"Detecting leading edge only"<< endl;  
	  break;
	case 3 :
	  cout<<"Detecting both leading and trailing edges"<< endl;  
	}
    }

    if (fun == 2){
      cout<<"Enter 0 for pair mode, 1 for trailing, 2 for leading, 3 for both leading and trailing"<<endl;
      unsigned short opt = 0;
      opt = getdecd(opt);
      unsigned short opCode = buildOPCODE(0x22,0);
      v1190->micro = opCode;
      while (!microReady4Write()){
	usleep(1); 
      }
      v1190->micro = opt;  
    }

    if (fun == 3){
      cout<<"Enter 0 for 800 ps, 1 for 200 ps, 2 for 100 ps"<<endl;
      unsigned short opt = 0;
      opt = getdecd(opt);
      unsigned short opCode = buildOPCODE(0x24,0);
      v1190->micro = opCode;
      while (!microReady4Write()){
	usleep(1); 
      }
      v1190->micro = opt;  
    }

    if (fun == 4){
      cout<<"Enter 0 for 5 ns, 1 for 10 ns, 2 for 30 ns, 3 for 100 ns"<<endl;
      unsigned short opt = 0;
      opt = getdecd(opt);
      unsigned short opCode = buildOPCODE(0x28,0);
      v1190->micro = opCode;
      while (!microReady4Write()){
	usleep(1); 
      }
      v1190->micro = opt;  
    }

    if (fun == 5){
      unsigned short opCode = buildOPCODE(0x29,0);
      v1190->micro = opCode;
      while (!microReady2Read()){
	usleep(1); 
      }
      unsigned short value = v1190->micro;
      switch (value)
	{
	case 0 : 
	  cout<<"Dead tiem between hits is about 5 ns"<< endl;  
	  break;
	case 1 :
	  cout<<"Dead tiem between hits is about 10 ns"<< endl;  
	  break;
	case 2 :
	  cout<<"Dead tiem between hits is about 30 ns"<< endl;  
	  break;
	case 3 :
	  cout<<"Dead tiem between hits is about 100 ns"<< endl;  
	}
    }
  }  
}
/****************************************/
void FIFOMenu()
/****************************************/
{
  
  int fun = 1;
  while (fun != 0){
    cout<<""<<endl;
    printf("   1  Enable Test FIFO (output buffer test mode enabled)\n");
    printf("   2  Disable Test FIFO (output buffer test mode disabled)\n");
    printf("   3  Check if Test FIFO is enabled\n");
    printf("   4  Enable EVENT FIFO \n");
    printf("   5  Disable EVENT FIFO \n");
    printf("   6  Check if EVENT FIFO is enabled\n");
    printf("   7  Read EVENT FIFO\n");
    printf("   8  Get # of words in Event FIFO (should be same as # of events in output buffer\n");
    printf("   9  Get status of EVENT FIFO\n");
    printf("  10  Set Effective size of READOUT FIFO EVENT\n");
    printf("  11  Read effective size of READOUT FIFO (DEFAULT IS 7 == 256)\n");

    fun = getdecd(fun);
    if (fun == 1){
      unsigned short status =  v1190->controlRegister;
      status |= 0x40;
      v1190->controlRegister = status;
    }
    if (fun == 2){
      unsigned short status =  v1190->controlRegister;
      unsigned int fifo_enabled = 0x40;
      status &= ~fifo_enabled;
      v1190->controlRegister = status;
    }
    if (fun == 3){
      unsigned short status =  v1190->controlRegister;
      cout <<" TEST FIFO is";
      ((status & 0x40)>>6) ? cout << "  ENABLED "<<endl : cout << "  DISABLED "<<endl;
    }

    if (fun == 4){
      unsigned short status =  v1190->controlRegister;
      status |= 0x100;
      v1190->controlRegister = status;
    }
    if (fun == 5){
      unsigned short status =  v1190->controlRegister;
      unsigned int fifo_enabled = 0x100;
      status &= ~fifo_enabled;
      v1190->controlRegister = status;
    }
    if (fun == 6){
      unsigned short status =  v1190->controlRegister;
      cout <<" EVENT FIFO is";
      ((status & 0x100)>>6) ? cout << "  ENABLED "<<endl : cout << "  DISABLED "<<endl;
    }

    if (fun == 7){
      unsigned int value = v1190->eventFIFO;
      unsigned short ev = (value&0xffff0000)>>16;
      unsigned short nw = value&0xffff;
      cout<<" Event number "<< ev << " contains "<< nw <<" words "<<endl; 
    }
    if (fun == 8)
      cout<<"Number of words in event FIFO "<<v1190->eventFIFOStored<<endl;
    if (fun == 9){
      (v1190->eventFIFOStatus & 1) ? cout<<"EV FIFO data ready"<<endl : cout<<"No events in EV FIFO"<<endl;
      ((v1190->eventFIFOStatus>>1) & 1) ? cout<<"EV FIFO full"<<endl : cout<<"EV FIFO not full"<<endl;
    }
    if (fun == 10){
      cout<<" Select effective FIFO size: "<<endl;
      cout<<" 000 --> 2 words "<<endl;
      cout<<" 001 --> 4 words"<<endl;
      cout<<" 010 --> 8 words"<<endl;
      cout<<" 011 --> 16 words"<<endl;
      cout<<" 100 --> 32 words"<<endl;
      cout<<" 101 --> 64 words"<<endl;
      cout<<" 110 --> 128 words"<<endl;
      cout<<" 110 --> 256 words(default)"<<endl;
      unsigned short m = 0;
      m = gethexd(m);
      m &= 7;
      cout<<" Got value 0x"<<HEX(m);
      unsigned short opCode = buildOPCODE(0x3b,0);
      v1190->micro = opCode;
      while (!microReady4Write()){
	usleep(1); 
      }
      v1190->micro = m;  
    }
    if (fun == 11){
      unsigned short opCode = buildOPCODE(0x3c,0);
      v1190->micro = opCode;
      while (!microReady2Read()){
	usleep(1); 
      }
      unsigned short value = v1190->micro;
      value &= 7;
      cout<<" Effective size of readout FIFO is 0x"<<HEX(value)<< endl;  

    }

  }
}
/****************************************/
void acquisitionModeMenu()
/****************************************/
{
  int fun = 1;
  while (fun != 0){
    printf("\n");
    printf("ACQUISITION MODE MENU\n");
    printf("Select an option:\n");
    printf("   0 back to main menu\n");
    printf("   1 Set Trigger Matching Mode\n");
    printf("   2 Set Continuous Storage Mode\n");
    printf("   3 load default configuration\n");
    printf("   4 save user configuration\n");
    printf("   5 load user configuration\n");
    printf("Your choice ");
    fun = getdecd(fun);
    if (fun == 1){
      unsigned short opCode = buildOPCODE(0,0);
      v1190->micro = opCode;
    }
    else if (fun == 2){
      unsigned short opCode = buildOPCODE(1,0);
      v1190->micro = opCode;
    }
    else if (fun == 3){
      unsigned short opCode = buildOPCODE(5,0);
      v1190->micro = opCode;
    }
    else if (fun == 4){
      unsigned short opCode = buildOPCODE(6,0);
      v1190->micro = opCode;
    }
    else if (fun == 5){
      unsigned short opCode = buildOPCODE(7,0);
      v1190->micro = opCode;
    }
  }
}
/****************************************/
void channelMenu()
/****************************************/
{
  int fun = 1;
  while (fun != 0){
    printf("\n");
    printf("CHANNEL MENU\n");
    printf("Select an option:\n");
    printf("   0 back to main menu\n");
    printf("   1 enable all channels\n");
    printf("   2 disable all channels\n");
    printf("   3 enable one channels\n");
    printf("   4 disable one channels\n");
    printf("   5 write enable pattern\n");
    printf("   6 read enable pattern\n");
    printf("   7 write enable pattern 32\n");
    printf("   8 read enable pattern 32\n");
    printf("Your choice ");
    fun = getdecd(fun);
    if( fun == 1){
      unsigned short opCode = buildOPCODE(0x42,0);
      v1190->micro = opCode;
    }
    if( fun == 2){
      unsigned short opCode = buildOPCODE(0x42,0);
      v1190->micro = opCode;
    }
    if( fun == 3){
      cout<<" enter channel number "<<endl;
      unsigned short ch = 0;
      ch = getdecd(ch);
      unsigned short opCode = buildOPCODE(0x40,ch);
      v1190->micro = opCode;
    }
    if( fun == 4){
      cout<<" enter channel number "<<endl;
      unsigned short ch = 0;
      ch = getdecd(ch);
      unsigned short opCode = buildOPCODE(0x41,ch);
      v1190->micro = opCode;
    }
    if( fun == 5){
      cout<<" enter first channel mask (ch 0-15): 0x"<<endl;
      unsigned short m1 = 0;
      m1 = gethexd(m1);
      cout<<" Got value 0x"<<HEX(m1);

      cout<<" enter second channel mask (ch 0-15): 0x "<<endl;
      unsigned short m2 = 0;
      m2 = gethexd(m2);
      cout<<" Got value 0x"<<HEX(m2);

      cout<<" enter third channel mask (ch 0-15): 0x "<<endl;
      unsigned short m3 = 0;
      m3 = gethexd(m3);
      cout<<" Got value 0x"<<HEX(m3);

      cout<<" enter fourth channel mask (ch 0-15); 0x "<<endl;
      unsigned short m4 = 0;
      m4 = gethexd(m4);
      cout<<" Got value 0x"<<HEX(m4);

      unsigned short opCode = buildOPCODE(0x44,0);
      v1190->micro = opCode;

      while (!microReady4Write()){
	usleep(1); 
      }
      v1190->micro = m1;  
      while (!microReady4Write()){
	usleep(1); 
      }
      v1190->micro = m2;  
      while (!microReady4Write()){
	usleep(1); 
      }
      v1190->micro = m3;  
      while (!microReady4Write()){
	usleep(1); 
      }
      v1190->micro = m4;  
    }

    if( fun == 6){

      unsigned short opCode = buildOPCODE(0x45,0);
      v1190->micro = opCode;

      while (!microReady2Read()){
	usleep(1); 
      }
      unsigned short value = v1190->micro;
      cout<<" First word reads 0x"<<HEX(value)<< endl;  

      while (!microReady2Read()){
	usleep(1); 
      }
      value = v1190->micro;
      cout<<" Second word reads 0x"<<HEX(value)<< endl;  

      while (!microReady2Read()){
	usleep(1); 
      }
      value = v1190->micro;
      cout<<" Third word reads 0x"<<HEX(value)<< endl;  

      while (!microReady2Read()){
	usleep(1); 
      }
      value = v1190->micro;
      cout<<" Fourth word reads 0x"<<HEX(value)<< endl;  
    }
    if( fun == 7){
      cout<<" Select TDC number "<<endl;
      unsigned short TDC = 0;
      TDC = getdecd(TDC);
      cout<<" Enter Bit 0-15 mask "<<endl;
      unsigned short m1 = 0;
      m1 = gethexd(m1);
      cout<<" Got value 0x"<<HEX(m1);
      cout<<" Enter Bit 16-31 mask "<<endl;
      unsigned short m2 = 0;
      m2 = gethexd(m2);
      cout<<" Got value 0x"<<HEX(m2);

      unsigned short opCode = buildOPCODE(0x46,TDC);
      v1190->micro = opCode;
      while (!microReady4Write()){
	usleep(1); 
      }
      v1190->micro = m1;  

      while (!microReady4Write()){
	usleep(1); 
      }
      v1190->micro = m2;  

    }
    if( fun == 8){
      cout<<" Select TDC number "<<endl;
      unsigned short TDC = 0;
      TDC = getdecd(TDC);
      unsigned short opCode = buildOPCODE(0x47,TDC);
      v1190->micro = opCode;
      while (!microReady2Read()){
	usleep(1); 
      }
      unsigned short val1 = v1190->micro;
      while (!microReady2Read()){
	usleep(1); 
      }
      unsigned short val2 = v1190->micro;
      unsigned int mask = (val2<<16)| val1;
      cout<<"Enabled channel mask of TDC "<< TDC <<" is 0x"<<HEX(mask)<< endl;  

    }
  }
}

/****************************************/
void bltAcquisition( int *handle, int address )
/****************************************/
{
  int* tmp_handle = handle;
  u_int timeout = 0;
  //int fsize = 0;
  volatile u_short value;

  //Wait until there is an event ready
  value = v1190->statusRegister;  
 
  cout << "DataChannelV1190::getNextFragmentBLT status register = " << value << endl;
  while (!(value & 0x1))
  {
    timeout++;
    if (timeout == 1000000)   //with timeout = 1000000 we will wait for about 1 s
    {
      cout << "DataChannelV1190::getNextFragmentBLT: Time out when waiting for DREADY" << endl;
      //prepare an empty event
      cout << "TIMEOUT = " << timeout << endl;
      return;
    }
    
    value = v1190->statusRegister;
    if( (timeout%100000)==0 )
      cout << "DataChannelV1190::getNextFragmentBLT status register = "<< value << endl;
  }
  
  if ( *m_ptrFIFO == 0 ){
  
    unsigned int eventsInFIFO = ( v1190->eventFIFOStored ) & 0x7ff;
    unsigned int eventsToTransfer = 0;
    unsigned int ret;
    unsigned int idata;

    if (eventsInFIFO < MAXEVENTSFIFO )
      eventsToTransfer = eventsInFIFO;
    else
      eventsToTransfer = MAXEVENTSFIFO;

    for( unsigned int i=0; i < MAXEVENTSFIFO; i++ )
	m_wordCountFIFO[i] = 0;

    for ( unsigned int i = 0; i < eventsToTransfer; i++ ){
      ret = VME_ReadSafeUInt( *tmp_handle, EVENTFIFO, &idata );
      if ( ret != VME_SUCCESS )
	cout << "Error in DataChannelV1190::getNextFragmentBLT: VME_ReadSafeUInt error" << endl;
      m_wordCountFIFO.push_back( idata & 0xffff );
    }

    m_ptrFIFO = m_wordCountFIFO.data();
    m_ptrEndFIFO = &m_wordCountFIFO[ MAXEVENTSFIFO ]; //one after the end of vector
    int sumWordsFIFO = 0;
    for ( unsigned int i = 0; i < m_wordCountFIFO.size(); ++i )
      sumWordsFIFO += m_wordCountFIFO.at(i);

    //ERS_LOG("VME Address:"<<HEX(m_vmeaddr));

    //ERS_LOG("PCI ADDRESS: "<<HEX(pciAddress));

    //v1190->BLTeventNumber = eventsToTransfer;

    VME_BlockTransferList_t rlist;
    rlist.number_of_items = 1;
    rlist.list_of_items[0].vmebus_address = address;
    rlist.list_of_items[0].system_iobus_address = m_cmem_desc_paddr;
    rlist.list_of_items[0].size_requested = sumWordsFIFO * 4;
    rlist.list_of_items[0].control_word = VME_FIFO_DMA_D32R;
    rlist.list_of_items[0].size_remaining = 0;
    rlist.list_of_items[0].status_word = 0;
    //rlist.list_of_items[0].control_word = VME_DMA_D32R;
    m_rptr = ( unsigned int* ) m_cmem_desc_uaddr;

    *m_rptr = 0x8000dead;

    cout << "Before: Size req:"<<rlist.list_of_items[0].size_requested
	    <<" iobus address: "<<m_cmem_desc_paddr
	    <<" Control word: "<<rlist.list_of_items[0].control_word
	    <<" size_remaining: "<<rlist.list_of_items[0].size_remaining
	 <<" status word: "<<rlist.list_of_items[0].status_word << endl;
    
    ret = VME_BlockTransfer(&rlist, 10000);
    if (ret != VME_SUCCESS)
      cout << "Error in DataChannelV1190::getNextFragmentBLT: VME_BlockTransfer error  " << ret << endl;
    if (ret != VME_SUCCESS) {
      VME_ErrorPrint(ret);  
      VME_ErrorCode_t vmestatus;
      char error_string[VME_MAXSTRING];
      if(!VME_BlockTransferStatus(&rlist, 0, &vmestatus)) 
	// check status of block transfer 
	VME_ErrorString(vmestatus, error_string);
      cout << error_string << endl;
    }

    cout << "After: Size req:"<<rlist.list_of_items[0].size_requested
	    <<" iobus address: "<<m_cmem_desc_paddr
	    <<" Control word: "<<rlist.list_of_items[0].control_word
	    <<" size_remaining: "<<rlist.list_of_items[0].size_remaining
	 <<" status word: "<<rlist.list_of_items[0].status_word << endl;

    //valutare se azzerare sumWordsFIFO
    sumWordsFIFO = 0;
    m_rptr = ( u_int* ) m_cmem_desc_uaddr;

  }
    
 
  //Return the raw data from the V1190, until a global trailer is found

  bool goon = true;
  u_int data;
  //  unsigned int measurementCounter = 0;
  unsigned int checkCounter = 0;
 
  while(goon) {

    data = *m_rptr;
    unsigned int buf = (data>>27) & 0x1f;
    cout << "v1190DataChannel : 0x"<<HEX(data) << endl;

    if(buf == 16){  //Global trailer (10000) - stop reading 
      goon = false;
    } else if( buf == 4 ){  //TDC error

      //      unsigned int val = data & 0x7fff;
      //      unsigned int val2 = ( data >> 24 ) & 0x3;
      //std::cout << "TDC ERROR on TDC number " << val2 << "with error flags " << val << std::endl;
      goon = false;

      //}  else if( buf == 8 ){
      //std::cout<<"--GLOBAL HEADER."<<std::endl;
      //}

    } else if (buf == 0){

      cout<<"-- TDC DATA = "<< data << endl;
    }
    m_rptr++;
    checkCounter++;
  }

  
  //TMP check if number of words read by while loop is the same of that contained in vector m_wordCountFIFO
  if ( *m_ptrFIFO == checkCounter )
    cout << "NUMBER OF WORDS IN m_ptrFIFO is the same of checkCounter " << checkCounter << endl;
  else cout << "NUMBER OF WORDS IN m_ptrFIFO IS NOT THE SAME OF CHECKCOUNTER " << *m_ptrFIFO << "   " << checkCounter << endl;
    

  m_ptrFIFO++;
  if ( m_ptrFIFO == m_ptrEndFIFO ){
    cout << "REACHED END OF m_ptrFIFO, NOW SET TO ZERO" << endl;
    m_ptrFIFO = m_wordCountFIFO.data();
    *m_ptrFIFO = 0;
  }
}



/****************************************/
void triggerSettingsMenu()
/****************************************/
{
  int fun = 1;
  while (fun != 0){
    printf("\n");
    printf("CHANNEL MENU\n");
    printf("Select an option:\n");
    printf("   0 back to main menu\n");
    printf("   1 set trigger window width\n");
    printf("   2 set absolute value of window offset\n");
    printf("   3 set extra search margin \n");
    printf("   4 set reject margin\n");
    printf("   5 Enable subtraction of trigger time\n");
    printf("   6 Disable subtraction of trigger time\n");
    printf("   7 Read trigger configuration\n");
    printf("Your choice ");
    fun = getdecd(fun);
    if (fun == 1){
      cout<<" Enter trigger window width: 0x "<< endl;
      short width = 0;
      width = gethexd(width);
      cout<<" will write 0x"<<HEX(width)<<" into micro register"<<endl;
      unsigned short opCode = buildOPCODE(0x10,0);
      v1190->micro = opCode;
      while (!microReady4Write()){
	usleep(1); 
      }
      v1190->micro = width;   
    }
    if (fun == 2){
      cout<<" Enter absolute value of trigger window offset 0x"<< endl;
      short offset = 0;
      offset = gethexd(offset);
      cout<<" will write 0x"<<HEX(offset)<<" into micro register"<<endl;
      unsigned short opCode = buildOPCODE(0x11,0);
      v1190->micro = opCode;
      while (!microReady4Write()){
	usleep(1); 
      }
      v1190->micro = offset;   
    }
    if (fun == 3){
      cout<<" Enter extra search margin: 0x"<< endl;
      short margin = 0;
      margin = gethexd(margin);
      cout<<" will write 0x"<<HEX(margin)<<" into micro register"<<endl;
      unsigned short opCode = buildOPCODE(0x12,0);
      v1190->micro = opCode;
      while (!microReady4Write()){
	usleep(1); 
      }
      v1190->micro = margin;   
    }
    if (fun == 4){
      cout<<" Enter reject margin (minimum 1 clock cycle=25 ns): 0x"<< endl;
      short margin = 0;
      margin = gethexd(margin);
      cout<<" will write 0x"<<HEX(margin)<<" into micro register"<<endl;
      unsigned short opCode = buildOPCODE(0x13,0);
      v1190->micro = opCode;
      while (!microReady4Write()){
	usleep(1); 
      }
      v1190->micro = margin;   
    }
    if (fun == 5){
      unsigned short opCode = buildOPCODE(0x14,0);
      v1190->micro = opCode;
    }
    if (fun == 6){
      unsigned short opCode = buildOPCODE(0x15,0);
      v1190->micro = opCode;
    }
    if (fun == 7){
      unsigned short opCode = buildOPCODE(0x16,0);
      v1190->micro = opCode;

      unsigned short value = 0; 
      // 1
      while (!microReady2Read()){
	usleep(1); 
      }
      value = v1190->micro;
      value &= 0xfff;
      cout<<"(0x"<<HEX(value)<<") - ";
      unsigned res = value*25;
      cout<<"Trigger window width is "<<res<<" ns"<<endl;
      // 2
      while (!microReady2Read()){
	usleep(1); 
      }
      value = v1190->micro;
      cout<<"(0x"<<HEX(value)<<") - ";
      short data = value;
      bool sign = data & 0xF000;
      sign ? cout<<"offset sign is negative"<<endl : cout<<"offset sign is positive"<<endl;
      res = (0xfff -(data&0xfff))*25; //this is the absolute value
      cout<<"Trigger window offset absolute value is "<<res<<" ns"<<endl;
      // 3
      while (!microReady2Read()){
	usleep(1); 
      }
      value = v1190->micro;
      value &= 0xfff;
      cout<<"(0x"<<HEX(value)<<") - ";
      res = value*25;
      cout<<"Extra search width is "<<res<<" ns"<<endl;
      // 4
      while (!microReady2Read()){
	usleep(1); 
      }
      value = v1190->micro;
      cout<<"(0x"<<HEX(value)<<") - ";
      res = value*25;
      cout<<"Reject margin is "<<res<<" ns"<<endl;
      // 5
      while (!microReady2Read()){
	usleep(1); 
      }
      value = v1190->micro;
      cout<<"Trigger time subtraction is  ";
      (value & 1) ? cout<<" ENABLED"<<endl :  cout<<" DISABLED"<<endl;
    }

  }
}
/****************************************/
void debugAndTestMenu()
/****************************************/
{
  int fun = 1;
  while (fun != 0){
    printf("\n");
    printf("DEBUG AND TEST MENU (acquisition mode must be set to trigger matching)\n");
    printf("Select an option:\n");
    printf("   0  back to main menu\n");
    printf("   1  Enable test mode\n");
    printf("   2  disable test mode\n");
    printf("   3  set Max hits per event to 1\n");
    printf("   4  generate a SW trigger\n");
    printf("   5  Read Event counter\n");
    printf("   6  Get # of Events in Output Buffer\n");
    printf("   7  Read output buffer\n");
    printf("Your choice ");
    fun = getdecd(fun);
    if (fun == 1)
      enableTestMode();
    if (fun == 2)
      disableTestMode();
    else if(fun == 3)
      setMaxHitsInEvent(1);
    else if(fun == 4){
      v1190->softwareTrigger = 0;
      usleep(20000);}
    else if(fun == 5)
      cout<<"Number of occurred triggers/acquired events is "<<v1190->eventCounter<<endl;
    else if(fun == 6)
      cout<<"Number of events in output buffer "<<v1190->eventStored<<endl;
    else if(fun == 7)
      readOneEventfromOutputBuffer();
  }
 }
/****************************************/
void TDCReadoutMenu()
/****************************************/
{

  int fun = 1;
  while (fun != 0){
    printf("\n");
    printf("ACQUISITION MODE MENU\n");
    printf("Select an option:\n");
    printf("   0 back to main menu\n");
    printf("   1 enable TDC header and traile\n");
    printf("   2 disable TDC header and traile \n");
    printf("   3 set Max hits per event\n");
    printf("   4 Read header and trailer status\n");
    printf("Your choice ");
    fun = getdecd(fun);
    if (fun == 1){
      unsigned short opCode = buildOPCODE(0x30,0);
      v1190->micro = opCode;
    }
    if (fun == 2){
      unsigned short opCode = buildOPCODE(0x31,0);
      v1190->micro = opCode;
    }
    if (fun == 3){
      int maxNhit = 3;
      printf("enter max hit per event (a 2 power up to 128, unlimited == 3");
      maxNhit = getdecd(maxNhit);  
      setMaxHitsInEvent(maxNhit);
    }
    if (fun == 4){
      unsigned short opCode = buildOPCODE(0x32,0);
      v1190->micro = opCode;
      while (!microReady2Read()){
	usleep(1); 
      }
      unsigned short value = v1190->micro;
      cout<<" Word found in micro reads "<<HEX(value)<<endl;
      if(value & 1) 
	cout<<"Header and Trailer ENABLED"<<endl;  
      else
	cout<<"Header and Trailer DISABLED"<<endl;         
    }
  }
}
/****************************************/
void readConfigurationROM()
/****************************************/
{
  cout<<" checksum = 0x"<<HEX(v1190->checksum)<<endl;                   
  cout<<" checksum_lenght2 = 0x"<< HEX(v1190->checksum_lenght2)<<endl;           
  cout<<" checksum_lenght1 = 0x"<< HEX(v1190->checksum_lenght1)<<endl;           
  cout<<" checksum_lenght0 = 0x"<< HEX(v1190->checksum_lenght0)<<endl;           
  cout<<" constant2 = 0x"<< HEX(v1190->constant2)<<endl;                  
  cout<<" constant1 = 0x"<< HEX(v1190->constant1)<<endl;                  
  cout<<" constant0 = 0x"<< HEX(v1190->constant0)<<endl;                  
  cout<<" c_code = 0x"<< HEX(v1190->c_code )<<endl;                     
  cout<<" r_code = 0x"<< HEX(v1190->r_code )<<endl;                     
  cout<<" oui2 = 0x"<< HEX(v1190->oui2 )<<endl;                       
  cout<<" oui1 = 0x"<< HEX(v1190->oui1)<<endl;                       
  cout<<" oui0 = 0x"<< HEX(v1190->oui0 )<<endl;                       
  cout<<" vers = 0x"<< HEX(v1190->vers )<<endl;                       
  cout<<" board2 = 0x"<< HEX(v1190->board2 )<<endl;                     
  cout<<" board1 = 0x"<< HEX(v1190->board1 )<<endl;                     
  cout<<" board0 = 0x"<< HEX(v1190->board0 )<<endl;                     
  cout<<" revis3 = 0x"<< HEX(v1190->revis3 )<<endl;                     
  cout<<" revis2 = 0x"<< HEX(v1190->revis2 )<<endl;                     
  cout<<" revis1 = 0x"<< HEX(v1190->revis1 )<<endl;                     
  cout<<" revis0 = 0x"<< HEX(v1190->revis0 )<<endl;                     
  cout<<" sernum1 = 0x"<< HEX(v1190->sernum1 )<<endl;                    
  cout<<" sernum0 = 0x"<< HEX(v1190->sernum0 )<<endl;                    
}

/****************************************/
void decodeControlRegister()
/****************************************/
{
  unsigned short value = v1190->controlRegister;
  
  if((value&1) == 1){
    cout<<"Bus error enabled"<<endl;
  }
  else{
    cout<<"Bus error disable"<<endl;
  }
  if(((value>>1)&1) == 1){
    cout<<"SW termination ON (makes only sense if termination via SW is set)"<<endl;
  }
  else  
    cout<<"SW termination OFF (makes only sense if termination via SW is set)"<<endl;
  if(((value>>2)&1) == 1){
    cout<<"termination via SW"<<endl;
  }
  else{
    cout<<"termination via Dip-switch"<<endl;
  }
  if(((value>>3)&1) == 1){  
    cout<<"Write empty events (even if no TDC data)"<<endl;
  }
  else{
    cout<<"Do NOT write empty events (Global header and trailer not written)"<<endl;
  }
  if(((value>>4)&1) == 1){
    cout<<"Dummy 32-bit word added if needed"<<endl;
  }
  else{
    cout<<"Dummy 32-bit word NOT added"<<endl;
  }  
  if(((value>>5)&1) == 1){
    cout<<"INL compensation enabled"<<endl;
  }
  else {
    cout<<"INL compensation disabled"<<endl;
  }
  if(((value>>6)&1) == 1){
    cout<<"Test mode output buffer enabled"<<endl;
  }
  else{
    cout<<"Test mode output buffer disabled"<<endl;
  }
  if(((value>>7)&1) == 1){
    cout<<"Readout of compensation SRAM enabled"<<endl;
  }
  else{
    cout<<"Readout of compensation SRAM disabled"<<endl;
  }

  if(((value>>8)&1) == 1){
    cout<<"event counter in event FIFO (FIFO enabled)"<<endl;
  }
  else{
    cout<<"No event counters in event FIFO (FIFO disabled, default)"<<endl;
  }
  if(((value>>9)&1) ==1){
    cout<<"Extended trigger time writing enabled"<<endl;
  }
  else{
    cout<<"Extended trigger time writing disabled"<<endl;
  }
  if(((value>>12)&1) == 1){
    cout<<"MEB access enabled"<<endl;
  }
  else{
    cout<<"MEB access disabled"<<endl;
  }

}
/****************************************/
void decodeStatusRegister()
/****************************************/
{
  unsigned short value = v1190->statusRegister;

  ( value & 1) ? cout<<"Data Ready present"<<endl : cout<<"Data ready absent"<<endl;
  ( (value>>1) & 1) ? cout<<"Almost full Buffer reached"<<endl : cout<<"Buffer is not almost full"<<endl;
  ( (value>>2) & 1) ? cout<<"Output Buffer is full"<<endl : cout<<"Still space in Output buffer"<<endl;
  ( (value>>3) & 1) ? cout<<"Trigger matching mode"<<endl : cout<<"Continuous storage mode"<<endl;
  ( (value>>4) & 1) ? cout<<"Header and trailer enabled"<<endl : cout<<"Header and trailer disabled"<<endl;
  ( (value>>5) & 1) ? cout<<"Control bus termination ON"<<endl : cout<<"Control bus termination OFF"<<endl;
  ( (value>>6) & 1) ? cout<<"Error in TDC 0"<<endl : cout<<"No errors in TDC 0"<<endl;
  ( (value>>7) & 1) ? cout<<"Error in TDC 1"<<endl : cout<<"No errors in TDC 1"<<endl;
  ( (value>>10) & 1) ? cout<<"Bus Error present"<<endl : cout<<"No bus errors"<<endl;
  ( (value>>11) & 1) ? cout<<"Board purged"<<endl : cout<<"Board not purged"<<endl;

  unsigned short res = (value>>12) & 3; 
  if( res == 0) 
    cout<<"TDC Resolution is 800 ps"<<endl;
  else if( res == 1) 
    cout<<"TDC Resolution is 200 ps"<<endl;
  else if( res == 2) 
    cout<<"TDC Resolution is 100 ps"<<endl;
  else
    cout <<"Error decoding TDC resolution: (got value "<<res<<")"<< endl;
  ( (value>>14) & 1) ? cout<<"Module in PAIR mode"<<endl : cout<<"Module is NOT in PAIR mode"<<endl;
  ( (value>>15) & 1) ? cout<<"At least one trigger was lost"<<endl : cout<<"No lost triggers"<<endl;
}

/****************************************/
void readOneEventfromOutputBuffer(){
/****************************************/
  bool goOn = true;
  unsigned int val, val2, val3, hitT;
  unsigned int buf;

  int nwords = 0;
  //  int evNum_old = 0;
  //  int evNum = 0;
  
  while ( goOn ){

    const unsigned int data = v1190->outputBuffer;
    cout<<"0x"<<HEX(data)<<endl;

    if(data == FILLER) goOn = false;
    ++nwords;

    // bits 27-31 identify the word type
    buf = (data>>27) & 0x1f;

    if( buf == 17 ){
      val = data & 0x7ffffff;
      cout<<"This is a GLOBAL TRIGGER time: 0x"<<HEX(val)<<endl;
    }
    else if( buf == 16){
      val = (data & 0x000fffff) >> 5;
      val2 = (data & 0x7ffffff) >> 24;
      cout<<"GLOBAL TRAILER found. Event contained "<< val <<" words. Status is "<< val2 <<endl;
      //      evNum_old = evNum;
      goOn = false;
    }
    else if( buf == 8 ){
      cout<<"This is a GLOBAL HEADER.";
      val = data & 0x1f;
      cout <<"  The geographical address is 0x"<<HEX(val);
      val = (data & 0x7ffffff) >> 5;
      cout <<"; the event number is "<< val << endl;
      //      evNum = val;
    }
    else if (buf<8){

      cout<<"This is a TDC DATA.";      
      if( buf == 1 ){	
	val = data & 0xfff;
	val2 = (data & 0xfff000)>>12;
	val3 = (data & 0x3000000)>>24;
	cout<<"  a TDC HEADER with bunch id "<< val <<", event id "<< val2 << ", TDC number "<< val3 <<endl;
      }
      else if( buf == 0 ){
	hitT = data & 0x7fff;
	val2 = (data & 0x3ffffff)>>19;
	cout<<"  a TDC meas. of 0x"<<HEX( hitT )<<" ("<<double(hitT)/10. << " ns) from channel "<< val2 <<endl ;
      }
      else if( buf == 4 ){
	val = data & 0xffffff;
        val2 = (data>>24)&3;
	cout<<"  a TDC ERROR or a TEST WORD 0x"<< HEX(val)<<" for TDC "<<val2 <<endl;
      }
      if( buf == 3 ){
	val = (data & 0xffffff)>>12;
	val2 = data & 0xfff;
	cout<<"  a TDC TRAILER of event "<< val <<" with word count equal to "<< val2 <<endl;
      }
    }   
  }
  cout << "A total of "<< nwords <<" where read from the Output Buffer"<<endl;
}
/****************************************/
void setMaxHitsInEvent(unsigned short opt){
/****************************************/
  unsigned short opCode = buildOPCODE(0x33,0);
  v1190->micro = opCode;
  unsigned short size;
  switch (opt)
    {
    case 0 : 
      size =  0;
      break;
    case 1 :
      size = 1;
      break;
    case 2 :
      size = 2;
      break;
    case 4 :
      size = 3;
      break;
    case 8 :
      size = 8;
      break;
    case 16 :
      size = 9;
      break;
    case 32 :
      size = 10;
      break;
    case 128 :
      size = 16;
      break;
    case 3 :
      size = 17;
      break;
    default :
      size = 17;
    }
  while (!microReady4Write()){
    usleep(1); 
  }
  v1190->micro = size;

}
/****************************************/
void setECLoutput(){
/****************************************/
}

/****************************************/
void setContinuousStorageMode(){
/****************************************/
  unsigned short opCode = buildOPCODE(1,0);
  v1190->micro = opCode;
}
/****************************************/
void setTriggerMatchingMode(){
/****************************************/
  unsigned short opCode = buildOPCODE(0,0);
  v1190->micro = opCode;
}
/****************************************/
void enableHeaderAndTrailer(){
/****************************************/
  unsigned short opCode = buildOPCODE(0x30,0);
  v1190->micro = opCode;
}
/****************************************/
void disableHeaderAndTrailer(){
/****************************************/
  unsigned short opCode = buildOPCODE(0x31,0);
  v1190->micro = opCode;
}
/****************************************/
void disableTestMode(){
/****************************************/
  unsigned short opCode = buildOPCODE(0xc6,0);
  v1190->micro = opCode;
}
/****************************************/
void enableTestMode(){
/****************************************/
  unsigned short test_data_low;
  unsigned short test_data_high;
  test_data_low = 0xcdef;
  test_data_high = 0xab;

  unsigned short opCode = buildOPCODE(0xc5,0);
  v1190->micro = opCode;

  while (!microReady4Write()){
    usleep(1); 
  }
  v1190->micro = test_data_low;

  while (!microReady4Write()){
    usleep(1); 
  }
  v1190->micro = test_data_high;  

  //now reset it with opcode

  opCode = buildOPCODE(0xc7,0);
  while (!microReady4Write()){
    usleep(1); 
  }
  v1190->micro = 0x1110;  

  opCode = buildOPCODE(0xc7,1);
  while (!microReady4Write()){
    usleep(1); 
  }
  v1190->micro = 0x2220;  

}

/****************************************/
unsigned short buildOPCODE(unsigned short command, unsigned short object){
/****************************************/
  unsigned short opcode = ((command & 0xff)<<8) | (object & 0xff);
  cout <<"returning OPCODE 0x"<<HEX(opcode)<<endl;  
  return opcode;
}
/****************************************/
bool microReady4Write(){
/****************************************/
  return (v1190->microHandShake & 1) ? true : false;
}
/****************************************/
bool microReady2Read(){
/****************************************/
  return ((v1190->microHandShake>>1) & 1) ? true : false;
}
