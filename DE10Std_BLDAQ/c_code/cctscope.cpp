// $Id$
/****************************************************************/
/*                                              		*/
/*  file: cctscope.c                            		*/
/*                                              		*/
/*  cctscope decodes and displays the contents  		*/
/*  of various registers of PSE/C12 and VP/PMC boards   	*/
/*  from Concurrent Technologies                                */
/*                                              		*/
/*  Author: Markus Joos, CERN-ECP               		*/
/*                                              		*/
/*  25.Aug.00  MAJO  created                   		  	*/
/*   1.Mar.02  MAJO  ported to VME_RCC driver			*/
/********C 2016 - A nickel program worth a dime******************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include "rcc_error/rcc_error.h"
#include "vme_rcc/vme_rcc.h"
#include "io_rcc/io_rcc.h"
#include "DFDebug/DFDebug.h"
#include "ROSGetInput/get_input.h"
#include "rcc_time_stamp/tstamp.h"
#include "vme_rcc/universe.h"


/*PSE / VP-PMC special registers*/
#define CSR0           0x210
#define CSR1           0x212
#define CSR2           0x216
#define CSR3           0x217
#define CMOSA          0x70
#define CMOSD          0x71
#define BID1           0x35
#define BID2           0x36

/*VP-100 special registers*/
#define CSR0_100       0x210
#define CSR1_100       0x212
#define CSR2_100       0x211
#define CSR3_100       0x217
#define BERR_100       0x213

/*VP-110 special registers*/
#define CSR4_110       0x215

/*VP-315/317 special registers*/
#define CSR0_315       0x210
#define CSR1_315       0x212
#define CSR2_315       0x211
#define CSR3_315       0x21d
#define BERR_315       0x213
#define GPIO_315       0x31c
#define SERI_315       0x31d
#define SLOT_315       0x31e


// change these to find the TU chip
#define _BUS 0 
#define _DEVICE 0xa

// prototypes
void SigQuitHandler(int signum);
int modi(void);
int disallmm(void);
int testvme(void);
int slavemap(void);
int mastermap(void);
int setswap(void);
int hwconf(void);
int berrget(void);
int berrhand(void);
int mainhelp(void);
int vmemenu(void);
int decirq(void);
int decmisc(void);
int decpci(void);
int decslave(void);
int decslsi(void);
int decbma(void);
int decbma(void);
int decmaster(void);
int decminf(int no, int d1, int d2, int d3, int d4);
int decsinf(int no, int d1, int d2, int d3, int d4);
int vmehelp(void);
int setdebug(void);
int u2spec(void);
void dumpioctl(void);

// globals
universe_regs_t *uni;
int cont, btype, utype, iobase;
void *unip;
u_long unibase;



/*********************************/
void SigQuitHandler(int /*signum*/)
/*********************************/
{
  cont=0;
}


/******************/
void dumpioctl(void)
/******************/
{
  std::cout << "VMEMASTERMAP             is " << VMEMASTERMAP << std::endl;
  std::cout << "VMEMASTERUNMAP           is " << VMEMASTERUNMAP << std::endl;          
  std::cout << "VMEMASTERMAPDUMP         is " << VMEMASTERMAPDUMP << std::endl;
  std::cout << "VMEBERRREGISTERSIGNAL    is " << VMEBERRREGISTERSIGNAL << std::endl;
  std::cout << "VMEBERRINFO              is " << VMEBERRINFO << std::endl;
  std::cout << "VMESLAVEMAP              is " << VMESLAVEMAP << std::endl;
  std::cout << "VMESLAVEMAPDUMP          is " << VMESLAVEMAPDUMP << std::endl;
  std::cout << "VMESCSAFE                is " << VMESCSAFE << std::endl;
  std::cout << "VMEDMASTART              is " << VMEDMASTART << std::endl;
  std::cout << "VMEDMAPOLL               is " << VMEDMAPOLL << std::endl;
  std::cout << "VMEDMADUMP               is " << VMEDMADUMP << std::endl;
  std::cout << "VMELINK                  is " << VMELINK << std::endl;
  std::cout << "VMEINTENABLE             is " << VMEINTENABLE << std::endl;
  std::cout << "VMEINTDISABLE            is " << VMEINTDISABLE << std::endl;
  std::cout << "VMEWAIT                  is " << VMEWAIT << std::endl;
  std::cout << "VMEREGISTERSIGNAL        is " << VMEREGISTERSIGNAL << std::endl;
  std::cout << "VMEINTERRUPTINFOGET      is " << VMEINTERRUPTINFOGET << std::endl;
  std::cout << "VMEUNLINK                is " << VMEUNLINK << std::endl;
  std::cout << "VMEUPDATE                is " << VMEUPDATE << std::endl;
  std::cout << "VMESYSFAILUNLINK         is " << VMESYSFAILUNLINK << std::endl;
  std::cout << "VMESYSFAILWAIT           is " << VMESYSFAILWAIT << std::endl;
  std::cout << "VMESYSFAILREGISTERSIGNAL is " << VMESYSFAILREGISTERSIGNAL << std::endl;
  std::cout << "VMESYSFAILLINK           is " << VMESYSFAILLINK << std::endl;
  std::cout << "VMESYSFAILREENABLE       is " << VMESYSFAILREENABLE << std::endl;
  std::cout << "VMESYSFAILPOLL           is " << VMESYSFAILPOLL << std::endl;
  std::cout << "VMESYSFAILSET            is " << VMESYSFAILSET << std::endl;
  std::cout << "VMESYSFAILRESET          is " << VMESYSFAILRESET << std::endl;
  std::cout << "VMESYSRST                is " << VMESYSRST << std::endl;
  std::cout << "VMESBCTYPE               is " << VMESBCTYPE << std::endl;
  std::cout << "VMETEST                  is " << VMETEST << std::endl;
  std::cout << "VMEMASTERMAPCRCSR        is " << VMEMASTERMAPCRCSR << std::endl;
}


