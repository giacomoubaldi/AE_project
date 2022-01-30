// $Id$
/****************************************************************/
/*                                              		*/
/*  file: tsiscope.c                            		*/
/*                                              		*/
/*  tsiscope decodes and displays the contents  		*/
/*  of various registers of the Tundra Tsi148 chip	   	*/
/*                                              		*/
/*  Author: Markus Joos, CERN-ECP               		*/
/*                                              		*/
/*  14.Jul.08  MAJO  created                   		  	*/
/********C 2016 - A nickel program worth a dime******************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include "rcc_error/rcc_error.h"
#include "vme_rcc/vme_rcc.h"
#include "io_rcc/io_rcc.h"
#include "DFDebug/DFDebug.h"
#include "ROSGetInput/get_input.h"
#include "rcc_time_stamp/tstamp.h"
#include "vme_rcc/tsi148.h"


/*CCT SBC special register*/  //MJ: Check which of these registers are relevant for the tsi148 based family of SBCs
#define CMOSA          0x70
#define CMOSD          0x71
#define BID1           0x35
#define BID2           0x36
#define CSR0_717       0x210
#define CSR2_717       0x211
#define CSR1_717       0x212
#define CSR4_717       0x214
#define SLOT_717       0x31e


// prototypes
void SigQuitHandler(int signum);
void togglesysreset(void);
int hwconf(void);
int mainhelp(void);
void vmemenu(void);
void pcimenu(void);
int setdebug(void);
int vmehelp(void);
void pcidecode(void);
void pcierror(void);
void decmmap(void);
void decmmapraw(void);
void setmmap(void);
void decsmap(void);
void decminf(u_int nr, u_int otsau, u_int otsal, u_int oteau, u_int oteal, u_int otofu, u_int otofl, u_int otbs, u_int otat);
void decsinf(u_int nr, u_int itsau, u_int itsal, u_int iteau, u_int iteal, u_int itofu, u_int itofl, u_int itat);
void decirq(void);
void decmisc(void);
void decberr(void);
void decdma(void);
void setreg(void);
void irqmapdecode(u_int data, u_int pos);
void selfaddr(void);

//globals
u_int cont;
u_long tsibase;
int btype, tsitype;
tsi148_regs_t *tsi;


/*********************************/
void SigQuitHandler(int /*signum*/)
/*********************************/
{
  cont = 0;
}


/*********************/
u_int bswap(u_int word)
/*********************/
{
  return ((word & 0xFF) << 24) + ((word & 0xFF00) << 8) + ((word & 0xFF0000) >> 8) + ((word & 0xFF000000) >> 24);
}


