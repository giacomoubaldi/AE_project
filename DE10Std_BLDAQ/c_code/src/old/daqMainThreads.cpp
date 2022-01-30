#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "FpgaInterface.h"
#include "circular.h"
#include "EventCircularBuffer.h"

// Maximum event size allowed (in words)
const uint32_t MaxEventSize  = 50000;

// structure to hold data shared between producer, consumer and main
struct SharedData {
  FpgaInterface *fpga;
  bool isRunning;
  EventCircularBuffer* cb;	
  uint32_t wordsread;
  uint32_t eventsOut;
  uint32_t eventsOK;
  uint32_t outfifoStatus;
  pthread_mutex_t lock;
  pthread_cond_t cond;
  uint32_t verbosity;
};

//-----------------------------------------------------------------
// Thread for reading data from FIFO
// It fills the circular buffer
// Called during run only!
//
void *producer(void *arg){
  SharedData *sd = (SharedData*) arg;
  std::vector<uint32_t> evt;
  sd->wordsread = 0;
  sd->outfifoStatus=sd->fpga->getOFifoStatus();
  
  while(sd->isRunning){
    sd->outfifoStatus=sd->fpga->getOFifoStatus();
	
    if( sd->fpga->FifoEmpty(sd->outfifoStatus) || (sd->outfifoStatus&0xffff)==0 ){
      usleep(20*1000);  // 0.02 s sleep    A better idea?
    } else  {
      // here an event is waiting for us!!
      if( sd->verbosity>0 ) 
	printf("PRODUCER: an event is waiting\n");
      sd->fpga->readOfifo(evt); // read a chunk of data (max 2000?) from FIFO
      sd->wordsread += evt.size();

      // thread safe insertion in the buffer
      pthread_mutex_lock(&sd->lock);
      if( sd->verbosity>0 ){
	printf("PRODUCER: data in circular buffer before inserting event: %d words\n", sd->cb->size());
	printf("PRODUCER: data to store: %d words\n", evt.size());
      }
      // check there is enough space in the buffer
      while (sd->cb->capacity()<sd->cb->size()+evt.size()) { //this needs to be checked!!
	pthread_cond_wait(&sd->cond, &sd->lock);
      }

      // actual writing of data in the circular buffer
      sd->cb->write(evt);
      if( sd->verbosity>0 ){ 
	printf("PRODUCER: data in circular buffer now: %d words\n", 
	       sd->cb->size());
      }
	  
      pthread_cond_broadcast(&sd->cond); // wake up consumer
      pthread_mutex_unlock(&sd->lock);

      evt.erase(evt.begin(),evt.end());
    }
  }  
  //  here when out of a run: 
  printf("PRODUCER: end of run\n");
  usleep(20*1000);  // 0.02 s sleep

  //  clean fifo, evts and check!
  sd->fpga->readOfifo(evt);
  evt.erase(evt.begin(),evt.end());
  if( !sd->fpga->OFifoEmpty() ){
    printf("\n\nWARNING: Output fifo not empty!\n\n");
  }
  return NULL;  // no meaning in the return data
}