/************/
int main(void)
/************/
{
  static int fun = 1;
  unsigned int id, ret;
  static struct sigaction sa;
  unsigned char d1;
  u_long virtaddr;
  u_char *id_ptr;
  int stat, i = 0;

  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sa.sa_handler = SigQuitHandler;
  stat = sigaction(SIGQUIT, &sa, NULL);
  if (stat < 0)
  {
    printf("Cannot install signal handler (error=%d)\n", stat);
    exit(0);
  }

  ret = VME_Open();
  if (ret != VME_SUCCESS)
  {
    VME_ErrorPrint(ret);
    exit(-1);
  }
  
  ret = IO_Open();
  if (ret)
  {
    rcc_error_print(stdout, ret);
    exit(-1);
  }
  
  // Read the board identification of VPExx/Bxx
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
      printf("Board Id Structure Version:%d\n", id_ptr[8]);   //VPExx = 1, VPBxx = 2 
      printf("Board Type:%d\n", id_ptr[9]);                   //VPExx = 2, VPBxx = 2
      printf("Board Id:%d,%d\n", id_ptr[10], id_ptr[11]);     //VPExx = 0x2b, VPBxx = 0x29
      if (id_ptr[8] == 1 && id_ptr[9] == 2 && id_ptr[10] == 0x2b && id_ptr[11] == 0) 
        btype = 11;   
      if (id_ptr[8] == 2 && id_ptr[9] == 2 && id_ptr[10] == 0x29 && id_ptr[11] == 0) 
        btype = 12;   
    }
    if (btype != 0)
      break;      
    id_ptr++;
  }
  
  // Read the board identification of VP110/315/417
  if (btype == 0)
  {
    ret = IO_IOPokeUChar(CMOSA, BID1);
    if (ret)
    {
      rcc_error_print(stdout, ret);
      return(-1);
    }
    ret = IO_IOPeekUChar(CMOSD, &d1);
    if (ret)
    {
      rcc_error_print(stdout, ret);
      return(-1);
    }

    if (!(d1&0x80))
    {
      printf("Unable to determine board type\n");
      exit(-1);
    }

    ret = IO_IOPokeUChar(CMOSA, BID2);
    if (ret)
    {
      rcc_error_print(stdout, ret);
      return(-1);
    }
    ret = IO_IOPeekUChar(CMOSD, &d1);
    if (ret)
    {
      rcc_error_print(stdout, ret);
      return(-1);
    }

    d1 &= 0x1f;  // Mask board ID bits
    btype = 0;
	 if (d1 == 2) btype = 1;
    else if (d1 == 3) btype = 1;
    else if (d1 == 4) btype = 1;
    else if (d1 == 5) btype = 2;
    else if (d1 == 6) btype = 3;
    else if (d1 == 7) btype = 4;
    else if (d1 == 8) btype = 5;
    else if (d1 == 11) btype = 6;
    else if (d1 == 13) btype = 7;
    else if (d1 == 14) btype = 8;
    else if (d1 == 15) btype = 9;
    else if (d1 == 23) btype = 10;
  }

  if (!btype)
  {
    printf("This type of CCT board is not supported\n");
    exit(-1);
  }

  ret = VME_UniverseMap(&unibase);
  if (ret != VME_SUCCESS)
  {
    VME_ErrorPrint(ret);
    exit(-1);
  }

  uni = (universe_regs_t *)unibase;
  
  id = (uni->pci_class) & 0xff;   	//mask Revision ID
  if (id == 0)
    utype = 1;				//Universe I
  else if (id == 1)
    utype = 2;				//Universe II
  else if (id == 2)
    utype = 2;				//Universe IIb
  else
  {
    printf("pci_class is 0x%08x\n", uni->pci_class);
    printf("Universe revision is %d\n", id);
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    printf("WARNING: The revision of the Universe chip is unknown\n");
    printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
    utype = 2;
  }

  if (btype == 3 || btype == 5 || btype == 6 || btype == 7 || btype == 8 || btype == 9 || btype == 10)
  {
    // Enable VMEbus BERR capturing
    ret = IO_IOPokeUChar(BERR_100, 0x1);
    if (ret)
    {
      rcc_error_print(stdout, ret);
      return(-1);
    }
  printf("VMEbus BERR capturing enabled\n");
  }
  
  fun = 1;
  if (btype == 1)
    printf("\n\n\nThis is CCTSCOPE for a Concurrent Technologies VP-PSE under Linux\n");
  if (btype == 2)
    printf("\n\n\nThis is CCTSCOPE for a Concurrent Technologies VP-PMC under Linux\n");
  if (btype == 3)
    printf("\n\n\nThis is CCTSCOPE for a Concurrent Technologies VP-100 under Linux\n");
  if (btype == 4)
    printf("\n\n\nThis is CCTSCOPE for a Concurrent Technologies VP-CP1 under Linux\n"); 
  if (btype == 5)
    printf("\n\n\nThis is CCTSCOPE for a Concurrent Technologies VP-110 under Linux\n");  
  if (btype == 6)
    printf("\n\n\nThis is CCTSCOPE for a Concurrent Technologies VP-315 under Linux\n"); 
  if (btype == 7)
    printf("\n\n\nThis is CCTSCOPE for a Concurrent Technologies VP-317 under Linux\n"); 
  if (btype == 8)
    printf("\n\n\nThis is CCTSCOPE for a Concurrent Technologies VP-325 under Linux\n"); 
  if (btype == 9)
    printf("\n\n\nThis is CCTSCOPE for a Concurrent Technologies VP-327 under Linux\n"); 
  if (btype == 10)
    printf("\n\n\nThis is CCTSCOPE for a Concurrent Technologies VP-417 under Linux\n"); 
  if (btype == 11)
    printf("\n\n\nThis is CCTSCOPE for a VPE2x\n");
  if (btype == 12)
    printf("\n\n\nThis is CCTSCOPE for a VPB1x\n");
  printf("==================================================================\n");
 
  while(fun != 0)
  {
    printf("\n");
    printf("Select an option:\n");
    printf("   1 Help                        2 Decode VME registers\n");
    printf("   3 Modify VME registers        4 Show special registers\n");
    printf("   5 Toggle VMEbus BERR support  6 Read VMEbus BERR information\n");
    printf("   7 Set VMEbus byte swapping    8 Test VMEbus\n");
    printf("   9 Set debug parameters       10 Dump ioctl codes\n");
    printf("   0 Quit\n");
    printf("Your choice: ");
    fun = getdecd(fun);
    if (fun == 1) mainhelp();
    if (fun == 2) vmemenu();
    if (fun == 3) modi();
    if (fun == 4) hwconf();
    if (fun == 5) berrhand();
    if (fun == 6) berrget();
    if (fun == 7) setswap();
    if (fun == 8) testvme();
    if (fun == 9) setdebug();
    if (fun == 10) dumpioctl();
  }

  ret = VME_UniverseUnmap(unibase);
  if (ret != VME_SUCCESS)
    VME_ErrorPrint(ret);

  ret = VME_Close();
  if (ret != VME_SUCCESS)
    VME_ErrorPrint(ret);  
    
  ret = IO_Close();
  if (ret)
    rcc_error_print(stdout, ret);
}


/****************/
int setdebug(void)
/****************/
{
  static unsigned int dblevel = 0, dbpackage = DFDB_VMERCC;
  
  printf("Enter the debug level: ");
  dblevel = getdecd(dblevel);
  printf("Enter the debug package: ");
  dbpackage = getdecd(dbpackage);
  DF::GlobalDebugSettings::setup(dblevel, dbpackage);
  return(0);
}


/***************/
int sysfail(void)
/***************/
{
  uni->vcsr_clr = 0x40000000;
  printf("Done\n");
  return(0);
}


/************/
int modi(void)
/************/
{
  static int fun;
  unsigned int status, dummy[8];
  
  fun = 1;
  printf("\n=========================================================================\n");
  while(fun != 0)
  {
    printf("\n");
    printf("Select an option:\n");
    printf("   1 Create slave mapping         2 Create master mapping\n");
    printf("   3 Disable all master mappings  4 Disable SYSFail\n");
    printf("   5 Program the U2SPEC register\n");
    printf("   0 Quit\n");
    printf("Your choice: ");
    fun=getdecd(fun);
    if (fun == 1) slavemap();
    if (fun == 2) mastermap();
    if (fun == 3) disallmm();
    if (fun == 4) sysfail();
    if (fun == 5) u2spec();

    // Make the VME_RCC driver aware of the changes
    dummy[0] = 0; //Do not update IRQ modes
    status = VME_Update(&dummy[0]);
    if (status)
    {
      VME_ErrorPrint(status);
      exit(-1);
    }  
    printf("VME_RCC driver synchronized\n");
  }
  printf("=========================================================================\n\n");
  return(0);
}


/**************/
int u2spec(void)
/**************/
{
  int data, dsf, asf, dif, t11, t27, t28a, t28b;
  
  data = uni->pci_class & 0xff;
  if ((data != 1) && (data != 2))
  {
    printf("The Universe chip on your SBC is of revision %d. Therefore it does not support the U2SPEC register\n", data);
    return(-1);
  }

  data = uni->u2spec;
  printf("The U2SPEC register is currently set to 0x%08x\n", data);

  printf("Enable  data strobe filtering                       (1=yes  0=no): ");
  dsf = getdecd(0);
  printf("Enable address strobe filtering                     (1=yes  0=no): ");
  asf = getdecd(0);
  printf("Select VME DTACK inactive filter                 (1=fast  0=slow): ");
  dif = getdecd(0);
  printf("Select master T11 (DS high timing)            (1=fast  0=default): ");
  t11 = getdecd(0);
  printf("Select master T27 (DS negation) (2=no delay  1=faster  0=default): ");
  t27 = getdecd(0);
  printf("Select slave T28 (DS->DTACK posted write)     (1=fast  0=default): ");
  t28a = getdecd(0);
  printf("Select slave T28 (DS->DTACK prefetch read)    (1=fast  0=default): ");
  t28b = getdecd(0);
 
  data = (dsf << 14) | (asf << 13) | (dif << 12) | (t11 << 10) | (t27 << 8) | (t28a << 2) | t28b;
  printf("writing 0x%08x to U2SPEC register\n", data);
  uni->u2spec = data;
  return(0);
}


/****************/
int disallmm(void)
/****************/
{
  printf("\n=========================================================================\n");

  uni->lsi0_ctl = 0;
  uni->lsi1_ctl = 0;
  uni->lsi2_ctl = 0;
  uni->lsi3_ctl = 0;
  uni->lsi4_ctl = 0;
  uni->lsi5_ctl = 0;
  uni->lsi6_ctl = 0;
  uni->lsi7_ctl = 0;
  printf("Done.\n");

  printf("=========================================================================\n\n");
  return(0);
}



/***************/
int testvme(void)
/***************/
{
  volatile unsigned int idata, *iptr;
  unsigned int ret;
  unsigned long virtaddr;
  volatile unsigned short sdata, *sptr;
  volatile unsigned char cdata, *cptr;
  static unsigned int pciaddr = 0x80000000, dire = 0, width = 32;
 
  printf("\n=========================================================================\n");
  printf("Enter the PCI address ");
  pciaddr = gethexd(pciaddr);
  printf("Enter the data width  (8=D8  16=D16  32=D32) ");
  width = getdecd((int) width);
  printf("Enter the direction (0=read  1=write) ");
  dire = getdecd((int) dire); 
      
  ret = IO_PCIMemMap(pciaddr, 0x1000, &virtaddr);
  if (ret)
  {
    rcc_error_print(stdout, ret);
    return(1);
  }

  cont = 1;
  printf("Test running. Press <ctrl+\\> to stop\n");
  while(cont)
  {
    if (width == 8 && dire == 0) {cptr = (unsigned char *)virtaddr; cdata = *cptr;}
    if (width == 8 && dire == 1) {cptr = (unsigned char *)virtaddr; *cptr = 0x12;}
    if (width == 16 && dire == 0) {sptr = (unsigned short *)virtaddr; sdata = *sptr;}
    if (width == 16 && dire == 1) {sptr = (unsigned short *)virtaddr; *sptr = 0x1234;}
    if (width == 32 && dire == 0) {iptr = (unsigned int *)virtaddr; idata = *iptr;}
    if (width == 32 && dire == 1) {iptr = (unsigned int *)virtaddr; *iptr = 0x12345678;}
  }
      
  ret = IO_PCIMemUnmap(virtaddr, 0x1000);    
  if (ret)
  {
    rcc_error_print(stdout, ret);
    return(1);
  }

  printf("=========================================================================\n\n");
  return(0);
}

