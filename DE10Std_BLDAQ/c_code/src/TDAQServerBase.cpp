/*****************************/
/*                           */
/* Date: September 2017      */
/* Author: S. Biondi         */
/*****************************/

/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/

/* Base (and actual) class for the network comunication and handling of the actual server interface
 */

#include "TDAQServerBase.hh"
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <string.h>
#include <iostream>
#include <iomanip>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>

using namespace std;

//---------------------------------------------------------
// constructor: it prepares the environment for a call to runServer()
TDAQServerBase::TDAQServerBase(std::vector<std::string> & arguments, 
			       DAQServerInterface* servInt){
	
  if (arguments.size() < 2) {
    std::cerr<<"slowInfo:: ERROR, no port provided"<<std::endl;
    exit(1);
  }

  // assigning arguments passed to the code
  m_ipinterface = arguments.at(1);
  m_portno = atoi( (arguments.at(2)).c_str() );
  m_verbose = atoi( (arguments.at(3)).c_str() );
  if( arguments.size()>5 ){
    m_blockSize = atoi( (arguments.at(5)).c_str() );
	if( m_blockSize>1000000 ) m_blockSize = 1000000;
    std::cout<<" Transferring data with a chunk size of "<<m_blockSize<<std::endl;
  } else {
	m_blockSize = 20000;
  }
  std::cout<<" Transferring data with a chunk size of "<<m_blockSize<<std::endl;
  // data server  
  p_server = servInt;    	

  // Prepare Data storage
  m_cb = new EventCircularBuffer(250000); // 1MB memory

  // all the rest: internal data initializazion
  m_isRunning = false;      // set to no running
  m_wordsread = 0;
  m_eventsOut = 0;

  // initialize to null all TCP pointers
  slowStream = NULL;
  slowAcceptor = NULL;
  fastStream = NULL;
  fastAcceptor = NULL;
  
  // lock initialization
  pthread_mutex_init(&m_lock, NULL);
}



//---------------------------------------------------------
TDAQServerBase::~TDAQServerBase(){
  // what needs to be done here??
  if( fastStream!=NULL)   delete fastStream;
  if( fastAcceptor!=NULL) delete fastAcceptor;
  if( slowStream!=NULL)   delete slowStream;
  if( slowAcceptor!=NULL) delete slowAcceptor;
}



//---------------------------------------------------------
// actual call to run the server
// open the slow connection (configure, monitoring, start the running mode)
void TDAQServerBase::runServer(){  // contains code from slowInfo and dostuff

  uint32_t msgcounter =0;
  //************************************
  if( p_server->initialize()!=0 ){
    error("slowInfo:: ERROR on DAQ initialization ");
  }

  if ( m_verbose )  cout << "runServer :: initializing connection on port :"<<(std::dec)<<m_portno<<std::endl;
  slowAcceptor = new TCPAcceptor(m_portno);

  if (slowAcceptor->start() == 0) {
    while (1) {
      slowStream = slowAcceptor->accept();
      if (slowStream != NULL) {
        if ( m_verbose )   cout << "runServer:: new connection"<<std::endl;
	
        ssize_t len;
        // while ((len = slowStream->receive(m_slowBuffer, sizeof(m_slowBuffer), 0)) > 0) {
		// get data with a 0.5 ms timeout
        len = slowStream->receive(m_slowBuffer, sizeof(m_slowBuffer), 500);
        if( len > 0) {
		  msgcounter++;
          m_slowBuffer[len] = 0;
		  m_numChars = len;
          std::cout<<"received - "<<m_slowBuffer<< " length="<<len<<std::endl;
          
          if( msgcounter<20 ) 
			std::cout<<"runServer :: Here is the message: "<<m_slowBuffer<<std::endl;
          string param(m_slowBuffer);
          
          // configuring settings
          if ( param.find("CONFIG") != string::npos ) {
            configure();
          }
          // reading settings
          else if ( param.find("PUBLISH") != string::npos ){
            publish();
          }
          // telling server to prepare for data flux
          else if ( param.find("RUN") != string::npos ){ 
            run();
          }
          // stop the acquisition
          else if ( param.find("STOPDC") != string::npos ) {
            stopDC();
          }	
        }
	
        delete slowStream;      
        slowStream = NULL;

      } else {
        cout << "runServer:: error on new connection "<<std::endl;
      }
    }  // end of while
  } else {
    cout << "runServer:: error on listen "<<std::endl;
  }

  p_server->shutdown();
}