//-----------------------------------------------------------------
// Thread for reading the circular buffer and shipping it somewhere!
// now it prints the data
// called during run only!
//
void *consumer(void *arg){
  SharedData *sd = (SharedData*) arg;
  std::vector<uint32_t> evt;

  sd->eventsOut =0;          // events read out
  sd->eventsOK  =0;          // events with good tail
  bool gotHeader = false;    // in between an event reading?
  uint32_t remains=0;        // remaining data of a given event

  while(sd->isRunning){
    
    if( sd->cb->size()>6 || (gotHeader && sd->cb->size()>0) ){ // minimal size of an event
      if( sd->verbosity>0 ) printf("CONSUMER: an event is waiting\n");
      // try to pop one full event
      pthread_mutex_lock(&sd->lock);
      while (sd->cb->empty() && sd->isRunning) { //this needs to be checked!!
	pthread_cond_wait(&sd->cond, &sd->lock);
      }
      if(!sd->isRunning){  
        // here when the run has been ended during a reading
	pthread_cond_broadcast(&sd->cond); // wake up producer
	pthread_mutex_unlock(&sd->lock);	  
	break; // exit while loop!  
      }
      if(!gotHeader){  // check if you've not yet started to read this event 
        // Start reading from the beginning:
	// the first two words contains the event length and the header (in this order!)
	uint32_t val1,val2;
	val2 = sd->cb->pop_front();
	do{ 
	  val1 =val2;
	  val2 = sd->cb->pop_front();
	} while( val2!=Header_event && !sd->cb->empty());  // continue popping till header or buffer empty
	if( val2==Header_event){ // got header!
	  evt.push_back(val1);    // contains the event length
	  evt.push_back(val2);    // contains the header
          if( val1<MaxEventSize){ // put here a reasonable number please!
	    remains=val1-2;
	    gotHeader=true;
	    if( sd->verbosity>0 ) 
	      printf("CONSUMER: Reading an event : %d words\n", val1);

	  } // else consider the event as unrealiable and skip it
	}  // else cb is empty and no action    
      }  // else joined with the next if
	  
      if( gotHeader ){ // here can come multiple times for an event
	uint32_t toread=(remains < sd->cb->size() ? remains : sd->cb->size());
	sd->cb->read(evt,toread);
	remains -= toread;
      }
	  
      if(gotHeader && remains==0 ){

        sd->eventsOut++;
	if( evt.back()== Footer_event) {	  
	  sd->eventsOK++;
	} else {
	  printf("CONSUMER: corrupted event here! evt nr: %d\n",sd->eventsOut); 
	}
	// here an event has been read. Do what you want!!
	if( sd->verbosity>0 ){ 
	  printf("CONSUMER: Event %d has been read: %d words\n", 
		 sd->eventsOut, evt.size());
	  // printing the data of a single event
	  for(unsigned int i=0; i<evt.size(); i++){
	    printf("%08x ", evt[i]);			
	  }
	  printf("\n");
	  printf("CONSUMER: data in circular buffer: %d words\n", sd->cb->size());
	}
        // 
	// ship out of a single event        
	//

	// be ready to restart an event
        evt.erase(evt.begin(),evt.end());
        gotHeader=false;
      }

      pthread_cond_broadcast(&sd->cond); // wake up producer
      pthread_mutex_unlock(&sd->lock);	  
    }
  }
  printf("CONSUMER: end of run\n");
  // here at the end of a run. Pop everything
  usleep(20*1000);  // 0.02 s sleep    
  if( !sd->cb->empty() ){
    unsigned int size = sd->cb->size();
    pthread_mutex_lock(&sd->lock);
    printf("CONSUMER: data in circular buffer: %d words\n", sd->cb->size());
    sd->cb->read(evt,size);
    pthread_mutex_unlock(&sd->lock);	
  }  
  usleep(20*1000);  // 0.02 s sleep 
  if( !sd->cb->empty() ){
    printf("\n\nWARNING: Event buffer not empty!\n\n");
  }
  return NULL;
}