/****************/
int slavemap(void)
/****************/
{
  static unsigned int vbase = 0, pbase = 0x80000000, size = 0x01000000;
  static int md = 1, pcisp = 0, wp = 1, rp = 1, am = 2;
  int p64, maxmap;
  unsigned int sctl, sbs, sbd, sto;

  printf("\n=========================================================================\n");
  if (utype == 1)
    maxmap = 4;
  else
    maxmap = 8;

  printf("Select map decoder <1..%d>                   ", maxmap);
  md = getdecd(md);
  if ((md < 1) || (md > maxmap)) md = 1;
  printf("Select VMEbus base address                  ");
  vbase = gethexd(vbase);
  printf("Select PCI base address                     ");
  pbase = gethexd(pbase);
  printf("Select PCI space <0=MEM, 1=I/O>             ");
  pcisp = getdecd(pcisp);
  printf("Select the window size (bytes)              ");
  size = gethexd(size);
  if (md == 1 || md == 5) size &= 0xfffff000; else size &= 0xffff0000;
  if (size == 0) size = 0x10000;
  if (pcisp != 1) pcisp = 0; 
  printf("Select Write posting <0/1>                  ");
  wp = getdecd(wp);
  if (wp != 1) wp = 0;
  printf("Select read prefetching <0/1>               ");
  rp = getdecd(rp);
  if (rp != 1) rp = 0;
  printf("Select address space <0=A16, 1=A24, 2=A32>  ");
  am = getdecd(am);
  if ((am < 0) || (am > 2)) am = 2;
  p64 = 0;

  sctl = 0x80500000 + (wp << 30) + (rp << 29) + (am << 16) + (p64 << 7) + pcisp;
  sbs = vbase;
  sbd = vbase + size;
  sto = pbase - vbase;

  if (md == 1)
  {
    uni->vsi0_bs  = (sbs);
    uni->vsi0_bd  = (sbd);
    uni->vsi0_to  = (sto);
    uni->vsi0_ctl = (sctl);
  }
  if (md == 2)
  {
    uni->vsi1_bs  = (sbs);  
    uni->vsi1_bd  = (sbd);
    uni->vsi1_to  = (sto);
    uni->vsi1_ctl = (sctl);
  }
  if (md == 3)
  {
    uni->vsi2_bs  = (sbs);  
    uni->vsi2_bd  = (sbd);
    uni->vsi2_to  = (sto);
    uni->vsi2_ctl = (sctl);
  }
  if (md == 4)
  {
    uni->vsi3_bs  = (sbs);  
    uni->vsi3_bd  = (sbd);
    uni->vsi3_to  = (sto);
    uni->vsi3_ctl = (sctl);
  }
  if (md == 5)
  {
    uni->vsi4_bs  = (sbs);
    uni->vsi4_bd  = (sbd);
    uni->vsi4_to  = (sto);
    uni->vsi4_ctl = (sctl);
  }
  if (md == 6)
  {
    uni->vsi5_bs  = (sbs);
    uni->vsi5_bd  = (sbd);
    uni->vsi5_to  = (sto);
    uni->vsi5_ctl = (sctl);
  }
  if (md == 7)
  {
    uni->vsi6_bs  = (sbs);
    uni->vsi6_bd  = (sbd);
    uni->vsi6_to  = (sto);
    uni->vsi6_ctl = (sctl);
  }
  if (md == 8)
  {
    uni->vsi7_bs  = (sbs);
    uni->vsi7_bd  = (sbd);
    uni->vsi7_to  = (sto);
    uni->vsi7_ctl = (sctl);
  }
  printf("=========================================================================\n\n");
  return(0);
}


/*****************/
int mastermap(void)
/*****************/
{
  static int md = 1, pcisp = 0, wp = 1, am = 2, mdw = 3, cty = 1;
  static unsigned int vbase = 0, pbase = 0x80000000, size = 0x01000000;
  int maxmap;
  unsigned int sctl, sbs, sbd, sto;

  printf("\n=========================================================================\n");

  if (utype == 2)
    maxmap = 8; 
  else
    maxmap = 4;
  printf("Select map decoder <1..%d>                             ", maxmap);
  md = getdecd(md);
  if ((md < 1) || (md > maxmap)) md = 1;
  printf("Select VMEbus base address                            ");
  vbase = gethexd(vbase);
  printf("Select PCI base address                               ");
  pbase = gethexd(pbase);
  printf("Select PCI space <0=MEM, 1=I/O>                       ");
  pcisp = getdecd(pcisp);
  printf("Select the window size (bytes)                        ");
  size = gethexd(size);
  if (md == 1 || md == 5) size &= 0xfffff000; else size &= 0xffff0000;
  if (size == 0) size = 0x10000;
  if (pcisp != 1) pcisp = 0;
  printf("Select Write posting <0/1>                            ");
  wp = getdecd(wp);
  if (wp != 1) wp = 0;
  printf("Select address space <0=A16, 1=A24, 2=A32>            ");
  am = getdecd(am);
  if ((am < 0) || (am > 2)) am = 2;
  printf("Select maximum data width <0=D8, 1=D16, 2=D32 3=D64>  ");
  mdw = getdecd(mdw);
  if ((mdw < 0) || (mdw > 3)) mdw = 2;
  printf("Select cycle type <0=single 1=single and DMA>         ");
  cty = getdecd(cty);
  if (cty != 1) cty = 0; 
  sctl=0x80000000 + (cty << 8) + (mdw << 22) + (wp << 30) + (am << 16) + pcisp;
  sbs = pbase;
  sbd = pbase + size;
  sto = vbase - pbase;

  if (md == 1)
  {
    uni->lsi0_bs  = sbs;
    uni->lsi0_bd  = sbd;
    uni->lsi0_to  = sto;
    uni->lsi0_ctl = sctl;
  }
  if (md == 2)
  {
    uni->lsi1_bs  = sbs;
    uni->lsi1_bd  = sbd;
    uni->lsi1_to  = sto;
    uni->lsi1_ctl = sctl;
  }
  if (md == 3)
  {
    uni->lsi2_bs  = sbs;
    uni->lsi2_bd  = sbd;
    uni->lsi2_to  = sto;
    uni->lsi2_ctl = sctl;
  }
  if (md == 4)
  {
    uni->lsi3_bs  = sbs;
    uni->lsi3_bd  = sbd;
    uni->lsi3_to  = sto;
    uni->lsi3_ctl = sctl;
  }
  if (md == 5)
  {
    uni->lsi4_bs  = sbs;  
    uni->lsi4_bd  = sbd;  
    uni->lsi4_to  = sto;  
    uni->lsi4_ctl = sctl;
  }
  if (md == 6)
  {
    uni->lsi5_bs  = sbs;  
    uni->lsi5_bd  = sbd;  
    uni->lsi5_to  = sto;  
    uni->lsi5_ctl = sctl;
  }
  if (md == 7)
  {
    uni->lsi6_bs  = sbs;  
    uni->lsi6_bd  = sbd;  
    uni->lsi6_to  = sto;  
    uni->lsi6_ctl = sctl;
  }
  if (md == 8)
  {
    uni->lsi7_bs  = sbs;  
    uni->lsi7_bd  = sbd;  
    uni->lsi7_to  = sto;  
    uni->lsi7_ctl = sctl;
  }
  printf("=========================================================================\n\n");
  return(0);
}


/***************/
int setswap(void)
/***************/
{
  static int mswap = 1, sswap = 0, fswap = 0;
  unsigned int ret;
  unsigned char data;

  printf("Master swapping (1=Yes 0=No) ");
  mswap = getdecd(mswap);

  printf("Slave swapping  (1=Yes 0=No) ");
  sswap = getdecd(sswap);

  printf("Fast swapping   (1=Yes 0=No) ");
  fswap = getdecd(fswap);

  data = (mswap << 3);
  data |= (sswap << 4);
  data |= (fswap << 5);

  ret = VME_CCTSetSwap(data);
    if (ret != VME_SUCCESS)
    {
      VME_ErrorPrint(ret);
      exit(-1);
    }
  return(0);
}