//---------------------------------------------------------
// process command configure
void TDAQServerBase::configure(){

  cout << "configure:: Receiving instructions for call: CONFIGURE" << endl;

  std::vector<uint32_t> param;
  loadVectorFromBuffer(param);
  for(int i=0; i<param.size(); i+=2){
	  cout<< "reg="<<param[i]<<" - value="<<(std::hex)<<param[i+1]<<std::endl;
  }
  p_server->configure(param);  // here a vector<int> is needed!!
  
  char line[256];
  snprintf(line,256,"configure:: CONFIGURE status: doing nothing now");

  ssize_t n = slowStream->send(line, strlen(line));
  if ( n < 0) error("configure:: ERROR writing to socket");
}


//---------------------------------------------------------
// process command run
void TDAQServerBase::run(){
  std::vector<uint32_t> param;
  loadVectorFromBuffer(param);

  m_eventsOut = 0;
  m_runNumber = 0;

  if( param.size()>0 ) m_runNumber = param[0];
  param.erase(param.begin());

  cout << "run:: Receiving instructions for call: RUN  " <<  m_runNumber <<endl;
  cout << "run:: RUN == prepareForRun status of the machine" << endl;
  cout << "run:: prepareForRun status:: passing some parameters to server" << endl << endl;
  cout<<"Received parameters ["<<param.size()<<"] :  ";
  for(unsigned int i=0; i<param.size(); ++i){
    cout<< param[i] <<" - ";
  }
  cout<<endl;
  cout<< "words available: "<<p_server->wordsAvailable()<<endl;
  cout << endl << "TDAQServerBase::run  Go to run mode" << endl << endl;
  m_isRunning = true;
  
  
  p_producer = new std::thread(&TDAQServerBase::producerAndConsumer,this);  // this needs to be checked!!
  
  usleep(10000);// 10 ms
  
  
//  p_producer = new std::thread(&TDAQServerBase::producer,this);  // this needs to be checked!!

  

  // open the thread for data flux from sub-detector to DAQ
  //cout << "run:: connecting the data thread..." << endl << endl; 
  //p_dataFlux = new std::thread(&TDAQServerBase::dataFlux,this);
    
  //    cout << "run:: waiting for the thread to exit..." << endl << endl; 
  //    p_dataFlux->join();
  //    cout << "run:: thread finished..." << endl << endl; 
  string readInfo = "run:: prepareForRun status:: passing some parameters to server.";
							  
  ssize_t n = slowStream->send(readInfo.c_str(), readInfo.size());
  if (n < 0) error("run:: ERROR writing to socket");
														 
  // MV to be checked if this should go here
  //p_server->GoToRun();    // GO TO RUN MODE
  p_server->GoToRun(m_runNumber, param);    // GO TO RUN MODE
}


//---------------------------------------------------------
// process command stopdc
void TDAQServerBase::stopDC(){
  char line[256];
  cout << "stopdc:: Receiving instructions for call: STOPDC" << endl;
  cout << "stopdc:: STOPDC == passing some parameters to server" << endl;
  cout << "stopdc:: STOPDC status:: closing the data thread" << endl;
  // ?? this needs o be checked: the data server should be closed by the FAST data connection
  if( m_isRunning ){
    p_server->StopDC();
    m_isRunning = false;
    snprintf(line,255,"stopDC:: STOPDC status: got a stop sending data from fast connection");
    
    ssize_t n = slowStream->send(line, strlen(line));
    if ( n < 0) error("stopDC:: ERROR writing to socket");
    
  } else {
    
    snprintf(line,256,"stopdc:: STOPDC status: stop sending data");
    ssize_t n = slowStream->send(line, strlen(line));
    if (n < 0) error("stopdc:: ERROR writing to socket");
    
  }
  
  m_isRunning = false;
  sleep(1);
  
  cout << "Waiting for the threads to join" << endl << endl;
  if ( p_producer->joinable() )   p_producer->join();
  cout << "producer joined" << endl << endl;
  //  if ( p_consumer->joinable() )   p_consumer->join();
  cout << "Threads finished" << endl << endl;
}