/************/
int main(void)
/************/
{
  static int fun = 1;
  static struct sigaction sa;
  int ret, stat;
  u_int id;
  int found = 0, i = 0;
  u_long virtaddr;
  u_char *id_ptr;
  
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = SigQuitHandler;
  stat = sigaction(SIGQUIT, &sa, NULL);
  if (stat < 0)
  {
    printf("Cannot install signal handler (error=%d)\n", stat);
    exit(0);
  }
  
  ret = IO_Open();
  if (ret)
  {
    rcc_error_print(stdout, ret);
    exit(-1);
  }
 
  printf("Searching for Board Id Structure\n");
  btype = 0;

  ret = IO_PCIMemMap(0xe0000, 0x20000, &virtaddr);
  if (ret != IO_RCC_SUCCESS)
    rcc_error_print(stdout, ret);

  id_ptr = (u_char *)virtaddr;

  for (i = 0; (i + 7) < 0x20000; i++)
  {
    if ((id_ptr[0] == '$') && (id_ptr[1] == 'S') && (id_ptr[2] == 'H') && (id_ptr[3] == 'A') && (id_ptr[4] == 'A') && (id_ptr[5] == 'N') && (id_ptr[6] == 'T') && (id_ptr[7] == 'I'))
    {
      printf("Board ID table found\n");
      printf("Board Id Structure Version:%d\n", id_ptr[8]);   //VP717 = 1
      printf("Board Type:%d\n", id_ptr[9]);                   //VP717 = 2
      printf("Board Id:%d,%d\n", id_ptr[10], id_ptr[11]);     //VP717 = 28
      found = 1;
      if (id_ptr[8] == 1 && id_ptr[9] == 2 && id_ptr[10] == 28 && id_ptr[11] == 0) 
        btype = 1;   
      if (id_ptr[8] == 1 && id_ptr[9] == 2 && id_ptr[10] == 31 && id_ptr[11] == 0) 
        btype = 2;   
      if (id_ptr[8] == 2 && id_ptr[9] == 2 && id_ptr[10] == 31 && id_ptr[11] == 0) 
        btype = 2;   
    }
    if (found)
      break;
    id_ptr++;
  }

  if(found == 0)
  {
    printf("Board Id structure not found !!\n");
    exit(0);
  }

  if (btype == 1)
    printf("\n\n\nThis is TSISCOPE for a VP717\n");
  else if (btype == 2)
    printf("\n\n\nThis is TSISCOPE for a VP917\n");
  else
  {
    printf("This type of CCT board is not supported\n");
    exit(-1);
  }

  printf("=======================================\n");


  DF::GlobalDebugSettings::setup(5, DFDB_VMERCC);


  ret = VME_Open();
  if (ret != VME_SUCCESS)
  {
    VME_ErrorPrint(ret);
    exit(-1);
  }

  ret = VME_Tsi148Map(&tsibase);
  if (ret != VME_SUCCESS)
  {
    VME_ErrorPrint(ret);
    exit(-1);
  }

  tsi = (tsi148_regs_t *)tsibase;
  
  id = (tsi->noswap_clas_revi) & 0xff;   	//mask Revision ID
  if (id == 1)
    tsitype = 1;	 		        //Tsi148 I
  else
  {
    printf("devi_veni is 0x%08x\n", tsi->noswap_devi_veni);
    printf("stat_cmmd is 0x%08x\n", tsi->noswap_stat_cmmd);
    printf("clas_revi is 0x%08x\n", tsi->noswap_clas_revi);
    printf("Tsi148 revision is %d\n", id);
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("WARNING: The revision of the Tsi148 chip is unknown\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    tsitype = 1;
  }
 
  while(fun != 0)
  {
    printf("\n");
    printf("Select an option:\n");
    printf("   1 Help                        2 Decode VME registers\n");
    printf("   3 Decode PCI registers        4 Show special registers\n");
    printf("   5 Toggle SYSRESET bit         6 Modify a register\n");
    printf("   7 Test self addressing\n");
    printf("   0 Quit\n");
    printf("Your choice: ");
    fun = getdecd(fun);
    if (fun == 1) mainhelp();
    if (fun == 2) vmemenu();
    if (fun == 3) pcimenu();
    if (fun == 4) hwconf();
    if (fun == 5) togglesysreset();
    if (fun == 6) setreg();
    if (fun == 7) selfaddr();
  }
}


/*****************/
void selfaddr(void)
/*****************/
{
  int sa_handle;
  u_int odata, ret, rdata, sa_base = 0x10000, sa_am = 1;
  VME_MasterMap_t master_map;

  printf("Enter the A24 base address for the CGR register base: ");
  sa_base = gethexd(sa_base);
  
  printf("Enter the AM code for the CGR register base: (1=A24, 2=A32)");
  sa_am = gethexd(sa_am);

  tsi->cbau  = 0;
  tsi->cbal  = bswap(sa_base & 0xfffff000);
  tsi->crgat = bswap(0x85 + (sa_am << 4));
  
  master_map.vmebus_address = sa_base;
  master_map.window_size = 0x1000;
  if (sa_am == 1)
    master_map.address_modifier = VME_A24;
  else
    master_map.address_modifier = VME_A32;
  master_map.options = 0;
  
  ret = VME_MasterMap(&master_map, &sa_handle);
  if (ret != VME_SUCCESS)
    VME_ErrorPrint(ret);
  else
    printf("Handle = %d\n", sa_handle);
    
  ret = VME_ReadSafeUInt(sa_handle, 0, &rdata);  
  if (ret != VME_SUCCESS)
    VME_ErrorPrint(ret); 
  else
    printf("The register at offset 0 contains data pattern 0x%08x\n", rdata);

  ret = VME_ReadSafeUInt(sa_handle, 0x1e0, &rdata);  
  if (ret != VME_SUCCESS)
    VME_ErrorPrint(ret); 
  else
    printf("The register at offset 0x1e0 contains data pattern 0x%08x\n", rdata);
 
  odata = rdata; 
  
  VME_WriteSafeUInt(sa_handle, 0x1e0, 0xffffffff);  
  ret = VME_ReadSafeUInt(sa_handle, 0x1e0, &rdata);  
  if (ret != VME_SUCCESS)
    VME_ErrorPrint(ret); 
  else
    printf("The register at offset 0x1e0 contains data pattern 0x%08x\n", rdata);

  VME_WriteSafeUInt(sa_handle, 0x1e0, odata);  
  ret = VME_ReadSafeUInt(sa_handle, 0x1e0, &rdata);  
  if (ret != VME_SUCCESS)
    VME_ErrorPrint(ret); 
  else
    printf("The register at offset 0x1e0 contains data pattern 0x%08x\n", rdata);


  ret = VME_MasterUnmap(sa_handle);
  if (ret != VME_SUCCESS)
    VME_ErrorPrint(ret);
  else
    printf("Handle = %d unmapped\n", sa_handle); 
    
  tsi->cbau  = 0;
  tsi->cbal  = 0;
  tsi->crgat = 0;
}





/***************/
void setreg(void)
/***************/
{
  u_int *reg_ptr, data;
  static u_int offset;
  
  printf("==========================================================\n");
  printf("Enter the offset of the register: ");
  offset = gethexd(offset);
  
  reg_ptr = (u_int *)(tsibase + offset);
  data = *reg_ptr;
  
  if (offset > 0xff)
   data = bswap(data);
  
  printf("The register currently contains 0x%08x\n", data);
  printf("Enter the new data: ");
  data = gethexd(data);
  
  if (offset > 0xff)
    *reg_ptr = bswap(data);
  else
    *reg_ptr = data;
    
  printf("==========================================================\n");
}


/***********************/
void togglesysreset(void)
/***********************/
{
  u_char d1;
  u_int ret;

  printf("==========================================================\n");

  ret = IO_IOPeekUChar(CSR2_717, &d1);
  if (ret)
  { rcc_error_print(stdout, ret);  return; }

  if (d1 & 0x20)
  {
    d1 &= 0xdf;
    ret = IO_IOPokeUChar(CSR2_717, d1);
    if (ret)
    { rcc_error_print(stdout, ret);  return; }
    printf("SYSRESET now has no effect\n");
  }
  else
  {
    d1 |= 0x20;
    ret = IO_IOPokeUChar(CSR2_717, d1);
    if (ret)
    { rcc_error_print(stdout, ret);  return; }
    printf("SYSRESET now has no effect\n");
  }
  return;
}


/**************/
int hwconf(void)
/**************/
{
  u_char d1, d2, d3, d4, d5;
  u_int ret;

  ret = IO_IOPeekUChar(CSR0_717, &d1);
  if (ret)
  { rcc_error_print(stdout, ret);  return(-1); }

  ret = IO_IOPeekUChar(CSR1_717, &d2);
  if (ret)
  { rcc_error_print(stdout, ret);  return(-1); }

  ret = IO_IOPeekUChar(CSR2_717, &d3);
  if (ret)
  { rcc_error_print(stdout, ret);  return(-1); }

  ret = IO_IOPeekUChar(CSR4_717, &d4);
  if (ret)
  { rcc_error_print(stdout, ret);  return(-1); }

  ret = IO_IOPeekUChar(SLOT_717, &d5);
  if (ret)
  { rcc_error_print(stdout, ret);  return(-1); }

  printf("==========================================================\n");
  printf("Decoding control and status register 0 (0x%02x):\n", d1);
  printf("H/W revision:                    %d\n", d1 & 0x7);
  printf("VME byte swap master enable:     %s\n", (d1 & 0x8)?"enabled":"disabled");
  printf("VME byte swap slave enable:      %s\n", (d1 & 0x10)?"enabled":"disabled");
  printf("VME byte swap fast write enable: %s\n", (d1 & 0x20)?"enabled":"disabled");
  printf("Console:                         %s\n", (d1 & 0x80)?"COM port":"VGA");

  printf("\nDecoding control and status register 1 (0x%02x):\n", d2);
  printf("PMC1 present:                    %s\n", (d2 & 0x1)?"yes":"no");
  printf("PMC2 present:                    %s\n", (d2 & 0x2)?"yes":"no");
  
  printf("\nDecoding control and status register 2 (0x%02x):\n", d3);
  printf("Effect of VMEbus SYSRESET#:      %s\n", (d3 & 0x20)?"SBC reset":"No effect");

  printf("\nDecoding control and status register 4 (0x%02x):\n", d4);
  printf("Board fail signal is:            %s\n", (d4 & 0x1)?"not active":"active");

  printf("\nDecoding VME slot ID register(0x%02x):\n", d5);
  printf("The card is installed in slot:   %d\n", d5 & 0x1f);
  printf("The GA parity bit is:            %d\n", (d5 >> 5) & 0x1);
  printf("==========================================================\n");

  return(0);
}


/****************/
void pcimenu(void)
/****************/
{
  static int fun;

  fun = 1;
  while(fun != 0)
  {
    printf("\n");
    printf("Select an option:\n");
    printf("   1 Decode PCI registers        2 Decode PCI error record\n");
    printf("   0 Main menu\n");
    printf("Your choice: ");
    fun = getdecd(fun);
    if (fun == 1) pcidecode();
    if (fun == 2) pcierror();
  }
}


/*****************/
void pcierror(void)
/*****************/
{
  u_int data;
  
  printf("\n=========================================================================\n");
  
  data = bswap(tsi->edpat);
  
  if ((data & 0x80000000) == 0)
    printf("No PCI error detected\n");
  else
  {
    printf("register edpau = 0x%08x\n", bswap(tsi->edpau));
    printf("register edpal = 0x%08x\n", bswap(tsi->edpal));
    printf("register edpxa = 0x%08x\n", bswap(tsi->edpxa));
    printf("register edpxs = 0x%08x\n", bswap(tsi->edpxs));
    printf("register edpat = 0x%08x\n", bswap(tsi->edpat));
  }
  
  printf("=========================================================================\n\n");
}


/******************/
void pcidecode(void)
/******************/
{
  u_int data, data2;
  
  printf("\n=========================================================================\n");
  printf("PCI configuration registers\n");
  printf("---------------------------\n");
  data = tsi->noswap_stat_cmmd;
  printf("stat_cmmd (0x%08x):\n", data);
  printf("Parity error:           %s\n", data & 0x80000000?"Yes":"No");
  printf("System error:           %s\n", data & 0x40000000?"Yes":"No");
  printf("Master abort received:  %s\n", data & 0x20000000?"Yes":"No");
  printf("Target abort received:  %s\n", data & 0x10000000?"Yes":"No");
  printf("Target abort signalled: %s\n", data & 0x08000000?"Yes":"No");
  printf("Data parity error:      %s\n", data & 0x01000000?"Yes":"No");
  printf("Bus master enebled:     %s\n", data & 0x4?"Yes":"No");
  printf("Memory space enabled:   %s\n", data & 0x2?"Yes":"No");

  data = tsi->noswap_head_mlat_clsz;
  printf("\nhead_mlat_clsz (0x%08x):\n", data);
  printf("Master latency timer: %d\n", (data >> 8) & 0xff);
  
  data = tsi->noswap_mbarl;
  data2 = tsi->noswap_mbaru;
  printf("\nmbarl/u (0x%08x / 0x%08x):\n", data, data2);
  printf("PCI MEM base address of the registers: 0x%08x%08x\n", data2, data & 0xfffff000);

  data = tsi->noswap_mxla_mngn_intp_intl;
  printf("\nmxla_mngn_intp_intl (0x%08x):\n", data);
  printf("Interrupt line: %d\n", data & 0xff);
  
  data = tsi->noswap_pcixstat;
  printf("\npcixstat (0x%08x):\n", data);
  printf("Received split completion error: %s\n", data & 0x20000000?"Yes":"No");
  printf("Unexpected split completion:     %s\n", data & 0x00080000?"Yes":"No");
  printf("Split completion discarded:      %s\n", data & 0x00040000?"Yes":"No");
  
  printf("\nOther PCI registers\n");
  printf("-------------------\n");
  data = bswap(tsi->pcsr);
  printf("pcsr (0x%08x):\n", data);
  if (((data >> 24) & 0x7) == 0) printf("Split response time out:      16 us\n");
  if (((data >> 24) & 0x7) == 1) printf("Split response time out:      32 us\n");
  if (((data >> 24) & 0x7) == 2) printf("Split response time out:      64 us\n");
  if (((data >> 24) & 0x7) == 3) printf("Split response time out:      128 us\n");
  if (((data >> 24) & 0x7) == 4) printf("Split response time out:      256 us\n");
  if (((data >> 24) & 0x7) == 5) printf("Split response time out:      512 us\n");
  if (((data >> 24) & 0x7) == 6) printf("Split response time out:      1024 us\n");
  if (((data >> 24) & 0x7) == 7) printf("Split response time out:      disabled\n");
  
  printf("Maximum retry count:          %s\n", (data & 0x20000)?"Enabled":"Disabled");
  printf("Split response time out:      %s\n", (data & 0x400)?"Seen":"Not seen");
  printf("Delayed transaction time out: %s\n", (data & 0x200)?"Seen":"Not seen");
  printf("Maximum retry count error:    %s\n", (data & 0x100)?"Seen":"Not seen");
  printf("PCI bus width:                %s\n", (data & 0x40)?"32 bit":"64 bit");
  
  printf("=========================================================================\n\n");
}


/****************/
int setdebug(void)
/****************/
{
  static u_int dblevel = 0, dbpackage = DFDB_VMERCC;
  
  printf("Enter the debug level: ");
  dblevel = getdecd(dblevel);
  printf("Enter the debug package: ");
  dbpackage = getdecd(dbpackage);
  DF::GlobalDebugSettings::setup(dblevel, dbpackage);
  return(0);
}


/****************/
int mainhelp(void)
/****************/
{
  printf("\n=========================================================================\n");
  printf("Call Markus Joos, 72364, if you need more help\n");
  printf("=========================================================================\n\n");
  return(0);
}


/****************/
void vmemenu(void)
/****************/
{
  static int fun;

  fun = 1;
  while(fun != 0)
  {
    printf("\n");
    printf("Select an option:\n");
    printf("   1 Help                        2 Decode master mapping\n");
    printf("   3 Decode DMA registers        4 Decode interrupt status\n");
    printf("   5 Decode misc. registers      6 Decode BERR information\n");
    printf("   7 Decode slave mapping        8 Master map raw register values\n");
    printf("   9 Modify master map 1\n");
    printf("   0 Main menu\n");
    printf("Your choice: ");
    fun = getdecd(fun);
    if (fun == 1) vmehelp();
    if (fun == 2) decmmap();
    if (fun == 3) decdma();
    if (fun == 4) decirq();
    if (fun == 5) decmisc();
    if (fun == 6) decberr();
    if (fun == 7) decsmap();
    if (fun == 8) decmmapraw();
    if (fun == 9) setmmap();
  }
}


/***************/
void decdma(void)
/***************/
{
  u_int data, value, value2;
  
  printf("\n=========================================================================\n");
  printf("NOTE: Only DMA controller #0 is currently supported\n");
  
  data = bswap(tsi->dctl0);
  printf("dctl0 (0x%08x):\n", data);
  printf("DMA mode:                              %s\n", (data & 0x800000)?"Direct":"Linked list"); 
  printf("Deliver buffered data on VMEbus error: %s\n", (data & 0x20000)?"Yes":"No");
  printf("Deliver buffered data on PCI error:    %s\n", (data & 0x10000)?"Yes":"No");

  value = (data >> 12) & 0x7;
  if (value == 0) printf("VMEbus (read) block size:              32 bytes\n");
  if (value == 1) printf("VMEbus (read) block size:              64 bytes\n");
  if (value == 2) printf("VMEbus (read) block size:              128 bytes\n");
  if (value == 3) printf("VMEbus (read) block size:              256 bytes\n");
  if (value == 4) printf("VMEbus (read) block size:              512 bytes\n");
  if (value == 5) printf("VMEbus (read) block size:              1024 bytes\n");
  if (value == 6) printf("VMEbus (read) block size:              2048 bytes\n");
  if (value == 7) printf("VMEbus (read) block size:              4096 bytes\n");
  
  value = (data >> 8) & 0x7;
  if (value == 0) printf("VMEbus back off time:                  0 us\n");
  if (value == 1) printf("VMEbus back off time:                  1 us\n");
  if (value == 2) printf("VMEbus back off time:                  2 us\n");
  if (value == 3) printf("VMEbus back off time:                  4 us\n");
  if (value == 4) printf("VMEbus back off time:                  8 us\n");
  if (value == 5) printf("VMEbus back off time:                  16 us\n");
  if (value == 6) printf("VMEbus back off time:                  32 us\n");
  if (value == 7) printf("VMEbus back off time:                  64 us\n");
  
  value = (data >> 4) & 0x7;
  if (value == 0) printf("PCI (read) block size:                 32 bytes\n");
  if (value == 1) printf("PCI (read) block size:                 64 bytes\n");
  if (value == 2) printf("PCI (read) block size:                 128 bytes\n");
  if (value == 3) printf("PCI (read) block size:                 256 bytes\n");
  if (value == 4) printf("PCI (read) block size:                 512 bytes\n");
  if (value == 5) printf("PCI (read) block size:                 1024 bytes\n");
  if (value == 6) printf("PCI (read) block size:                 2048 bytes\n");
  if (value == 7) printf("PCI (read) block size:                 4096 bytes\n");
  
  value = data & 0x7;
  if (value == 0) printf("PCI back off time:                     0 us\n");
  if (value == 1) printf("PCI back off time:                     1 us\n");
  if (value == 2) printf("PCI back off time:                     2 us\n");
  if (value == 3) printf("PCI back off time:                     4 us\n");
  if (value == 4) printf("PCI back off time:                     8 us\n");
  if (value == 5) printf("PCI back off time:                     16 us\n");
  if (value == 6) printf("PCI back off time:                     32 us\n");
  if (value == 7) printf("PCI back off time:                     64 us\n");
  
  data = bswap(tsi->dsta0);
  printf("\ndsta0 (0x%08x):\n", data);
  printf("PCI/X or VME error:        %s\n", (data & 0x10000000)?"Yes":"No");
  printf("DMA aborted:               %s\n", (data & 0x8000000)?"Yes":"No");
  printf("DMA paused:                %s\n", (data & 0x4000000)?"Yes":"No");
  printf("DMA done wihtout error:    %s\n", (data & 0x2000000)?"Yes":"No");
  printf("DMA controller still busy: %s\n", (data & 0x1000000)?"Yes":"No");
  if (data & 0x10000000)
  {
    printf("Source of error:           %s\n", (data & 0x100000)?"PCI":"VME");
    value = (data >> 20) & 0x1;
    value2 = (data >> 16) & 0x3;
    if (value == 0 && value2 == 0) printf("Type details:              Bus error (SCT, BLT, MBLT, 2eVME even data, 2eSST)\n");
    if (value == 0 && value2 == 1) printf("Type details:              Bus error (2eVME odd data)\n");
    if (value == 0 && value2 == 2) printf("Type details:              Slave termination (2eVME even data, 2eSST read)\n");
    if (value == 0 && value2 == 3) printf("Type details:              Slave termination (2eVME odd data, 2eSST read last word invalid)\n");
    if (value == 1 && value2 == 0) printf("Type details:              PXI/X error\n");
    if (value == 1 && value2 == 1) printf("Type details:              undefined\n");
    if (value == 1 && value2 == 2) printf("Type details:              undefined\n");
    if (value == 1 && value2 == 3) printf("Type details:              undefined\n");
  }
  
  printf("\nInitial source address:   0x%08x%08x\n", bswap(tsi->dsau0), bswap(tsi->dsal0));  
  printf("(Current) source address: 0x%08x%08x\n", bswap(tsi->dcsau0), bswap(tsi->dcsal0));  
  data = bswap(tsi->dsat0);
  printf("\ndsat0 (0x%08x):\n", data);

  value = (data >> 28) & 0x3;
  if (value == 0) printf("Data source bus:     PCI\n");
  if (value == 1) printf("Data source bus:     VMEbus\n");
  if (value > 1)  printf("Data source bus:     Data pattern\n");
  
  value = (data >> 11) & 0x3;
  if (value == 0) printf("2eSST speed:         160 MB/s\n");
  if (value == 1) printf("2eSST speed:         267 MB/s\n");
  if (value == 2) printf("2eSST speed:         320 MB/s\n");
  if (value == 3) printf("2eSST speed:         undefined\n");
  
  value = (data >> 8) & 0x7;
  if (value == 0) printf("Transfer mode:       Single cycle\n");
  if (value == 1) printf("Transfer mode:       BLT\n");
  if (value == 2) printf("Transfer mode:       MBLT\n");
  if (value == 3) printf("Transfer mode:       2eVME\n");
  if (value == 4) printf("Transfer mode:       2eSST\n");
  if (value == 5) printf("Transfer mode:       2eSST broadcast\n");
  if (value > 5)  printf("Transfer mode:       undefined\n");

  value = (data >> 6) & 0x3;
  if (value == 0) printf("VMEbus data width:   16 bit\n");
  if (value == 1) printf("VMEbus data width:   32 bit\n");
  if (value > 1)  printf("VMEbus data width:   undefined\n");
  
  value = (data >> 4) & 0x3;
  if (value == 0) printf("VMEbus AM-code type: User / Data\n");
  if (value == 1) printf("VMEbus AM-code type: User / Program/n");
  if (value == 2) printf("VMEbus AM-code type: Supervisor / Data\n");
  if (value == 3) printf("VMEbus AM-code type: Supervisor / Program\n");

  value = data & 0xf;
       if (value == 0)  printf("VMEbus base AM-code: A16\n");
  else if (value == 1)  printf("VMEbus base AM-code: A24\n");
  else if (value == 2)  printf("VMEbus base AM-code: A32\n");
  else if (value == 3)  printf("VMEbus base AM-code: A64\n");
  else if (value == 4)  printf("VMEbus base AM-code: CR-CSR\n");
  else if (value == 8)  printf("VMEbus base AM-code: User1\n");
  else if (value == 9)  printf("VMEbus base AM-code: User2\n");
  else if (value == 10) printf("VMEbus base AM-code: User3\n");
  else if (value == 11) printf("VMEbus base AM-code: User4\n");
  else                  printf("VMEbus base AM-code: undefined\n");
  
  printf("\nInitial destination address:   0x%08x%08x\n", bswap(tsi->ddau0), bswap(tsi->ddal0));  
  printf("(Current) destination address: 0x%08x%08x\n", bswap(tsi->dcdau0), bswap(tsi->dcdal0));  
  data = bswap(tsi->ddat0);
  printf("\nddat0 (0x%08x):\n", data);
  printf("Data source bus:     %s\n", (data & 0x10000000)?"VMEbus":"PCI");
  
  value = (data >> 11) & 0x3;
  if (value == 0) printf("2eSST speed:         160 MB/s\n");
  if (value == 1) printf("2eSST speed:         267 MB/s\n");
  if (value == 2) printf("2eSST speed:         320 MB/s\n");
  if (value == 3) printf("2eSST speed:         undefined\n");
  
  value = (data >> 8) & 0x7;
  if (value == 0) printf("Transfer mode:       Single cycle\n");
  if (value == 1) printf("Transfer mode:       BLT\n");
  if (value == 2) printf("Transfer mode:       MBLT\n");
  if (value == 3) printf("Transfer mode:       2eVME\n");
  if (value == 4) printf("Transfer mode:       2eSST\n");
  if (value > 4)  printf("Transfer mode:       undefined\n");

  value = (data >> 6) & 0x3;
  if (value == 0) printf("VMEbus data width:   16 bit\n");
  if (value == 1) printf("VMEbus data width:   32 bit\n");
  if (value > 1)  printf("VMEbus data width:   undefined\n");
  
  value = (data >> 4) & 0x3;
  if (value == 0) printf("VMEbus AM-code type: User / Data\n");
  if (value == 1) printf("VMEbus AM-code type: User / Program/n");
  if (value == 2) printf("VMEbus AM-code type: Supervisor / Data\n");
  if (value == 3) printf("VMEbus AM-code type: Supervisor / Program\n");

  value = data & 0xf;
       if (value == 0)  printf("VMEbus base AM-code: A16\n");
  else if (value == 1)  printf("VMEbus base AM-code: A24\n");
  else if (value == 2)  printf("VMEbus base AM-code: A32\n");
  else if (value == 3)  printf("VMEbus base AM-code: A64\n");
  else if (value == 4)  printf("VMEbus base AM-code: CR-CSR\n");
  else if (value == 8)  printf("VMEbus base AM-code: User1\n");
  else if (value == 9)  printf("VMEbus base AM-code: User2\n");
  else if (value == 10) printf("VMEbus base AM-code: User3\n");
  else if (value == 11) printf("VMEbus base AM-code: User4\n");
  else                  printf("VMEbus base AM-code: undefined\n");
  
  printf("\nInitial linked list PCI address:   0x%08x%08x\n", bswap(tsi->dnlau0), bswap(tsi->dnlal0) & 0xfffffff8);  
  printf("Last element in list: %s\n", (bswap(tsi->dnlal0) & 0x1)?"Yes":"No");
  
  printf("(Current) linked list PCI address: 0x%08x%08x\n", bswap(tsi->dclau0), bswap(tsi->dclal0));  
  printf("DMA count:                         0x%08x\n", bswap(tsi->dcnt0));  
  printf("2eSST broadcast select:            0x%08x\n", bswap(tsi->ddbs0) & 0x1fffff);  
  
  printf("=========================================================================\n");
}


/****************/
void decberr(void)
/****************/
{  
  u_int data;
  
  printf("\n=========================================================================\n");
  printf("NOTE: After dumping the information the registers will be reset to prepare them for capturing the next error\n");
  
  data = bswap(tsi->veat);
  printf("veat (0x%08x):\n", data);
  
  printf("Exception status:           %s\n", (data & 0x80000000)?"Error seen":"No error seen");
  
  if (data & 0x80000000)
  {
    printf("Overflow:                   %s\n", (data & 0x80000000)?"Yes":"No");
    printf("2e odd termination:         %s\n", (data & 0x200000)?"Yes":"No");
    printf("2e slave termination:       %s\n", (data & 0x100000)?"Yes":"No");
    printf("BERR:                       %s\n", (data & 0x80000)?"Yes":"No");
    printf("LWORD:                      %s\n", (data & 0x40000)?"Asserted":"Not asserted");
    printf("WRITE:                      %s\n", (data & 0x20000)?"Asserted":"Not asserted");
    printf("IACK:                       %s\n", (data & 0x10000)?"Asserted":"Not asserted");
    printf("DS1:                        %s\n", (data & 0x8000)?"Asserted":"Not asserted");
    printf("DS0:                        %s\n", (data & 0x4000)?"Asserted":"Not asserted");
    printf("AM code:                    %d\n", (data >> 8) & 0x1f);
    printf("XAM code:                   %d\b", data & 0xff);
    printf("VMEbus address:             0x%08x%08x\n", bswap(tsi->veal), bswap(tsi->veau));

    tsi->veat = bswap(0x20000000);
  }
  
  printf("=========================================================================\n");
}


/***************/
void decirq(void)
/***************/
{  
  u_int data, d1, d2, d3, d4, d5, value;
  printf("\n=========================================================================\n");

  data = bswap(tsi->vicr);
  printf("vicr (0x%08x):\n", data);
  value = (data >> 26) & 0x3;
  if (value == 0) printf("Function of IRQ1:                         Normal interrupts\n");
  if (value == 1) printf("Function of IRQ1:                         Pulse generator\n");
  if (value == 2) printf("Function of IRQ1:                         Programmable clock\n");
  if (value == 3) printf("Function of IRQ1:                         1.02 us clock\n");

  value = (data >> 24) & 0x3;
  if (value == 0) printf("Function of IRQ2:                         Normal interrupts\n");
  if (value == 1) printf("Function of IRQ2:                         Pulse generator\n");
  if (value == 2) printf("Function of IRQ2:                         Programmable clock\n");
  if (value == 3) printf("Function of IRQ2:                         1.02 us clock\n");

  printf("Level of outgoing interrupt:              %d\n", (data >> 12) & 0x7);
  printf("Outgoing interrupt has been acknowledged: %s\n", (data & 0x800)?"Yes":"No");
  printf("Outgoing Interrupt vector:                %d\n", data & 0xff);

  d1 = (bswap(tsi->inten));
  d2 = (bswap(tsi->inteo));
  d3 = (bswap(tsi->ints));
  d4 = (bswap(tsi->intm1));
  d5 = (bswap(tsi->intm2));

  printf("inten = 0x%08x, inteo = 0x%08x, ints = 0x%08x, intm1 = 0x%08x, intm2 = 0x%08x\n", d1, d2, d3, d4, d5);

  printf("\n         DMA1| DMA0| PCI-ERR| VME-ERR| VIEEN| IACKEN| SYSFAIL| ACFAIL| VME7| VME6| VME5| VME4| VME3| VME2| VME1\n");
  printf("=============|=====|========|========|======|=======|========|=======|=====|=====|=====|=====|=====|=====|=====\n");
  printf("Enabled: ");
  printf(" %s|", d1 & 0x02000000?"Yes":" No");
  printf("  %s|", d1 & 0x01000000?"Yes":" No");
  printf("     %s|", d1 & 0x00002000?"Yes":" No");
  printf("     %s|", d1 & 0x00001000?"Yes":" No");
  printf("   %s|", d1 & 0x00000800?"Yes":" No");
  printf("    %s|", d1 & 0x00000400?"Yes":" No");
  printf("     %s|", d1 & 0x00000200?"Yes":" No");
  printf("    %s|", d1 & 0x00000100?"Yes":" No");
  printf("  %s|", d1 & 0x00000080?"Yes":" No");
  printf("  %s|", d1 & 0x00000040?"Yes":" No");
  printf("  %s|", d1 & 0x00000020?"Yes":" No");
  printf("  %s|", d1 & 0x00000010?"Yes":" No");
  printf("  %s|", d1 & 0x00000008?"Yes":" No");
  printf("  %s|", d1 & 0x00000004?"Yes":" No");
  printf("  %s\n", d1 & 0x00000002?"Yes":" No");
 
  printf("En. out: ");
  printf(" %s|", d2 & 0x02000000?"Yes":" No");
  printf("  %s|", d2 & 0x01000000?"Yes":" No");
  printf("     %s|", d2 & 0x00002000?"Yes":" No");
  printf("     %s|", d2 & 0x00001000?"Yes":" No");
  printf("   %s|", d2 & 0x00000800?"Yes":" No");
  printf("    %s|", d2 & 0x00000400?"Yes":" No");
  printf("     %s|", d2 & 0x00000200?"Yes":" No");
  printf("    %s|", d2 & 0x00000100?"Yes":" No");
  printf("  %s|", d2 & 0x00000080?"Yes":" No");
  printf("  %s|", d2 & 0x00000040?"Yes":" No");
  printf("  %s|", d2 & 0x00000020?"Yes":" No");
  printf("  %s|", d2 & 0x00000010?"Yes":" No");
  printf("  %s|", d2 & 0x00000008?"Yes":" No");
  printf("  %s|", d2 & 0x00000004?"Yes":" No");
  printf("  %s\n", d2 & 0x00000002?"Yes":" No");
  
  printf("Pending: ");
  printf(" %s|", d3 & 0x02000000?"Yes":" No");
  printf("  %s|", d3 & 0x01000000?"Yes":" No");
  printf("     %s|", d3 & 0x00002000?"Yes":" No");
  printf("     %s|", d3 & 0x00001000?"Yes":" No");
  printf("   %s|", d3 & 0x00000800?"Yes":" No");
  printf("    %s|", d3 & 0x00000400?"Yes":" No");
  printf("     %s|", d3 & 0x00000200?"Yes":" No");
  printf("    %s|", d3 & 0x00000100?"Yes":" No");
  printf("  %s|", d3 & 0x00000080?"Yes":" No");
  printf("  %s|", d3 & 0x00000040?"Yes":" No");
  printf("  %s|", d3 & 0x00000020?"Yes":" No");
  printf("  %s|", d3 & 0x00000010?"Yes":" No");
  printf("  %s|", d3 & 0x00000008?"Yes":" No");
  printf("  %s|", d3 & 0x00000004?"Yes":" No");
  printf("  %s\n\n", d3 & 0x00000002?"Yes":" No");

  printf("          DMA1| DMA0| PCI-ERR| VME-ERR| VIEEN| IACKEN| SYSFAIL| ACFAIL| VME7| VME6| VME5| VME4| VME3| VME2| VME1\n");
  printf("==============|=====|========|========|======|=======|========|=======|=====|=====|=====|=====|=====|=====|=====\n");
  printf("Mapping: ");
  printf(" "); irqmapdecode(d4, 18);
  printf("| "); irqmapdecode(d4, 16);
  printf("|    "); irqmapdecode(d3, 26);
  printf("|    "); irqmapdecode(d3, 24);
  printf("|  "); irqmapdecode(d3, 22);
  printf("|   "); irqmapdecode(d3, 20);
  printf("|    "); irqmapdecode(d3, 18);
  printf("|   "); irqmapdecode(d3, 16);
  printf("| "); irqmapdecode(d3, 14);
  printf("| "); irqmapdecode(d3, 12);
  printf("| "); irqmapdecode(d3, 10);
  printf("| "); irqmapdecode(d3, 8);
  printf("| "); irqmapdecode(d3, 4);
  printf("| "); irqmapdecode(d3, 2);
  printf("| "); irqmapdecode(d3, 0);
  printf("\n");

  printf("=========================================================================\n");
}


/**************************************/
void irqmapdecode(u_int data, u_int pos)
/**************************************/
{
  u_int mapping = (data >> pos) & 0x3;
  if (mapping == 0) printf("INTA");
  if (mapping == 1) printf("INTB");
  if (mapping == 2) printf("INTC");
  if (mapping == 3) printf("INTD");
}


/****************/
void decmisc(void)
/****************/
{  
  u_int data, data2;
  printf("\n=========================================================================\n");

  data = bswap(tsi->vmctrl);
  printf("vmctrl (0x%08x):\n", data);
  
  if (((data >> 12) & 0x7) == 0) printf("VMEbus time off:      0 us\n");
  if (((data >> 12) & 0x7) == 1) printf("VMEbus time off:      1 us\n");
  if (((data >> 12) & 0x7) == 2) printf("VMEbus time off:      2 us\n");
  if (((data >> 12) & 0x7) == 3) printf("VMEbus time off:      4 us\n");
  if (((data >> 12) & 0x7) == 4) printf("VMEbus time off:      8 us\n");
  if (((data >> 12) & 0x7) == 5) printf("VMEbus time off:      16 us\n");
  if (((data >> 12) & 0x7) == 6) printf("VMEbus time off:      32 us\n");
  if (((data >> 12) & 0x7) == 7) printf("VMEbus time off:      64 us\n");

  if (((data >> 8) & 0x7) == 0) printf("VMEbus time on:       4 us / 128 bytes\n");
  if (((data >> 8) & 0x7) == 1) printf("VMEbus time on:       8 us / 128 bytes\n");
  if (((data >> 8) & 0x7) == 2) printf("VMEbus time on:       16 us / 128 bytes\n");
  if (((data >> 8) & 0x7) == 3) printf("VMEbus time on:       32 us / 256 bytes\n");
  if (((data >> 8) & 0x7) == 4) printf("VMEbus time on:       64 us / 512 bytes\n");
  if (((data >> 8) & 0x7) == 5) printf("VMEbus time on:       128 us / 1024 bytes\n");
  if (((data >> 8) & 0x7) == 6) printf("VMEbus time on:       256 us / 2048 bytes\n");
  if (((data >> 8) & 0x7) == 7) printf("VMEbus time on:       512 us / 4096 bytes\n");

  if (((data >> 3) & 0x3) == 0) printf("VMEbus release mode:  TIME ON or DONE\n");
  if (((data >> 3) & 0x3) == 1) printf("VMEbus release mode:  (TIME ON and REQ) or DONE\n");
  if (((data >> 3) & 0x3) == 2) printf("VMEbus release mode:  (TIME ON and BCLR) or DONE\n");
  if (((data >> 3) & 0x3) == 3) printf("VMEbus release mode:  (TIME ON or DONE) and REQ\n");

  printf("Fair arbitration:     %s\n", (data & 0x4)?"Enabled":"Disabled");
  printf("VMEbus request level: %d\n", data & 0x3);

  data = bswap(tsi->vctrl);
  printf("\nvctrl (0x%08x):\n", data);
  
  printf("No early release of bus: %s\n", (data & 0x100000)?"Enabled":"Disabled");
  printf("2eSST broadcast ID:      %d\n", (data >> 8) & 0x1f);
  printf("16 us arbiter tiomeout   %s\n", (data & 0x80)?"Enabled":"Disabled");
  printf("Arbitratinon mode        %s\n", (data & 0x40)?"Round Robin":"Priority");
  
       if ((data & 0xf) == 0) printf("VMEbus time out:         8 us\n");
  else if ((data & 0xf) == 1) printf("VMEbus time out:         16 us\n");
  else if ((data & 0xf) == 2) printf("VMEbus time out:         32 us\n");
  else if ((data & 0xf) == 3) printf("VMEbus time out:         64 us\n");
  else if ((data & 0xf) == 4) printf("VMEbus time out:         128 us\n");
  else if ((data & 0xf) == 5) printf("VMEbus time out:         256 us\n");
  else if ((data & 0xf) == 6) printf("VMEbus time out:         512 us\n");
  else if ((data & 0xf) == 7) printf("VMEbus time out:         1024 us\n");
  else if ((data & 0xf) == 8) printf("VMEbus time out:         2048 us\n");
  else                       printf("VMEbus time out:         Out of range\n");
  
  data = bswap(tsi->vstat);
  printf("\nvstat (0x%08x):\n", data);
  
  printf("Power up reset:       %s\n", (data & 0x1000)?"Set":"Not set");
  printf("Board fail:           %s\n", (data & 0x800)?"Set":"Not set");
  printf("System fail:          %s\n", (data & 0x400)?"Set":"Not set");
  printf("AC fail:              %s\n", (data & 0x200)?"Set":"Not set");
  printf("Slot 1 functions:     %s\n", (data & 0x100)?"Enabled":"Disabled");
  printf("GA parity:            %d\n", (data >> 5) & 0x1);
  printf("Geographical address: %d\n", data & 0x1f);

  data = bswap(tsi->vmefl);
  printf("\nvmefl (0x%08x):\n", data);
  if (((data >> 24) & 0x3) == 0) printf("Acknowledge delay:  Slow\n");
  if (((data >> 24) & 0x3) == 1) printf("Acknowledge delay:  Medium\n");
  if (((data >> 24) & 0x3) == 2) printf("Acknowledge delay:  Fast\n");
  if (((data >> 24) & 0x3) == 3) printf("Acknowledge delay:  Out of range\n");
  
  printf("Bus grant filter:   %s\n", (data & 0x800)?"Enabled":"Disabled");
  printf("Bus request filter: %s\n", (data & 0x400)?"Enabled":"Disabled");
  printf("Bus clear filter:   %s\n", (data & 0x200)?"Enabled":"Disabled");
  printf("Bus busy filter:    %s\n", (data & 0x100)?"Enabled":"Disabled");
  printf("Acknowledge filter: %s\n", (data & 0x10)?"Enabled":"Disabled");
  printf("Strobe filter:      %s\n", (data & 0x1)?"Enabled":"Disabled");
  
  data = bswap(tsi->gbal);
  data2 = bswap(tsi->gbau);
  printf("\ngbau (0x%08x) - gbal (0x%08x):\n", data2, data);
  printf("VMEbus base address of the registers of the Tsi148: 0x%08x%08x\n", data2, data);
  
  data = bswap(tsi->gcsrat);
  printf("\ngcsrat (0x%08x):\n", data);
  printf("Mapping of TIS148 registers to VMEbus: %s\n", (data & 0x80)?"Enabled":"Disabled");

  data2 = (data >> 4) & 0x7;
       if (data == 0) printf("Address space for Tis148 registers:    A16\n");
  else if (data == 1) printf("Address space for Tis148 registers:    A24\n");
  else if (data == 2) printf("Address space for Tis148 registers:    A32\n");
  else if (data == 3) printf("Address space for Tis148 registers:    A64\n");
  else                printf("Address space for Tis148 registers:    Undefined\n");
  
  printf("Response to SUPERVISOR AM codes:      %s\n", (data & 0x8)?"Yes":" No");
  printf("Response to USER AM codes:            %s\n", (data & 0x4)?"Yes":" No");
  printf("Response to PROGRAM AM codes:         %s\n", (data & 0x2)?"Yes":" No");
  printf("Response to DATA AM codes:            %s\n", (data & 0x1)?"Yes":" No");

  data = bswap(tsi->crat);
  printf("\ncrat (0x%08x):\n", data);
  printf("The CR/CSR space is: %s\n", (data & 0x80)?"Enabled":"Disabled");

  data = bswap(tsi->devi_veni_2);
  printf("\ndevi_veni_2 (0x%08x):\n", data);
  
  data = bswap(tsi->gctrl);
  printf("\ngctrl (0x%08x):\n", data);
  printf("Local reset:         %s\n", (data & 0x80000000)?"In progress":"Not active");
  printf("SYSFAIL enabled:     %s\n", (data & 0x40000000)?"Yes":"No");
  printf("Board fail:          %s\n", (data & 0x20000000)?"Asserted":"No");
  printf("Slot1 functionality: %s\n", (data & 0x10000000)?"Active":"Not active");
  printf("GA Parity:           %d\n", (data >> 13) & 0x1);
  printf("GA slot number:      %d\n", (data >> 8) & 0x1f);
  printf("Tsi148 revision:     %d\n", data & 0xff);

  data = bswap(tsi->csrbcr);
  printf("\ncsrbcr (0x%08x):\n", data);

  data = bswap(tsi->csrbsr);
  printf("\ncsrbsr (0x%08x):\n", data);

  data = bswap(tsi->cbar);
  printf("\ncbar (0x%08x):\n", data);
  printf("CR/CSR VMEbus base address: 0x%08x\n", (data & 0xf8) << 16);
  
  printf("=========================================================================\n");
}


/****************/
void decmmap(void)
/****************/
{
  printf("\n=======================================================================================\n");
 
  printf("LSI|                       VME address range|                       PCI address range|  EN| MRPFD|     PFS|     2eSST|        TM|       DBW|  SUP|  PGM|     AM|    OTBS\n");
  printf("===|========================================|========================================|====|======|========|==========|==========|==========|=====|=====|=======|========\n");
  decminf(0, bswap(tsi->otsau0), bswap(tsi->otsal0), bswap(tsi->oteau0), bswap(tsi->oteal0), bswap(tsi->otofu0), bswap(tsi->otofl0), bswap(tsi->otbs0), bswap(tsi->otat0));
  decminf(1, bswap(tsi->otsau1), bswap(tsi->otsal1), bswap(tsi->oteau1), bswap(tsi->oteal1), bswap(tsi->otofu1), bswap(tsi->otofl1), bswap(tsi->otbs1), bswap(tsi->otat1));
  decminf(2, bswap(tsi->otsau2), bswap(tsi->otsal2), bswap(tsi->oteau2), bswap(tsi->oteal2), bswap(tsi->otofu2), bswap(tsi->otofl2), bswap(tsi->otbs2), bswap(tsi->otat2));
  decminf(3, bswap(tsi->otsau3), bswap(tsi->otsal3), bswap(tsi->oteau3), bswap(tsi->oteal3), bswap(tsi->otofu3), bswap(tsi->otofl3), bswap(tsi->otbs3), bswap(tsi->otat3));
  decminf(4, bswap(tsi->otsau4), bswap(tsi->otsal4), bswap(tsi->oteau4), bswap(tsi->oteal4), bswap(tsi->otofu4), bswap(tsi->otofl4), bswap(tsi->otbs4), bswap(tsi->otat4));
  decminf(5, bswap(tsi->otsau5), bswap(tsi->otsal5), bswap(tsi->oteau5), bswap(tsi->oteal5), bswap(tsi->otofu5), bswap(tsi->otofl5), bswap(tsi->otbs5), bswap(tsi->otat5));
  decminf(6, bswap(tsi->otsau6), bswap(tsi->otsal6), bswap(tsi->oteau6), bswap(tsi->oteal6), bswap(tsi->otofu6), bswap(tsi->otofl6), bswap(tsi->otbs6), bswap(tsi->otat6));
  decminf(7, bswap(tsi->otsau7), bswap(tsi->otsal7), bswap(tsi->oteau7), bswap(tsi->oteal7), bswap(tsi->otofu7), bswap(tsi->otofl7), bswap(tsi->otbs7), bswap(tsi->otat7));
  printf("=======================================================================================\n");
}


/****************/
void setmmap(void)
/****************/
{
  static u_int value;
  u_int ret, loop, dummy[9];

  printf("\n=======================================================================================\n");
  value = bswap(tsi->otsau0);
  printf("Enter new value for otsau0: ");
  value = gethexd(value);
  tsi->otsau0 = bswap(value);
  
  value = bswap(tsi->otsal0);
  printf("Enter new value for otsal0: ");
  value = gethexd(value);
  tsi->otsal0 = bswap(value);
  
  value = bswap(tsi->oteau0);
  printf("Enter new value for oteau0: ");
  value = gethexd(value);
  tsi->oteau0 = bswap(value);
  
  value = bswap(tsi->oteal0);
  printf("Enter new value for oteal0: ");
  value = gethexd(value);
  tsi->oteal0 = bswap(value);
  
  value = bswap(tsi->otofu0);
  printf("Enter new value for otofu0: ");
  value = gethexd(value);
  tsi->otofu0 = bswap(value);
  
  value = bswap(tsi->otofl0);
  printf("Enter new value for otofl0: ");
  value = gethexd(value);
  tsi->otofl0 = bswap(value);
  
  tsi->otbs0 = 0;
  
  printf("Enable (1) or disable (0) the mapping?: ");
  value = getdecd(0);
  if (value == 1)
    tsi->otat0 =  bswap(0x80000042);
  else
    tsi->otat0 =  bswap(0x00000042);
    
  // Update the driver
  for(loop = 0; loop < 9; loop++)
    dummy[loop] = 0;
    
  ret = VME_Update(dummy);
  if (ret)
  {
    VME_ErrorPrint(ret);
    exit(-1);
  } 

  printf("=======================================================================================\n");
    
}


/*******************/
void decmmapraw(void)
/*******************/
{
  printf("\n=======================================================================================\n");
  
  printf("Decoder 0: ");
  printf("otsau = 0x%08x, otsal = 0x%08x, oteau = 0x%08x, oteal = 0x%08x ", bswap(tsi->otsau0), bswap(tsi->otsal0), bswap(tsi->oteau0), bswap(tsi->oteal0));
  printf("otofu = 0x%08x, otofl = 0x%08x, otbs = 0x%08x, otat = 0x%08x\n", bswap(tsi->otofu0), bswap(tsi->otofl0), bswap(tsi->otbs0), bswap(tsi->otat0));

  printf("Decoder 1: ");
  printf("otsau = 0x%08x, otsal = 0x%08x, oteau = 0x%08x, oteal = 0x%08x ", bswap(tsi->otsau1), bswap(tsi->otsal1), bswap(tsi->oteau1), bswap(tsi->oteal1));
  printf("otofu = 0x%08x, otofl = 0x%08x, otbs = 0x%08x, otat = 0x%08x\n", bswap(tsi->otofu1), bswap(tsi->otofl1), bswap(tsi->otbs1), bswap(tsi->otat1));

  printf("Decoder 2: ");
  printf("otsau = 0x%08x, otsal = 0x%08x, oteau = 0x%08x, oteal = 0x%08x ", bswap(tsi->otsau2), bswap(tsi->otsal2), bswap(tsi->oteau2), bswap(tsi->oteal2));
  printf("otofu = 0x%08x, otofl = 0x%08x, otbs = 0x%08x, otat = 0x%08x\n", bswap(tsi->otofu2), bswap(tsi->otofl2), bswap(tsi->otbs2), bswap(tsi->otat2));

  printf("Decoder 3: ");
  printf("otsau = 0x%08x, otsal = 0x%08x, oteau = 0x%08x, oteal = 0x%08x ", bswap(tsi->otsau3), bswap(tsi->otsal3), bswap(tsi->oteau3), bswap(tsi->oteal3));
  printf("otofu = 0x%08x, otofl = 0x%08x, otbs = 0x%08x, otat = 0x%08x\n", bswap(tsi->otofu3), bswap(tsi->otofl3), bswap(tsi->otbs3), bswap(tsi->otat3));

  printf("Decoder 4: ");
  printf("otsau = 0x%08x, otsal = 0x%08x, oteau = 0x%08x, oteal = 0x%08x ", bswap(tsi->otsau4), bswap(tsi->otsal4), bswap(tsi->oteau4), bswap(tsi->oteal4));
  printf("otofu = 0x%08x, otofl = 0x%08x, otbs = 0x%08x, otat = 0x%08x\n", bswap(tsi->otofu4), bswap(tsi->otofl4), bswap(tsi->otbs4), bswap(tsi->otat4));

  printf("Decoder 5: ");
  printf("otsau = 0x%08x, otsal = 0x%08x, oteau = 0x%08x, oteal = 0x%08x ", bswap(tsi->otsau5), bswap(tsi->otsal5), bswap(tsi->oteau5), bswap(tsi->oteal5));
  printf("otofu = 0x%08x, otofl = 0x%08x, otbs = 0x%08x, otat = 0x%08x\n", bswap(tsi->otofu5), bswap(tsi->otofl5), bswap(tsi->otbs5), bswap(tsi->otat5));

  printf("Decoder 6: ");
  printf("otsau = 0x%08x, otsal = 0x%08x, oteau = 0x%08x, oteal = 0x%08x ", bswap(tsi->otsau6), bswap(tsi->otsal6), bswap(tsi->oteau6), bswap(tsi->oteal6));
  printf("otofu = 0x%08x, otofl = 0x%08x, otbs = 0x%08x, otat = 0x%08x\n", bswap(tsi->otofu6), bswap(tsi->otofl6), bswap(tsi->otbs6), bswap(tsi->otat6));

  printf("Decoder 7: ");
  printf("otsau = 0x%08x, otsal = 0x%08x, oteau = 0x%08x, oteal = 0x%08x ", bswap(tsi->otsau7), bswap(tsi->otsal7), bswap(tsi->oteau7), bswap(tsi->oteal7));
  printf("otofu = 0x%08x, otofl = 0x%08x, otbs = 0x%08x, otat = 0x%08x\n", bswap(tsi->otofu7), bswap(tsi->otofl7), bswap(tsi->otbs7), bswap(tsi->otat7));

  printf("=======================================================================================\n");
}


/****************/
void decsmap(void)
/****************/
{

  printf("\n=======================================================================================\n");
 

  printf("LSI|                      VME address range|                      PCI address range|  EN|    TH| VFS| 2eSST_SP| BCAST| 2eSST| 2eVME| MBLT| BLT| A-SP| SUP| USR| PGM| DTA\n");
  printf("===|=======================================|=======================================|====|======|====|=========|======|======|======|=====|====|=====|====|====|====|====\n");
  decsinf(0, bswap(tsi->itsau0), bswap(tsi->itsal0), bswap(tsi->iteau0), bswap(tsi->iteal0), bswap(tsi->itofu0), bswap(tsi->itofl0), bswap(tsi->itat0));
  decsinf(1, bswap(tsi->itsau1), bswap(tsi->itsal1), bswap(tsi->iteau1), bswap(tsi->iteal1), bswap(tsi->itofu1), bswap(tsi->itofl1), bswap(tsi->itat1));
  decsinf(2, bswap(tsi->itsau2), bswap(tsi->itsal2), bswap(tsi->iteau2), bswap(tsi->iteal2), bswap(tsi->itofu2), bswap(tsi->itofl2), bswap(tsi->itat2));
  decsinf(3, bswap(tsi->itsau3), bswap(tsi->itsal3), bswap(tsi->iteau3), bswap(tsi->iteal3), bswap(tsi->itofu3), bswap(tsi->itofl3), bswap(tsi->itat3));
  decsinf(4, bswap(tsi->itsau4), bswap(tsi->itsal4), bswap(tsi->iteau4), bswap(tsi->iteal4), bswap(tsi->itofu4), bswap(tsi->itofl4), bswap(tsi->itat4));
  decsinf(5, bswap(tsi->itsau5), bswap(tsi->itsal5), bswap(tsi->iteau5), bswap(tsi->iteal5), bswap(tsi->itofu5), bswap(tsi->itofl5), bswap(tsi->itat5));
  decsinf(6, bswap(tsi->itsau6), bswap(tsi->itsal6), bswap(tsi->iteau6), bswap(tsi->iteal6), bswap(tsi->itofu6), bswap(tsi->itofl6), bswap(tsi->itat6));
  decsinf(7, bswap(tsi->itsau7), bswap(tsi->itsal7), bswap(tsi->iteau7), bswap(tsi->iteal7), bswap(tsi->itofu7), bswap(tsi->itofl7), bswap(tsi->itat7));
  printf("=======================================================================================\n");
}


/**************************************************************************************************************/  
void decsinf(u_int nr, u_int itsau, u_int itsal, u_int iteau, u_int iteal, u_int itofu, u_int itofl, u_int itat)
/**************************************************************************************************************/  
{
  unsigned long long v_start, v_end, p_start, p_end, offset;
  u_int awidth, data;
  
  awidth = (itat >> 4) & 0x3;
  
  if (awidth == 4 || awidth == 2)
  {
    v_start = ((unsigned long long)itsau << 32) | (itsal & 0xffff0000);
    v_end = ((unsigned long long)iteau << 32) | (iteal & 0xffff0000);
    offset = ((unsigned long long)itofu << 32) | (itofl & 0xffff0000);
  }
  
  else if (awidth == 1)
  {
    v_start = ((unsigned long long)itsau << 32) | (itsal & 0x00fff000);
    v_end = ((unsigned long long)iteau << 32) | (iteal & 0x00fff000);
    offset = ((unsigned long long)itofu << 32) | (itofl & 0xfffff000);
  }

  else 
  {
    v_start = ((unsigned long long)itsau << 32) | (itsal & 0x0000fff0);
    v_end = ((unsigned long long)iteau << 32) | (iteal & 0x0000fff0);
    offset = ((unsigned long long)itofu << 32) | (itofl & 0xfffffff0);
  }

  p_start = v_start + offset;
  p_end = v_end + offset;
  
  printf("  %d|", nr);
  printf("0x%016llx - 0x%016llx|", v_start, v_end); 
  printf("0x%016llx - 0x%016llx|", p_start, p_end);
  printf(" %s|", (itat & 0x80000000)?"Yes":" No");   
  printf(" %s|", (itat & 0x40000)?" half":"empty");
  printf("   %d|", (itat >> 16) & 0x3);   

  data = (itat >> 12) & 0x7; 
       if (data == 0) printf(" 160 MB/s|");  
  else if (data == 1) printf(" 267 MB/s|");
  else if (data == 2) printf(" 320 MB/s|");
  else                printf("undefined|"); 
   
  printf(" %s|", (itat & 0x800)?"  Yes":"   No");
  printf(" %s|", (itat & 0x400)?"  Yes":"   No");
  printf(" %s|", (itat & 0x200)?"  Yes":"   No");
  printf(" %s|", (itat & 0x100)?" Yes":"  No");
  printf(" %s|", (itat & 0x80)?"Yes":" No");

  data = (itat >> 4) & 0x7; 
       if (data == 0) printf("  A16|");  
  else if (data == 1) printf("  A24|");
  else if (data == 2) printf("  A32|");
  else if (data == 4) printf("  A64|");
  else                printf("  ???|"); 

  printf(" %s|", (itat & 0x8)?"Yes":" No");
  printf(" %s|", (itat & 0x4)?"Yes":" No");
  printf(" %s|", (itat & 0x2)?"Yes":" No");
  printf(" %s\n", (itat & 0x1)?"Yes":" No");
}


/**************************************************************************************************************************/  
void decminf(u_int nr, u_int otsau, u_int otsal, u_int oteau, u_int oteal, u_int otofu, u_int otofl, u_int otbs, u_int otat)
/**************************************************************************************************************************/  
{
  unsigned long long v_start, v_end, p_start, p_end, offset;
  u_int data;
  
  p_start = ((unsigned long long)otsau << 32) | (otsal & 0xffff0000);
  p_end = ((unsigned long long)oteau << 32) | ((oteal & 0xffff0000) + 0xffff);
  offset = ((unsigned long long)otofu << 32) | (otofl & 0xffff0000);

  v_start = p_start + offset;
  v_end = p_end + offset;

  printf("  %d|", nr);
  printf(" 0x%016llx - 0x%016llx|", v_start, v_end); 
  printf(" 0x%016llx - 0x%016llx|", p_start, p_end);
  printf(" %s|", (otat & 0x80000000)?"Yes":" No");   
  printf("   %s|", (otat & 0x00400000)?"Yes":" No");   

  data = (otat >> 16) & 0x3;
  if (data == 0) printf(" 2 lines|");
  if (data == 1) printf(" 4 lines|");
  if (data == 2) printf(" 8 lines|");
  if (data == 3) printf("16 lines|");
  
  data = (otat >> 11) & 0x7;
  if      (data == 0) printf("  160 MB/s|");
  else if (data == 1) printf("  267 MB/s|");
  else if (data == 2) printf("  320 MB/s|");
  else                printf(" undefined|");

  data = (otat >> 8) & 0x7;
  if      (data == 0) printf("    single|");
  else if (data == 1) printf("       BLT|");
  else if (data == 2) printf("      MBLT|");
  else if (data == 3) printf("     2eVME|");
  else if (data == 4) printf("     2eSST|");
  else if (data == 5) printf("    b-cast|");
  else                printf(" undefined|");
  
  data = (otat >> 6) & 0x3;
  if      (data == 0) printf("    16 bit|");
  else if (data == 1) printf("    32 bit|");
  else                printf(" undefined|");
  
  printf(" %s|", (otat & 0x00000020)?"Sup.":"User");   
  printf(" %s|", (otat & 0x00000010)?"Prgm":"Data");   

  data = otat & 0xf;
  if      (data == 0)  printf("    A16|");
  else if (data == 1)  printf("    A24|");
  else if (data == 2)  printf("    A32|");
  else if (data == 4)  printf("    A64|");
  else if (data == 5)  printf(" CR/CSR|");
  else if (data == 8)  printf("  User1|");
  else if (data == 9)  printf("  User2|");
  else if (data == 10) printf("  User3|");
  else if (data == 11) printf("  User4|");
  else                 printf(" undef.|");  
  
  printf(" 0x%05x\n", otbs & 0xfffff);
}


/***************/
int vmehelp(void)
/***************/
{
  printf("\n=========================================================================\n");
  printf("Some functions in this menu require more than 80 characters per\n");
  printf("line to display the full information. It is therefore recommended\n");
  printf("to use the 132 characters/line mode of your vt100 terminal.\n");
  printf("To set a FALCO to 132 characters/line type 'setUp' followed\n");
  printf("by 'F10' and then edit the third item of the first line.\n");
  printf("\n");
  printf("Display formats\n");
  printf("Addresses are in Hex.\n");
  printf("\n");
  printf("Abbreviations used:\n");
  printf("EN       = Map decoder enabled\n");
  printf("MRPFD    = Memory read prefetch\n");
  printf("PFS      = Prefetch size\n");
  printf("2eSST    = 2 edge source synchronous transfer\n");
  printf("TM       = Transfer mode\n");
  printf("DBW      = Data bus (VMEbus) width\n");
  printf("SUP      = Supervisor more\n");
  printf("PGM      = Program mode\n");
  printf("AM       = Address modifier\n");
  printf("OTBS     = 2eSST broadcast mask\n");
  printf("TH       = Prefetch threshold\n");
  printf("VFS      = virtual FIFO size\n");
  printf("2eSST_SP = 2eSST speed\n");
  printf("BCAST    = 2eSST broadcast\n");
  printf("2eVME    = 2 edge VMEbus block transfers\n");
  printf("MBLT     = Multiplexed (D64) block transfer\n");
  printf("BLT      = (D32) block transfer\n");
  printf("A-SP     = Address space\n");
  printf("SUP      = Supervisor AM code\n");
  printf("USR      = User AM code\n");
  printf("PGM      = Program AM code\n");
  printf("DTA      = Data AM code\n");
  printf("Call Markus Joos, 72364, if you need more help\n");
  printf("=========================================================================\n\n");
  return(0);
}