/******************/
int hwconf_315(void)
/******************/
{
  unsigned char d1, d2, d3, d4, d5, d6, d7, d8;
  unsigned int ret;

  ret = IO_IOPeekUChar(CSR0_315, &d1);
  if (ret)
  { rcc_error_print(stdout, ret);  return(-1); }

  ret = IO_IOPeekUChar(CSR1_315, &d2);
  if (ret)
  { rcc_error_print(stdout, ret);  return(-1); }
 
  ret = IO_IOPeekUChar(CSR2_315, &d3);
  if (ret)
  { rcc_error_print(stdout, ret);  return(-1); }

  ret = IO_IOPeekUChar(CSR3_315, &d4);
  if (ret)
  { rcc_error_print(stdout, ret);  return(-1); }

  ret = IO_IOPeekUChar(BERR_315, &d5);
  if (ret)
  { rcc_error_print(stdout, ret);  return(-1); }

  ret = IO_IOPeekUChar(GPIO_315, &d6);
  if (ret)
  { rcc_error_print(stdout, ret);  return(-1); }

  ret = IO_IOPeekUChar(SERI_315, &d7);
  if (ret)
  { rcc_error_print(stdout, ret);  return(-1); }

  ret = IO_IOPeekUChar(SLOT_315, &d8);
  if (ret)
  { rcc_error_print(stdout, ret);  return(-1); }

  printf("Decoding control and status register 0 (0x%02x):\n", d1);
  printf("Board revision:                  %d\n", d1 & 0x7);
  printf("VME byte swap master enable:     %s\n", (d1 & 0x8)?"enabled":"disabled");
  printf("VME byte swap slave enable:      %s\n", (d1 & 0x10)?"enabled":"disabled");
  printf("VME byte swap fast write enable: %s\n", (d1 & 0x20)?"enabled":"disabled");
  printf("BIOS default switch:             %s\n", (d1 & 0x40)?"factory defaults":"user defaults");
  printf("Console:                         %s\n", (d1 & 0x80)?"COM port":"VGA");

  printf("\nDecoding control and status register 1 (0x%02x):\n",d2);
  printf("Onboard PMC A:                   %s\n", (d2 & 0x1)?"present":"absent");
  printf("Onboard PMC B:                   %s\n", (d2 & 0x2)?"present":"absent");
  printf("PMC A on carrier:                %s\n", (d2 & 0x4)?"present":"absent");
  printf("PMC B on carrier:                %s\n", (d2 & 0x8)?"present":"absent");
  printf("VMEbus BERR interrupt:           %s\n", (d2 & 0x10)?"enabled":"disabled");
  printf("VMEbus BERR detected:            %s\n", (d2 & 0x20)?"yes":"no");
  printf("Universe LINT1 detected:         %s\n", (d2 & 0x40)?"yes":"no");
  printf("Front panel switch caused NIM:   %s\n", (d2 & 0x80)?"yes":"no");
 
  printf("\nDecoding control and status register 2 (0x%02x):\n",d3);
  printf("PCI bus speed (PMC sites):       %s\n", (d3 & 0x01)?"66 MHz":"33 MHz");
  printf("Watchdog reset:                  %s\n", (d3 & 0x04)?"enabled":"disabled");
  printf("Watcdog enable switch:           %s\n", (d3 & 0x08)?"enabled":"disabled");
  printf("COM pot selection:               %s\n", (d3 & 0x10)?"COM 3 (front panel)":"COM 0 (P2)");
  printf("VMEbus SYSRESET:                 %s\n", (d3 & 0x20)?"enabled":"disabled");
  printf("Mode switch:                     %s\n", (d3 & 0x80)?"BIOS":"VSA");

  printf("\nDecoding VMEbus slot ID register (0x%02x):\n",d8);
  d8 = ~d8;
  printf("The card is installed in slot:   %d\n", d8 & 0x1f);
  printf("The GA parity bit is:            %d\n", (d8 >> 5) & 0x1);
  return(0);
}


/**************/
int hwconf(void)
/**************/
{
  unsigned char d1, d2, d3, d4, d5;
  unsigned int ret;

  printf("\n=========================================================================\n");

  if (btype == 6 || btype == 7 || btype == 8 || btype == 9 || btype == 10)
  {
    hwconf_315();
    printf("=========================================================================\n\n");
    return(0);
  }


  if(btype == 3 || btype == 5) ret = IO_IOPeekUChar(CSR0_100, &d1);
  else ret = IO_IOPeekUChar(CSR0, &d1);
  if (ret)
  {
    rcc_error_print(stdout, ret);
    return(-1);
  }
  
  if(btype == 3 || btype == 5) ret = IO_IOPeekUChar(CSR1_100, &d2);
  else ret = IO_IOPeekUChar(CSR1, &d2);
  if (ret)
  {
    rcc_error_print(stdout, ret);
    return(-1);
  }
  
  if(btype == 3 || btype == 5) ret = IO_IOPeekUChar(CSR2_100, &d3);
  else ret = IO_IOPeekUChar(CSR2, &d3);
  if (ret)
  {
    rcc_error_print(stdout, ret);
    return(-1);
  }  
  
  if(btype == 3 || btype == 5) ret = IO_IOPeekUChar(CSR3_100, &d4);
  else ret = IO_IOPeekUChar(CSR3, &d4);
  if (ret)
  {
    rcc_error_print(stdout, ret);
    return(-1);
  }
  
  if (btype == 5) 
  {
    ret = IO_IOPeekUChar(CSR4_110, &d5);
    if (ret)
    {
      rcc_error_print(stdout, ret);
      return(-1);
    }
  }
  
  printf("Decoding control and status register 0 (0x%02x):\n", d1);
  printf("Board revision:                  %d\n", d1 & 0x7);
  printf("VME byte swap master enable:     %s\n", (d1 & 0x8)?"enabled":"disabled");
  printf("VME byte swap slave enable:      %s\n", (d1 & 0x10)?"enabled":"disabled");
  printf("VME byte swap fast write enable: %s\n", (d1 & 0x20)?"enabled":"disabled");
  if (btype == 2 || btype == 3 || btype == 5)
  {
    printf("User switch:                     %s\n", (d1 & 0x40)?"on":"off");  
    printf("Console:                         %s\n", (d1 & 0x80)?"VGA":"COM1");
  } 
  if (btype == 4)
    printf("Console:                         %s\n", (d1 & 0x80)?"VGA":"COM1");
     
  printf("\nDecoding control and status register 1 (0x%02x):\n",d2);
  if (btype == 1)
  {
    printf("PMC A on carrier:                %s\n", (d2 & 0x1)?"present":"absent");
    printf("PMC B on carrier:                %s\n", (d2 & 0x2)?"present":"absent");
    printf("Onboard PMC:                     %s\n", (d2 & 0x4)?"present":"absent");
    printf("Console:                         %s\n", (d2 & 0x20)?"VGA":"COM1");
    printf("NMI from Universe LINT1          %s\n", (d2 & 0x40)?"yes":"no");
  }
  if (btype == 2 || btype == 3 || btype == 5)
  {
    printf("Onboard PMC A:                   %s\n", (d2 & 0x1)?"present":"absent");
    if(btype == 2 || btype == 5)
      printf("Onboard PMC B:                   %s\n", (d2 & 0x2)?"present":"absent");
    printf("PMC A on carrier:                %s\n", (d2 & 0x4)?"present":"absent");
    printf("PMC B on carrier:                %s\n", (d2 & 0x8)?"present":"absent");
    printf("VMEbus BERR interrupt:           %s\n", (d2 & 0x10)?"enabled":"disabled");
    printf("VMEbus BERR detected:            %s\n", (d2 & 0x20)?"yes":"no");
    printf("Universe LINT1 detected:         %s\n", (d2 & 0x40)?"yes":"no");
    printf("Front panel switch caused NIM:   %s\n", (d2 & 0x80)?"yes":"no");
  }
  if (btype == 4)
  {
    printf("Onboard PMC:                     %s\n", (d2 & 0x4)?"present":"absent");
    printf("PMC A on carrier:                %s\n", (d2 & 0x1)?"present":"absent");
    printf("PMC B on carrier:                %s\n", (d2 & 0x2)?"present":"absent");
    printf("VMEbus BERR interrupt:           %s\n", (d2 & 0x10)?"enabled":"disabled");
    printf("VMEbus BERR detected:            %s\n", (d2 & 0x20)?"yes":"no");
    printf("Universe LINT1 detected:         %s\n", (d2 & 0x40)?"yes":"no");
    printf("Front panel switch caused NIM:   %s\n", (d2 & 0x80)?"yes":"no");
  }
  
  if (btype == 1)
  {
    printf("\nDecoding control and status register 2 (0x%02x):\n", d3&0xf0);
    printf("DiskOnChip                       %s\n", (d3 & 0x40)?"enabled":"disabled");
  }
  if (btype == 3 || btype == 5)
  {
    printf("\nDecoding control and status register 2 (0x%02x):\n", d3);
    printf("Overtemperature alert            %s\n", (d3 & 0x20)?"enabled":"disabled");
    printf("BIOS/VSA select                  %s\n", (d3 & 0x40)?"VSA (not recommended)":"BIOS");
    if (btype == 5)   
      printf("PCI speed / voltage              %s\n", (d3 & 0x80)?"66 MHz / 3.3V":"33 MHz / 5V");
  }
  
  printf("\nDecoding status register 3 (0x%02x):\n", d4);
  printf("GA slot number:                  %d\n", 0x1f - (d4 & 0x1f));
  printf("GA parity:                       %d\n", (d4 >> 5) & 0x1);
  if (btype == 2 || btype == 3 || btype == 5)
    printf("VMEbus SYSRST# is                %s\n", (d4 & 0x40)?"enabled":"disabled");
    
  if (btype == 5)
  {
    printf("\nDecoding status register 4 (0x%02x):\n", d5);
    printf("The on-board Battery is          %s\n", (d5 & 0x4)?"Weak":"OK");
  }
    
  printf("=========================================================================\n\n");
  return(0);
}