//---------------------------------------------------------
// process command publish
void TDAQServerBase::publish(){

  if ( m_verbose ) cout << "publish:: Receiving instructions for call: PUBLISH" << endl << endl;
  std::vector<uint32_t> param;
  loadVectorFromBuffer(param);
  std::vector<uint32_t> param2=param;
  for(unsigned int i=0; i<param2.size(); i++){
    if( param2[i]>31 ) param2[i]=0;  //0 is a place holder
  }
  
  std::vector<uint32_t> results;
  p_server->publish(param2,results);  // here a vector<int> is needed!!
  if ( m_verbose ) cout << "publish:: got "<<param.size()<<" data to publish" << endl;
  //
  uint32_t* pvalues = new uint32_t[results.size()];
  for(unsigned int i=0; i<results.size(); i++){
    pvalues[i]= results[i];
    if( param[i]==0x1000 ) pvalues[i] = m_wordsWaiting;
    if( param[i]==0x1001 ) pvalues[i] = m_wordsread;
    if( param[i]==0x1002 ) pvalues[i] = 0;
    if( param[i]==0x1003 ) pvalues[i] = 0;
  }

  size_t len = results.size()*sizeof(uint32_t);
  ssize_t n = slowStream->send( (char*) pvalues, len);

  if (n < 0) error("publish:: ERROR writing to socket");
  if ( m_verbose ) cout << "publish:: end" << endl << endl;	
}


//---------------------------------------------------------
// load data from exchanged buffer
void TDAQServerBase::loadVectorFromBuffer(vector<uint32_t> & param){
  int numInts = (m_numChars-8)/4;
  uint32_t * p = (uint32_t*)(&m_slowBuffer[8]);
  
  if( numInts>0 ){
	uint32_t value;
    for(int i=0; i<numInts; i++){
	  value = *p;
//	  if( value<0x2000 ){
        param.push_back(*p);
        p++;
//	  }
    }
  } else {
    param.push_back(0);
  }
}

 
//---------------------------------------------------------
// questa proprio non mi piace:!! ?? da modificare
// function that creates a train of random number of random events
void TDAQServerBase::readProducerData () {
  
  std::vector<uint32_t> evt;
  
  if ( m_verbose )  cout << "dataFlux:: waiting for the lock"<<endl;
  pthread_mutex_lock(&m_lock);
  if ( m_verbose )  cout << "dataFlux:: locked"<<endl;
  
  uint32_t toread=(m_cb->size() <2000 ? m_cb->size() : 2000); //read no more than 2000 ints
  if ( m_verbose )  cout << "dataFlux:: before reading from cb " 
			 << toread << " words"<<endl;
  m_cb->read(evt,toread);
  
  //  pthread_cond_broadcast(&m_cond); // wake up consumer
  pthread_mutex_unlock(&m_lock);

  uint32_t *bufTmp = new uint32_t[ evt.size() ];
  for ( unsigned int k = 0; k < evt.size(); k++ ) {
    bufTmp[k] = evt.at(k);
  }
  if ( m_verbose )  cout << "dataFlux:: before sending" << endl;
  

  // send event per event to DAQ
  size_t len = evt.size()*sizeof(uint32_t);
  int n = fastStream->send( (const char * ) bufTmp, len ); 
  if ( m_verbose )  cout << "dataFlux:: n = " << n 
			 << "   evt.size() = " << evt.size()*4 << endl;
  
  evt.erase(evt.begin(),evt.end());
}


void TDAQServerBase::error(const char *msg){
  perror(msg);
  exit(1);  // ?? so bad ??
}								
											
											