int main(int argc, char* argv[]) {

  FpgaInterface fpga;
  int rc, idx,nmax;
  uint32_t verbosity = 0;
  nmax=20000000;
  
  if( argc>1 ){
    fpga.setVerbosity(atoi(argv[1]));
    if( argc>2 ){
      verbosity= atoi(argv[2]);
    }
  }
  
  // printf("Fifo Reading SOC!\n");
  
  rc = fpga.openMem();
  if( rc<0 ){
    printf("Closing prog\n");
    return 1;
  }

  printf("Set LEDs\n");
  // toggle the LEDs a bit
  for(idx=0; idx<8; idx++){
    fpga.setLEDs(1<<idx);
    usleep(50*1000);
  }
  fpga.setLEDs(0);

  printf("Print status\n");
  fpga.printStatus();
    
  printf(" Cleaning output fifo\n");
  fpga.cleanUpOFifo();
  fpga.cleanUpORegFifo();
  fpga.cleanUpOFifo();
  fpga.cleanUpORegFifo();
  //  fpga.printFifoStatus();

  printf("\n\n Check firmware \n");
  uint32_t fwver = fpga.ReadReg(16);
  printf("Firmware version: %x\n", fwver);

  // check status and go to CONFIG
  printf("\n\n Check current status\n");
  uint32_t status = fpga.ReadReg(17) & 7;
  if( status>7 ){
    printf("Error reading register 16!!\n");
    fpga.PrintAllRegs();
    fpga.closeMem();	
    return 1;
  }
    
  while( status!=1){
    if( status==0){
      fpga.sendGoTo(IdleToConfig);
    } else if( status==3){
      fpga.sendGoTo(RunToConfig);
    } else  if( status==5){
      fpga.cleanUpOFifo();
    } else {
      printf("waiting for a status change \n");
      sleep(1);
    }
    usleep(100);
    status = fpga.ReadReg(17) & 0x7;	
  }
  // here the FPGA FSM is in the CONFIG status

  // Prepare shared Data Object
  EventCircularBuffer cb(250000); // 1MB memory
  SharedData sd;
  sd.fpga = &fpga;
  sd.cb = &cb;
  sd.verbosity = verbosity;
  sd.isRunning = false;
  pthread_mutex_init(&sd.lock, NULL);
  pthread_cond_init(&sd.cond, NULL);

  // HERE GOES THE CONFIGURATION OF THE REGISTERS
  // set second header in event data (depends on the device)
  fpga.sendWriteReg(2,0x00eade00);
  // set transition to run timing
  fpga.sendWriteReg(4,0x44);


  printf("Start with SW0=0 and SW3=0\n");
    //wait for the right combination of switches
  int sw= fpga.getSwitches();
  while( (sw&9)!=0 ){
    if( (sw&8)==8 ){
      printf("Got reset; closing \n");
      rc = fpga.closeMem();	
      return( rc );
    }
    // wait 2.0 s
    usleep( 2000*1000 );
    // read switch
    sw= fpga.getSwitches();
    fpga.printStatus();
    fpga.printFifoStatus();
  }

  
  printf("\n\nReading all regs\n\n");
  fpga.PrintAllRegs();
  fpga.printFifoStatus();


  // prepare for RUNNING
  sd.isRunning = true;

  pthread_t prod, cons;
  pthread_create(&prod, NULL, &producer, (void*) (&sd));
  pthread_create(&cons, NULL, &consumer, (void*) (&sd));

  printf("\n\nGo to run mode\n\n");
  fpga.sendGoTo(ConfigToRun);    // GO TO RUN MODE
  sleep(1);

  fpga.printFifoStatus();
  int events = 0,counts=0;  
  int wordsread = 0;
  time_t start,stop;
  time(&start);
  bool printdone=false;

  // event loop.... should just respond to slow control here.
  // everything is done in the producer/consumer threads
  //
  while(nmax>0){
    sw= fpga.getSwitches();
    if( (sw&8)==8 ) break;
    if( fpga.OFifoEmpty() || (fpga.getOFifoStatus()&0xffff)==0 ){
      // no action... or just print FIFO status??
      counts++;
      usleep(20*1000);  // 0.02 s sleep
      if( (counts%1000)==0 && verbosity>0 ) fpga.printFifoStatus(); 
      if( (events%10)==0 && fpga.OFifoRegEmpty() && !printdone && verbosity>0){
	fpga.PrintAllRegs();	
	printf("Event read: %d, events ok: %d, word transferred: %d\n",
	       sd.eventsOut, sd.eventsOK, sd.wordsread);
	printdone=true;
      }
    } else  {
      if( (counts%10000)==1 ) fpga.printStatus();
      printdone=false;
      counts = 0;
      usleep(20*1000);  // 0.02 s sleep
    }	
  }

  // end of run called: evaluate rates and close down
  time(&stop);
  double diff = difftime(stop,start);
  if( diff<1) diff=1.;
  double rate=wordsread*4/diff;
  printf("Read %d events, %d words in %f seconds for a rate of %f\n", events, wordsread, diff, rate);

  // end of run statistics
  printf("Event read: %d, events ok: %d, word transferred: %d\n",
	 sd.eventsOut, sd.eventsOK, sd.wordsread);
  fpga.PrintAllRegs();	

  printf("\n\nGo to config mode\n\n");
  fpga.sendGoTo(RunToConfig);
  printf("\n\nGo to Idle mode\n\n");
  fpga.sendGoTo(ConfigToIdle);
  // end threads
  sd.isRunning = false;
  pthread_join(prod, NULL);
  pthread_join(cons, NULL);

  // clean up our memory mapping and exit
	
  printf("Got reset; closing \n");
  fpga.closeMem();	

  return( rc );
}