/***************/
int berrget(void)
/***************/
{
  unsigned char data, berr_info[16];
  unsigned int vme_ad, vme_am, vme_ds0, vme_ds1, vme_lword, vme_wr, ret, loop;

  printf("\n=========================================================================\n");
  
  if (btype == 1)
  {
    printf("The VP-PSE does not latch any information on the occasion of a VMEbus error\n");
    printf("=========================================================================\n\n");
    return(0);
  }
  if (btype == 2)
  {
    printf("The VP-PMC does not latch any information on the occasion of a VMEbus error\n");
    printf("=========================================================================\n\n");
    return(0);
  }
  if (btype == 4)
  {
    printf("The VP-CP1 does not latch any information on the occasion of a VMEbus error\n");
    printf("=========================================================================\n\n");
    return(0);
  }
  
  // Check if there was an error
  ret = IO_IOPeekUChar(CSR1_100, &data);
  if (ret)
  {
    rcc_error_print(stdout, ret);
    return(-1);
  }

  if (!(data&0x20))
  {
    printf("There has been no VMEbus BERR recently\n");
    printf("=========================================================================\n\n");
    return(0);
  }
  
  ret = IO_IOPeekUChar(BERR_100, &data);
  if (ret)
  {
    rcc_error_print(stdout, ret);
    return(-1);
  }
  
  if (data & 0x80)
  {
    printf("0x%02x read from VMEbus address capture data register\n", data);
    printf("The BERR capture logic is currently busy. Retry later.\n");
    printf("=========================================================================\n\n");
    return(1);
  }
  
  // Reset the read sequence
  ret = IO_IOPokeUChar(BERR_100, 0x2);
  if (ret)
  {
    rcc_error_print(stdout, ret);
    return(-1);
  }
  
  // Read the raw data
  for(loop = 0; loop < 16; loop++)
  {
    ret = IO_IOPeekUChar(BERR_100, &data);
    if (ret)
    {
      rcc_error_print(stdout, ret);
      return(-1);
    }
    berr_info[loop] = data&0xf;
  }
  
  // Assemble the values
  vme_ad = (berr_info[0] << 28) +
           (berr_info[1] << 24) +
           (berr_info[2] << 20) +
           (berr_info[3] << 16) +
           (berr_info[4] << 12) +
           (berr_info[5] << 8) +
           (berr_info[6] << 4) +
           (berr_info[7]&0xe);
  vme_am = ((berr_info[8]&0x3) << 4) + berr_info[9];
  vme_lword = berr_info[7]&0x1;
  vme_ds0 = (berr_info[8]&0x4) >> 2;
  vme_ds1 = (berr_info[8]&0x8) >> 3;
  vme_wr = (berr_info[10]&0x8) >> 3;

  printf("Here are the parameters of the VMEbus cycle that caused the BERR:\n");
  printf("Address          = 0x%08x\n", vme_ad);
  printf("Address Modifier = 0x%02x\n", vme_am);
  printf("Direction        = %s\n", vme_wr?"Write":"Read");
  printf("#LWORD           = %d\n", vme_lword);
  printf("#DS0             = %d\n", vme_ds0);
  printf("#DS1             = %d\n", vme_ds1);

  // Reset the capture buffer
  ret = IO_IOPokeUChar(BERR_100, 0x4);
  if (ret)
  {
    rcc_error_print(stdout, ret);
    return(-1);
  }

  //Reenable the capture
  ret = IO_IOPokeUChar(BERR_100, 0x1);
  if (ret)
  {
    rcc_error_print(stdout, ret);
    return(-1);
  }

  // Clear the VMEbus error
  ret = IO_IOPeekUChar(CSR1_100, &data);
  if (ret)
  {
    rcc_error_print(stdout, ret);
    return(-1);
  }
  
  data &= 0xdf;
  
  ret = IO_IOPokeUChar(CSR1_100, data);
  if (ret)
  {
    rcc_error_print(stdout, ret);
    return(-1);
  }

  printf("=========================================================================\n\n");
  return(0);
}
 

/****************/
int berrhand(void)
/****************/
{
  unsigned char d2, berrbit;
  unsigned int ret;

  printf("\n=========================================================================\n");
  
  if (btype == 1)
  {
    printf("The VP PSE does not support VMEbus BERR handling\n");
    printf("beyond what is provided by the Universe chip\n");
    printf("=========================================================================\n\n");
    return(0);
  }
  
  
  if(btype == 3 || btype == 5) ret = IO_IOPeekUChar(CSR1_100, &d2);
  else ret = IO_IOPeekUChar(CSR1, &d2);
  if (ret)
  {
    rcc_error_print(stdout, ret);
    return(-1);
  }
  
  berrbit = d2 & 0x10;
  if (berrbit)
  {
    d2 &= 0xcf;  //disable IRQ and clear BERR flag
    if(btype == 3 || btype == 5) ret = IO_IOPokeUChar(CSR1_100, d2);
    else ret = IO_IOPokeUChar(CSR1, d2);  
    if (ret)
    {
      rcc_error_print(stdout, ret);
      return(-1);
    }
    printf("VMEbus BERR interrupt is now disabled\n");
  }
  else
  {
    d2 |= 0x10;  //enable IRQ
    d2 &= 0xdf;  //clear BERR flag    
    if(btype == 3 || btype == 5) ret = IO_IOPokeUChar(CSR1_100, d2);
    else ret = IO_IOPokeUChar(CSR1, d2);  
    if (ret)
    {
      rcc_error_print(stdout, ret);
      return(-1);
    }
    printf("VMEbus BERR interrupt is now enabled\n");
  }
  printf("=========================================================================\n\n");
  return(0);
}


/****************/
int mainhelp(void)
/****************/
{
  printf("\n=========================================================================\n");
  printf("Abbreviations:\n");
  printf("\n");
  printf("RAH = Read ahead\n");
  printf("WP  = Write posting\n");
  printf("\n");
  printf("Call Markus Joos, 72364, if you need more help\n");
  printf("=========================================================================\n\n");
  return(0);
}


/***************/
int vmemenu(void)
/***************/
{
  static int fun;

  fun = 1;
  while(fun != 0)
  {
    printf("\n");
    printf("Select an option:\n");
    printf("   1 Help                        2 Decode master mapping\n");
    printf("   3 Decode slave mapping        4 Decode special slave image\n");
    printf("   5 Decode DMA registers        6 Decode misc. registers\n");
    printf("   7 Decode PCI conf. registers  8 Decode interrupt registers\n");
    printf("   0 Main menu\n");
    printf("Your choice: ");
    fun = getdecd(fun);
    if (fun == 1) vmehelp();
    if (fun == 2) decmaster();
    if (fun == 3) decslave();
    if (fun == 4) decslsi();
    if (fun == 5) decbma();
    if (fun == 6) decmisc();
    if (fun == 7) decpci();
    if (fun == 8) decirq();
  }
  return(0);
}