//-----------------------------------------------------------------
// Thread for reading data from FIFO
// It fills the circular buffer
// Called during run only!
//
void TDAQServerBase::producerAndConsumer(){

  // this is a thread created after a call to goto run mode!
  // set-up
  std::vector<uint32_t> evt;
  evt.reserve(250000);
  m_wordsread = 0;
  m_wordsWaiting = 0;
  m_lastbco = 0;
  m_lastevt = 0;
  m_sentevents = 0;
  std::string module(m_ipinterface.begin()+m_ipinterface.rfind(".")+1, m_ipinterface.end());
  m_checker.startRun(module, m_runNumber,false);

  if( m_verbose )  std::cout<<"PRODUCER: starting cycle, words already available "<< p_server->wordsAvailable()<<std::endl;

  time_t start_t, end_t;
  double diff_t;

  char buffer[256];
  bzero(buffer,256);
  int portno;
  // assigning arguments passed to the code
  // it's the next port wrt the slow connection
  portno = m_portno+1;
  if ( m_verbose )  
    cout << "run:: listening on port "<< portno <<endl;
  fastAcceptor = new TCPAcceptor(portno,m_ipinterface.c_str());
  
  if( fastAcceptor->start()!=0 ){
     cout<<"consumer:: listen on DATA port #"<<portno<<" failed"<<endl;
     delete fastAcceptor;
     fastAcceptor = NULL;
     return;
  }
  
  fastStream = fastAcceptor->accept();
  if (fastStream == NULL) {
    delete fastAcceptor;
    fastAcceptor = NULL;
    return;
  }

  if ( m_verbose )  
    cout << "run:: new connection"<< endl;
  usleep(1000000);
      
  // send a start of run string
  char cbuffer[21]=".,;:Start Of Run\0\0\0\0";
  *((int*)&cbuffer[0]) = 5; // 5 WORDS
  fastStream->send(cbuffer,20);

  m_numEvents=0;
  time(&start_t);

  // Main LOOP -------------------------------------
  while(m_isRunning){
    m_wordsWaiting = p_server->wordsAvailable();
    if( m_verbose ) std::cout<<"PRODUCER and CONS: wordsw: "<< m_wordsWaiting<<std::endl;
    if( m_wordsWaiting ){  
      usleep(100);  // this limits the DAQ rate to 10 kHz
      // read data from server
      if( m_verbose ) std::cout<<"Before reading"<<std::endl;
//    this activates the new event format!
	  evt.push_back(0);//message size

      p_server->readData(evt, m_blockSize); // read a chunk of data (max 10000?) from FIFO
      if( m_verbose ) std::cout<<"After reading"<<std::endl;
      m_wordsread += evt.size();
      if ( m_verbose )  std::cout<<"PRODUCER:: evt.size() ="<<evt.size()<<std::endl;
//      checker(evt);
      // send data through net

	  if( evt.size()>1 ){		
		m_numEvents++;
		// if( (m_numEvents%1000)==0 ) publish();
		if( m_numEvents<10 || (m_numEvents<100 && (m_numEvents%10)==0)
			  || (m_numEvents<10000 && (m_numEvents%100)==0)
			  || (m_numEvents<100000 && (m_numEvents%1000)==0)
			  || (m_numEvents%10000)==0 
	      			                                                  ){
		  time(&end_t);
          diff_t = difftime(end_t, start_t);	
		  double ratekwps = m_wordsread/diff_t/1000.; // units kilo-words-per-second
		  // here for some periodic statistics
		  std::cout<<"Events: "<<(std::dec)<<m_numEvents<<"    Total words: "<<m_wordsread
				<<"   Word rate:  "<<ratekwps<<"  kwps seconds:"<< diff_t<<std::endl;
		}		  
        m_wordsWaiting = p_server->wordsAvailable();

    
		uint32_t *bufTmp = evt.data();
        evt[0] = evt.size();   // send buffer size in WORDs

        m_checker.checkNEvents(evt);
		

        // temporary: dump
        //	  dumpEvent(evt);
        // send event per event to DAQ
        size_t len = evt.size()*4;
        if ( m_verbose )  cout << "consumer:: evt.size() (w)= " << evt.size() << endl;
          
        int n = fastStream->send( (const char * ) bufTmp, len ); 
        if ( m_verbose )  cout << "consumer:: n = " << n  << endl;
        if( n<0 ){
	    int errnum=errno;
	    std::cout<<"Value of errno: "<<errnum<<std::endl
				<<"Error writing to net:"<< strerror( errnum ) <<std::endl;
      }
      evt.clear();
      m_eventsOut++;
	  }
      if( m_verbose ) std::cout<<"PRODUCER: usleep 50 - 2"<<std::endl;
      usleep(500);
    }
  }  
  
  // --------------------Ending : wait & clean
  //  here when out of a run: 
  if( m_verbose )    std::cout<<"PRODUCER: end of run"<<std::endl;
  usleep(20*1000);  // 0.02 s sleep

  // clean data on remote 
  int n=1000;
  while( n>0 && p_server->wordsAvailable()>0){
    p_server->readData(evt, 25000); // read a chunk of data (max 2000?) from FIFO
    evt.clear();
    n--;
    if ( m_verbose ) std::cout << "Producer: erase the event" << std::endl;
  }
  if( p_server->wordsAvailable()>0 )
    std::cout<<"WARNING: Data are still available after 1000 reading!!"<<std::endl;

  m_checker.stopRun();
  sleep(4);

  delete fastAcceptor;
  fastAcceptor = NULL;
  delete fastStream;
  fastStream = NULL;
  // --------- end!
  std::cout<<"PRODUCER: joining"<<std::endl;
}