/**************/
int decirq(void)
/**************/
{
int d1, d2, d3, d4, d5;
 
  printf("\n============================================================================================\n");
  d1 = (uni->lint_en);
  d2 = (uni->lint_stat);
  d3 = (uni->lint_map0);
  d4 = (uni->lint_map1);
  d5 = (uni->lint_map2);
  printf("VMEbus to PCI:\n");
  printf("         ACFAIL SYSFAIL SW_INT SW_IACK VERR LERR DMA VME1 VME2 VME3 VME4 VME5 VME6 VME7 VOWN\n");
  printf("Masked: ");
  printf("    %s", d1 & 0x8000?" No":"Yes");
  printf("     %s", d1 & 0x4000?" No":"Yes");
  printf("    %s", d1 & 0x2000?" No":"Yes");
  printf("     %s",d1 & 0x1000?" No":"Yes");
  printf("  %s", d1 & 0x0400?" No":"Yes");
  printf("  %s", d1 & 0x0200?" No":"Yes");
  printf(" %s", d1 & 0x0100?" No":"Yes");
  printf("  %s", d1 & 0x0002?" No":"Yes");
  printf("  %s", d1 & 0x0004?" No":"Yes");
  printf("  %s", d1 & 0x0008?" No":"Yes");
  printf("  %s", d1 & 0x0010?" No":"Yes");
  printf("  %s", d1 & 0x0020?" No":"Yes");
  printf("  %s", d1 & 0x0040?" No":"Yes");
  printf("  %s", d1 & 0x0080?" No":"Yes");
  printf("  %s\n",d1& 0x0001?" No":"Yes");
  printf("Active: ");
  printf("    %s", d2 & 0x8000?"Yes":" No");
  printf("     %s", d2 & 0x4000?"Yes":" No");
  printf("    %s", d2 & 0x2000?"Yes":" No");
  printf("     %s", d2 & 0x1000?"Yes":" No");
  printf("  %s", d2 & 0x0400?"Yes":" No");
  printf("  %s", d2 & 0x0200?"Yes":" No");
  printf(" %s", d2 & 0x0100?"Yes":" No");
  printf("  %s", d2 & 0x0002?"Yes":" No");
  printf("  %s", d2 & 0x0004?"Yes":" No");
  printf("  %s", d2 & 0x0008?"Yes":" No");
  printf("  %s", d2 & 0x0010?"Yes":" No");
  printf("  %s", d2 & 0x0020?"Yes":" No");
  printf("  %s", d2 & 0x0040?"Yes":" No");
  printf("  %s", d2 & 0x0080?"Yes":" No");
  printf("  %s\n", d2 & 0x0001?"Yes":" No");
  printf("PCI lvl:");
  printf("      %d", (d4 >> 28) & 0x7);
  printf("       %d", (d4 >> 24) & 0x7);
  printf("      %d", (d4 >> 20) & 0x7);
  printf("       %d", (d4 >> 16) & 0x7);
  printf("    %d", (d4 >> 8) & 0x7);
  printf("    %d", (d4 >> 4) & 0x7);
  printf("   %d" , d4 & 0x7);
  printf("    %d", (d3 >> 4) & 0x7);
  printf("    %d", (d3 >> 8) & 0x7);
  printf("    %d", (d3 >> 12) & 0x7);
  printf("    %d", (d3 >> 16) & 0x7);
  printf("    %d", (d3 >> 20) & 0x7);
  printf("    %d", (d3 >> 24) & 0x7);
  printf("    %d", (d3 >> 28) & 0x7);
  printf("    %d\n", d3 & 0x7);

  if (utype == 2)
  {
    printf("          MBOX3   MBOX2  MBOX1   MBOX0  LM3  LM2 LM1  LM0 \n");
    printf("Masked: ");
    printf("    %s", d1 & 0x80000?" No":"Yes");
    printf("     %s", d1 & 0x40000?" No":"Yes");
    printf("    %s", d1 & 0x20000?" No":"Yes");
    printf("     %s", d1 & 0x10000?" No":"Yes");
    printf("  %s", d1 & 0x800000?" No":"Yes");
    printf("  %s", d1 & 0x400000?" No":"Yes");
    printf(" %s", d1 & 0x200000?" No":"Yes");
    printf("  %s\n", d1 & 0x100000?" No":"Yes");
    printf("Active: ");
    printf("    %s", d2 & 0x80000?"Yes":" No");
    printf("     %s", d2 & 0x40000?"Yes":" No");
    printf("    %s", d2 & 0x20000?"Yes":" No");
    printf("     %s", d2 & 0x10000?"Yes":" No");
    printf("  %s", d2 & 0x800000?"Yes":" No");
    printf("  %s", d2 & 0x400000?"Yes":" No");
    printf(" %s", d2 & 0x200000?"Yes":" No");
    printf("  %s\n", d2 & 0x100000?"Yes":" No");
    printf("PCI lvl:");
    printf("      %d", (d5 >> 12) & 0x7);
    printf("       %d", (d5 >> 8) & 0x7);
    printf("      %d", (d5 >> 4) & 0x7);
    printf("       %d", d5 & 0x7);
    printf("    %d", (d5 >> 28) & 0x7);
    printf("    %d", (d5 >> 24) & 0x7);
    printf("   %d", (d5 >> 20) & 0x7);
    printf("    %d\n", (d5 >> 16) & 0x7);
  }

  printf("\n\nPCI to VMEbus:\n");
  d1 = (uni->vint_en);  
  d2 = (uni->vint_stat);
  d3 = (uni->vint_map0);
  d4 = (uni->vint_map1);
  d5 = (uni->vint_map2);
  printf("         SW_IACK VERR LERR  DMA LINT0 LINT1 LINT2 LINT3 LINT4 LINT5 LINT6 LINT7\n");
  printf("Masked: ");
  printf("     %s", d1 & 0x1000?" No":"Yes");
  printf("  %s", d1 & 0x0400?" No":"Yes");
  printf("  %s", d1 & 0x0200?" No":"Yes");
  printf("  %s", d1 & 0x0100?" No":"Yes");
  printf("   %s", d1 & 0x0001?" No":"Yes");
  printf("   %s", d1 & 0x0002?" No":"Yes");
  printf("   %s", d1 & 0x0004?" No":"Yes");
  printf("   %s", d1 & 0x0008?" No":"Yes");
  printf("   %s", d1 & 0x0010?" No":"Yes");
  printf("   %s", d1 & 0x0020?" No":"Yes");
  printf("   %s", d1 & 0x0040?" No":"Yes");
  printf("   %s\n",d1& 0x0080?" No":"Yes");
  printf("Active: ");
  printf("     %s", d2 & 0x1000?"Yes":" No");
  printf("  %s", d2 & 0x0400?"Yes":" No");
  printf("  %s", d2 & 0x0200?"Yes":" No");
  printf("  %s", d2 & 0x0100?"Yes":" No");
  printf("   %s", d2 & 0x0001?"Yes":" No");
  printf("   %s", d2 & 0x0002?"Yes":" No");
  printf("   %s", d2 & 0x0004?"Yes":" No");
  printf("   %s", d2 & 0x0008?"Yes":" No");
  printf("   %s", d2 & 0x0010?"Yes":" No");
  printf("   %s", d2 & 0x0020?"Yes":" No");
  printf("   %s", d2 & 0x0040?"Yes":" No");
  printf("   %s\n", d2 & 0x0080?"Yes":" No");
  printf("VME lvl:");
  printf("       %d", (d4 >> 16) & 0x7);
  printf("    %d", (d4 >> 8) & 0x7);
  printf("    %d", (d4 >> 4) & 0x7);
  printf("    %d", d4 & 0x7);  
  printf("     %d", d3 & 0x7);  
  printf("     %d", (d3 >> 4) & 0x7);  
  printf("     %d", (d3 >> 8) & 0x7);  
  printf("     %d", (d3 >> 12) & 0x7);  
  printf("     %d", (d3 >> 16) & 0x7);  
  printf("     %d", (d3 >> 20) & 0x7);  
  printf("     %d", (d3 >> 24) & 0x7);  
  printf("     %d\n", (d3 >> 28) & 0x7);

  if(utype==2)
  {
    printf("            VME7 VME6 VME5 VME4  VME3  VME2  VME1 MBOX3 MBOX2 MBOX1 MBOX0\n");
    printf("Masked: ");
    printf("     %s", d1 & 0x80000000?" No":"Yes");
    printf("  %s", d1 & 0x40000000?" No":"Yes");
    printf("  %s", d1 & 0x20000000?" No":"Yes");
    printf("  %s", d1 & 0x10000000?" No":"Yes");
    printf("   %s", d1 & 0x8000000?" No":"Yes");
    printf("   %s", d1 & 0x4000000?" No":"Yes");
    printf("   %s", d1 & 0x2000000?" No":"Yes");
    printf("   %s", d1 & 0x80000?" No":"Yes");
    printf("   %s", d1 & 0x40000?" No":"Yes");
    printf("   %s", d1 & 0x20000?" No":"Yes");
    printf("   %s\n", d1 & 0x10000?" No":"Yes");
    printf("Active: ");
    printf("     %s", d2 & 0x80000000?"Yes":" No");
    printf("  %s", d2 & 0x40000000?"Yes":" No");
    printf("  %s", d2 & 0x20000000?"Yes":" No");
    printf("  %s", d2 & 0x10000000?"Yes":" No");
    printf("   %s", d2 & 0x8000000?"Yes":" No");
    printf("   %s", d2 & 0x4000000?"Yes":" No");
    printf("   %s", d2 & 0x2000000?"Yes":" No");
    printf("   %s", d2 & 0x80000?"Yes":" No");
    printf("   %s", d2 & 0x40000?"Yes":" No");
    printf("   %s", d2 & 0x20000?"Yes":" No");
    printf("   %s\n", d2 & 0x10000?"Yes":" No");
    printf("VME lvl:");
    printf("       7");
    printf("    6");
    printf("    5");
    printf("    4");  
    printf("     3");  
    printf("     2");  
    printf("     1");  
    printf("     %d", (d5 >> 12) & 0x7);  
    printf("     %d", (d5 >> 8) & 0x7);  
    printf("     %d", (d5 >> 4) & 0x7);  
    printf("     %d\n", d5 & 0x7);  
  }
  printf("============================================================================================\n\n");
  return(0);
}