void TDAQServerBase::checker(std::vector<uint32_t> & evt){
  
  static uint32_t lastevt=0;
  static uint32_t errors = 0;
  static std::vector<uint32_t> oldevt;
  bool errorFound = false;
  if( m_eventsOut==0){
    lastevt=0;
    errors = 0;
    lastevt = evt[3];   
  } else {
    if( evt[0]!=evt.size() ){
      std::cout<<"Error on size on evt "<<evt[3]<<",  size:"<<evt.size()<<"  evt[0]="<<evt[0]<<std::endl;
      errors++;
      errorFound = true;
    }
    if( evt[3]!=lastevt+1  ){
      std::cout<<"Error on evt number "<<evt[3]<<",  (last: "<<lastevt<<")"<<std::endl;
      errors++;
      errorFound = true;
    }
    lastevt = evt[3];
    if( evt[1]!=0xeadebaba || evt[evt.size()-1]!=0xbacca000 ){
      std::cout<<"Error on header ("<<(std::hex)<<evt[1]<<") or trailer ("<<evt[evt.size()-1]<<")"<<(std::dec)<<std::endl;
      errors++;
      errorFound = true;
    }
  }
  if( errorFound ){
	std::cout<<"Dump of event:"<<std::endl;
    dumpEvent(evt);
	std::cout<<"Previous event:"<<std::endl;
    dumpEvent(oldevt);
  }
  oldevt = evt;
}


  // check event data
void TDAQServerBase::dumpEvent(std::vector<uint32_t> & evt){
  std::cout<<"Event # "<<evt[3]<<",    size: "<<evt[0]<<std::endl;
  for(unsigned int i=0; i<evt.size(); ++i){
	std::cout<<" "<<std::setw(9)<<(std::hex)<<" "<<evt[i]<<std::endl;
  }
  std::cout<<(std::dec)<<std::endl; 
}

// void TDAQServerBase::checker(std::vector<uint32_t> & evt){
  
  // static uint32_t lastevt=0;
  // static uint32_t errors = 0;
  // static std::vector<uint32_t> oldevt;
  // bool errorFound = false;
  // if( m_eventsOut==0){
    // lastevt=0;
    // errors = 0;
    // lastevt = evt[3];   
  // } else {
    // if( evt[0]!=evt.size() ){
      // std::cout<<"Error on size on evt "<<evt[3]<<",  size:"<<evt.size()<<"  evt[0]="<<evt[0]<<std::endl;
      // errors++;
      // errorFound = true;
    // }
    // if( evt[3]!=lastevt+1  ){
      // std::cout<<"Error on evt number "<<evt[3]<<",  (last: "<<lastevt<<")"<<std::endl;
      // errors++;
      // errorFound = true;
    // }
    // lastevt = evt[3];
    // if( evt[1]!=0xeadebaba || evt[evt.size()-1]!=0xbacca000 ){
      // std::cout<<"Error on header ("<<(std::hex)<<evt[1]<<") or trailer ("<<evt[evt.size()-1]<<")"<<(std::dec)<<std::endl;
      // errors++;
      // errorFound = true;
    // }
  // }
  // if( errorFound ){
	// std::cout<<"Dump of event:"<<std::endl;
    // dumpEvent(evt);
	// std::cout<<"Previous event:"<<std::endl;
    // dumpEvent(oldevt);
  // }
  // oldevt = evt;
// }