/***************/
int decmisc(void)
/***************/
{
  unsigned int data, valu, slt1;

  printf("\n=========================================================================\n");
  printf("Miscellaneous Control Register:\n");
  data = (uni->misc_ctl);
  slt1 = (data >> 17) & 0x1;
  printf("Slot 1 functions:          %s\n", slt1?"Enabled":"Disabled");
  if (slt1)
  {
    printf("VMEbus time-out:           ");
    valu = data >> 28;
    if (valu == 0) printf("Disabled\n");
    if (valu == 1) printf("16 us\n");
    if (valu == 2) printf("32 us\n");   
    if (valu == 3) printf("64 us\n");   
    if (valu == 4) printf("128 us\n");      
    if (valu == 5) printf("256 us\n");     
    if (valu == 6) printf("512 us\n");     
    if (valu == 7) printf("1024 us\n");     
    if (valu > 7) printf("Reserved\n");
    printf("Arbitration time-out:      ");
    valu = (data >> 24) & 0x3; 
    if (valu == 0) printf("Disabled\n");
    if (valu == 1) printf("16 us\n");   
    if (valu == 2) printf("256 us\n");
    if (valu == 3) printf("Reserved\n");
    printf("Arbitration mode:          %s\n", data&0x04000000?"PRI":"RRS");
  }

  printf("\nMaster Control Register:\n");
  data = (uni->mast_ctl);
  printf("Max number of PCI retries: ");
  valu = (data >> 28);
  if (valu == 0) printf("Retry forever\n");
  else printf("%d\n", valu*64);
  printf("WP transfer count:         ");
  valu = (data >> 24) & 0xf;
  if (valu == 0) printf("128 bytes\n");
  else if (valu == 1) printf("256 bytes\n");
  else if (valu == 2) printf("512 bytes\n");
  else if (valu == 3) printf("1024 bytes\n");
  else if (valu == 4) printf("2048 bytes\n");
  else if (valu == 5) printf("4096 bytes\n");
  else if (utype == 1) printf("Reserved\n");
  else if (utype == 2)
  {
    if (valu == 15) printf("BBSY* release after each transaction\n");
    else printf("Reserved\n");
  }

  printf("VMEbus request level:      %d\n", (data >> 22) & 0x3);
  printf("VMEbus request mode:       %s\n", data & 0x200000?"Fair":"Demand");
  printf("VMEbus release mode:       %s\n", data & 0x100000?"ROR":"RWD");
  printf("VMEbus ownership:          %s\n", data & 0x80000?"Acquire and hold bus":"Release bus");
  printf("VMEbus ownership status:   %s\n", data & 0x40000?"VMEbus acquired and held":"VMEbus not owned");

  printf("\nMiscellaneous Status Register:\n");
  data = (uni->misc_stat);
  printf("PCI bus width:             %s bits\n", data & 0x40000000?"64":"32");
  printf("UNIVERSE drives BUSY*:     %s\n", data & 0x200000?"No":"Yes");
  printf("Transmit FIFO:             %s\n", data & 0x40000?"Empty":"Not empty");
  printf("Receive FIFO:              %s\n", data & 0x20000?"Empty":"Not empty");

  printf("\nUser AM Codes Register:\n");
  data = (uni->user_am);
  printf("User AM code 1:            0x%02x\n", (data >> 26) & 0x3f);
  printf("User AM code 2:            0x%02x\n", (data >> 18) & 0x3f);
  printf("=========================================================================\n\n");
  return(0);
}


/**************/
int decpci(void)
/**************/
{
  int data;

  printf("\n=========================================================================\n");
  printf("PCI_CLASS:\n");
  data = (uni->pci_class) & 0xff;   	//mask Revision ID
  printf("Chip type:         %s\n", data?"Universe II":"Universe I");


  data = (uni->pci_csr);
  printf("\nPCI_CSR: (0x%08x)\n", data);
  printf("Master:                   %s\n", data & 0x4?"Enabled":"Disabled");
  printf("PCI MEM space:            %s\n", data & 0x2?"Enabled":"Disabled");
  printf("PCI I/O space:            %s\n", data & 0x1?"Enabled":"Disabled");
  printf("Detected parity error:    %s\n", data & 0x80000000?"Yes":"No");
  printf("Signalled SERR:           %s\n", data & 0x40000000?"Yes":"No");
  printf("Received master abort:    %s\n", data & 0x20000000?"Yes":"No");
  printf("Received target abort:    %s\n", data & 0x10000000?"Yes":"No");
  printf("Signalled target abort:   %s\n", data & 0x08000000?"Yes":"No");
  printf("Master data parity error: %s\n", data & 0x01000000?"Yes":"No");
  

  printf("\nPCI_MISC0:\n");                       
  data = (uni->pci_misc0);
  printf("PCI latency timer: 0x%02x\n", (data >> 8) & 0xff);

  printf("\nPCI_BS:\n");
  data = (uni->pci_bs);
  printf("PCI Base address:         0x%08x\n", data & 0xfffffffe);
  printf("PCI address space:        %s\n", data & 0x1?"PCI I/O":"PCI MEM");
  if (utype == 2)
  {
    data=(uni->pci_bs1);
    printf("Second PCI Base address:  0x%08x\n", data & 0xfffffffe);
    printf("Second PCI address space: %s\n", data & 0x1?"PCI I/O":"PCI MEM");
  }
  printf("=========================================================================\n\n");
  return(0);
}


/****************/
int decslave(void)
/****************/
{
  printf("\n===========================================================================================\n");

  printf("VSI  VME address range  PCI address range   EN   WP   RP     VAS  AM  PCI64  RMW  PCI space\n");
  decsinf(0, (uni->vsi0_ctl), (uni->vsi0_bs), (uni->vsi0_bd), (uni->vsi0_to));
  decsinf(1, (uni->vsi1_ctl), (uni->vsi1_bs), (uni->vsi1_bd), (uni->vsi1_to));
  decsinf(2, (uni->vsi2_ctl), (uni->vsi2_bs), (uni->vsi2_bd), (uni->vsi2_to));
  decsinf(3, (uni->vsi3_ctl), (uni->vsi3_bs), (uni->vsi3_bd), (uni->vsi3_to));
  if (utype == 2)
  {
    decsinf(4, (uni->vsi4_ctl), (uni->vsi4_bs), (uni->vsi4_bd), (uni->vsi4_to));
    decsinf(5, (uni->vsi5_ctl), (uni->vsi5_bs), (uni->vsi5_bd), (uni->vsi5_to));
    decsinf(6, (uni->vsi6_ctl), (uni->vsi6_bs), (uni->vsi6_bd), (uni->vsi6_to));
    decsinf(7, (uni->vsi7_ctl), (uni->vsi7_bs), (uni->vsi7_bd), (uni->vsi7_to));
  }
  printf("===========================================================================================\n");
  return(0);
}


/***************/
int decslsi(void)
/***************/
{
  int valu,data;

  printf("\n===========================================================================================\n");
  data = (uni->slsi);
  printf("SLSI:                    0x%08x\n", data);
  printf("Image enabled:           %s\n", data & 0x80000000?"Yes":"No");
  printf("PCI base address:        0x%08x\n", (data << 24) & 0xfc000000);
  printf("PCI bus address space:   ");
  valu = data & 0x3;
  if (valu == 0) printf("PIC MEM\n");
  if (valu == 1) printf("PIC I/O\n");
  if (valu == 2) printf("Config.\n");
  if (valu == 3) printf("Reserved\n");
  printf("Write posting enable:    %s\n", data & 0x40000000?"Yes":"No");
  printf("Region    Cycle type   Max data width\n");

  printf("     1     %s,%s", data & 0x100?"Supervisor":"User",data&0x1000?"Program":"Data");
  printf("              %s\n", data & 0x100000?"D32":"D16");
  printf("     2     %s,%s", data & 0x200?"Supervisor":"User",data&0x2000?"Program":"Data");
  printf("              %s\n", data & 0x200000?"D32":"D16");
  printf("     3     %s,%s", data & 0x400?"Supervisor":"User",data&0x4000?"Program":"Data");
  printf("              %s\n", data & 0x400000?"D32":"D16");
  printf("     4     %s,%s", data & 0x800?"Supervisor":"User",data&0x8000?"Program":"Data");
  printf("              %s\n", data & 0x800000?"D32":"D16");
  printf("===========================================================================================\n");
  return(0);
}


/**************/
int decbma(void)
/**************/
{
  int data, valu;

  printf("\n=======================================================================================\n");
  printf("PCI address               (DLA)= 0x%08x\n", uni->dla); 
  printf("VME address               (DVA)= 0x%08x\n", uni->dva); 
  printf("Byte count               (DTBC)= 0x%08x\n", uni->dtbc);
  printf("Chain pointer            (DCPP)= 0x%08x\n", uni->dcpp);
  data = uni->dctl;
  printf("Transfer control reg.    (DCTL)= 0x%08x\n", data);
  printf("64 bit PCI is                    %s\n", data & 0x80?"Enabled":"Disabled");
  printf("VMEbus cycle type:               %s\n", data & 0x100?"Block cycles":"Single cycles");
  printf("AM code tpye:                    %s\n", data & 0x1000?"Supervisor":"User");
  printf("AM code tpye:                    %s\n", data & 0x4000?"Programm":"Data");
  printf("VMEbus address space:            ");
  valu = (data >> 16) & 0x7;
  if (valu == 0) printf("A16\n");
  if (valu == 1) printf("A24\n");
  if (valu == 2) printf("A32\n");
  if (valu == 3) printf("Reserved\n");     
  if (valu == 4) printf("Reserved\n");     
  if (valu == 5) printf("Reserved\n");     
  if (valu == 6) printf("User 1\n");       
  if (valu == 7) printf("User 2\n");       
  printf("VMEbus max data width:           ");
  valu = (data >> 22) & 0x3;
  if (valu == 0) printf("D8\n");           
  if (valu == 1) printf("D16\n");
  if (valu == 2) printf("D32\n");
  if (valu == 3) printf("D64\n");
  printf("Transfer direction:              VMEbus %s\n", data & 0x80000000?"Write":"Read");
  data = (uni->dgcs);
  printf("General control reg.     (DGCS)= 0x%08x\n", data);
  printf("Interrupt on protocol error:     %s\n", data & 0x1?"Enabled":"Disabled");
  printf("Interrupt on VME error:          %s\n", data & 0x2?"Enabled":"Disabled");
  printf("Interrupt on PCI error:          %s\n", data & 0x4?"Enabled":"Disabled");
  printf("Interrupt when done:             %s\n", data & 0x8?"Enabled":"Disabled");
  printf("Interrupt when halted:           %s\n", data & 0x20?"Enabled":"Disabled");
  printf("Interrupt when stopped:          %s\n", data & 0x40?"Enabled":"Disabled");
  printf("Protocol error:                  %s\n", data & 0x100?"Yes":"No");
  printf("VMEbus error:                    %s\n", data & 0x200?"Yes":"No");
  printf("PCI Bus error:                   %s\n", data & 0x400?"Yes":"No");
  printf("Transfer complete:               %s\n", data & 0x800?"Yes":"No");
  printf("DMA halted:                      %s\n", data & 0x2000?"Yes":"No");
  printf("DMA stopped:                     %s\n", data & 0x4000?"Yes":"No");
  printf("DMA active:                      %s\n", data & 0x8000?"Yes":"No");
  valu = (data >> 16) & 0xf;                     
  printf("Minimum inter tenure gap:        ");
  if (valu == 0) printf("0 us\n");
  else if (valu == 1) printf("16 us\n");   
  else if (valu == 2) printf("32 us\n");   
  else if (valu == 3) printf("64 us\n");
  else if (valu == 4) printf("128 us\n");
  else if (valu == 5) printf("256 us\n");  
  else if (valu == 6) printf("512 us\n");  
  else if (valu == 7) printf("1024 us\n"); 
  else printf("Out of range\n");
  printf("Bytes transfered per bus tenure: ");
  valu = (data >> 20) & 0xf;
  if (valu == 0) printf("Until done\n");
  else if (valu == 1) printf("256 bytes\n");
  else if (valu == 2) printf("512 bytes\n");
  else if (valu == 3) printf("1 Kbye\n");   
  else if (valu == 4) printf("2 Kbye\n");   
  else if (valu == 5) printf("4 Kbye\n");   
  else if (valu == 6) printf("8 Kbye\n");   
  else if (valu == 7) printf("16 Kbye\n");  
  else printf("Out of range\n"); 
  printf("DMA chaining:                    %s\n", data & 0x8000000?"Yes":"No");
  printf("DMA halt request:                %s\n", data & 0x20000000?"Halt DMA after current package":"No");
  printf("DMA stop request:                %s\n", data & 0x40000000?"process buffered data and stop":"No");
  printf("DMA go:                          %s\n", data & 0x80000000?"Yes":"No");
  printf("===========================================================================================\n");
  return(0);
}


/*****************/
int decmaster(void)
/*****************/
{
  printf("\n=======================================================================================\n");

  printf("LSI  VME address range  PCI address range   EN   WP  VDW  VAS     AM    Type  PCI space\n");
  decminf(0, (uni->lsi0_ctl), (uni->lsi0_bs), (uni->lsi0_bd), (uni->lsi0_to));
  decminf(1, (uni->lsi1_ctl), (uni->lsi1_bs), (uni->lsi1_bd), (uni->lsi1_to));
  decminf(2, (uni->lsi2_ctl), (uni->lsi2_bs), (uni->lsi2_bd), (uni->lsi2_to));
  decminf(3, (uni->lsi3_ctl), (uni->lsi3_bs), (uni->lsi3_bd), (uni->lsi3_to));
  if (utype == 2)
  {
    decminf(4, (uni->lsi4_ctl), (uni->lsi4_bs), (uni->lsi4_bd), (uni->lsi4_to));
    decminf(5, (uni->lsi5_ctl), (uni->lsi5_bs), (uni->lsi5_bd), (uni->lsi5_to));
    decminf(6, (uni->lsi6_ctl), (uni->lsi6_bs), (uni->lsi6_bd), (uni->lsi6_to));
    decminf(7, (uni->lsi7_ctl), (uni->lsi7_bs), (uni->lsi7_bd), (uni->lsi7_to));
  }
  printf("=======================================================================================\n");
  return(0);
}


/*************************************************/
int decminf(int no, int d1, int d2, int d3, int d4)
/*************************************************/
{
  int d5;

  if (no == 0 || no == 4)
  {
    if (d3 == 0) d3 = 0xffffffff;
    d2 = d2 & 0xfffff000;
    d4 = d4 & 0xfffff000;
  }
  else
  {
    d2 = d2 & 0xffff0000;
    d4 = d4 & 0xffff0000;
  }

  printf("  %d", no);
  printf("  %08x-%08x", d2 + d4, d3 + d4);
  printf("  %08x-%08x", d2, d3);
  printf("  %s", d1 & 0x80000000?"Yes":" No");
  printf("  %s", d1 & 0x40000000?"Yes":" No");
  d5 = (d1 >> 22) & 0x3;
  if (d5 == 0) printf("  D08");
  if (d5 == 1) printf("  D16");
  if (d5 == 2) printf("  D32");
  if (d5 == 3) printf("  D64");
  d5 = (d1 >> 16) & 0x7;
  if (d5 == 0) printf("  A16   ");
  if (d5 == 1) printf("  A24   ");
  if (d5 == 2) printf("  A32   ");
  if (d5 == 3) printf("  Reser.");
  if (d5 == 4) printf("  Reser.");
  if (d5 == 5) printf("  CR/CSR");
  if (d5 == 6) printf("  User1 ");
  if (d5 == 7) printf("  User2 ");
  d5 = (d1 >> 12) & 0x3;
  if (d5 == 0) printf("  U");
  if (d5 == 1) printf("  S");
  if (d5 == 2) printf("  ?");
  if (d5 == 3) printf("  ?");
  d5 = (d1 >> 14) & 0x3;
  if (d5 == 0) printf("D");
  if (d5 == 1) printf("P");
  if (d5 == 2) printf("?");
  if (d5 == 3) printf("?");
  printf("%s ", d1 & 0x100?"  SC+BLT":"      SC");
  if (utype == 1)
  {
    d5 = d1 & 0x3;
    if (d5 == 0) printf("   PCI MEM\n");
    if (d5 == 1) printf("   PCI I/O\n");
    if (d5 == 2) printf("   Config.\n");
    if (d5 == 3) printf("   Reserv.\n");
  }
  else
  {
    d5 = d1 & 0x1;
    if (d5 == 0) printf("   PCI MEM\n");
    if (d5 == 1) printf("   PCI I/O\n");
  }
  return(0);
}


/*************************************************/
int decsinf(int no, int d1, int d2, int d3, int d4)
/*************************************************/
{
  int d5;

  if(no == 0  || no == 4)
  { 
    d2 = d2 & 0xfffff000;
    d4 = d4 & 0xfffff000;
  }
  else
  {
    d2 = d2 & 0xffff0000;
    d4 = d4 & 0xffff0000;
  }

  printf("  %d", no); 
  printf("  %08x-%08x", d2, d3);
  printf("  %08x-%08x", d2 + d4, d3 + d4);
  printf("  %s", d1 & 0x80000000?"Yes":" No");
  printf("  %s", d1 & 0x40000000?"Yes":" No");
  printf("  %s", d1 & 0x20000000?"Yes":" No");
  d5 = (d1 >> 16) & 0x7;
  if (d5 == 0) printf("     A16");
  if (d5 == 1) printf("     A24");
  if (d5 == 2) printf("     A32");
  if (d5 == 3) printf("  Reser.");
  if (d5 == 4) printf("  Reser.");
  if (d5 == 5) printf("  Reser.");
  if (d5 == 6) printf("   User1");
  if (d5 == 7) printf("   User2");
  d5 = (d1 >> 20) & 0x3;
  if (d5 == 0) printf("  ?");
  if (d5 == 1) printf("  U");
  if (d5 == 2) printf("  S");
  if (d5 == 3) printf("  B");
  d5 = (d1 >> 22) & 0x3;
  if (d5 == 0) printf("?");  
  if (d5 == 1) printf("D");  
  if (d5 == 2) printf("P");
  if (d5 == 3) printf("B");
  printf("    %s", d1 & 0x80?"Yes":" No");
  printf("  %s", d1 & 0x40?"Yes":" No");
  d5 = d1 & 0x3; 
  if (d5 == 0) printf("    PCI MEM\n");
  if (d5 == 1) printf("    PCI I/O\n");  
  if (d5 == 2) printf("    Config.\n");
  if (d5 == 3) printf("    Reserv.\n");
  return(0);
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
  printf("VSI    = Number of VMEbus Slave Image rgister set\n");
  printf("LSI    = Number of Local (PCI) Slave Image rgister set\n");
  printf("EN     = Page enabled\n");
  printf("WP     = Write posting\n");
  printf("VDW    = Maximum data width on VMEbus\n");
  printf("VAS    = VMEbus address space\n");
  printf("AM     = VMbus address modifier attributes:\n");
  printf("       First letter:\n");
  printf("         U = User access\n");
  printf("         S = Supervisor access\n");
  printf("         B = Both User and Supervisor access\n");
  printf("         ? = reserved\n");
  printf("       Second letter:\n");
  printf("         D = Data\n");
  printf("         P = Program\n");
  printf("         B = Both Data and Program\n");
  printf("         ? = reserved\n");
  printf("Type   = VMEbus cycle type\n");
  printf("PCI64  = Enable 64bit transactions on PCI\n");
  printf("RMW    = Enable PIC Lock of VMEbus read-modify-write\n");
  printf("Reser. = reserved\n");
  printf("SC     = single cycles\n");
  printf("SC+BLT = single cycles and block transfers\n");
  printf("Call Markus Joos, 72364, if you need more help\n");
  printf("=========================================================================\n\n");
  return(0);
}


