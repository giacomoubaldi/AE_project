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
    fprintf(stderr,"slowInfo:: ERROR, no port provided\n");
    exit(1);
  }

  // assigning arguments passed to the code
  m_portno = atoi( (arguments.at(1)).c_str() );
  m_verbose = atoi( (arguments.at(2)).c_str() );

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
};



//---------------------------------------------------------
// actual call to run the server
// open the slow connection (configure, monitoring, start the running mode)
void TDAQServerBase::runServer(){  // contains code from slowInfo and dostuff

  //************************************
  if( p_server->initialize()!=0 ){
    error("slowInfo:: ERROR on DAQ initialization ");
  }

  if ( m_verbose )  cout << "runServer :: initializing connection on port :"<<m_portno<<std::endl;
  slowAcceptor = new TCPAcceptor(m_portno);

  if (slowAcceptor->start() == 0) {
    while (1) {
      slowStream = slowAcceptor->accept();
      if (slowStream != NULL) {
        if ( m_verbose )   cout << "runServer:: new connection"<<std::endl;
	
        ssize_t len;
        // while ((len = slowStream->receive(m_slowBuffer, sizeof(m_slowBuffer), 0)) > 0) {
        if ((len = slowStream->receive(m_slowBuffer, sizeof(m_slowBuffer), 0)) > 0) {
          m_slowBuffer[len] = 0;
          printf("received - %s\n", m_slowBuffer);
          
          printf("runServer :: Here is the message: %s\n",m_slowBuffer);
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
  p_server->configure(param);  // here a vector<int> is needed!!
  
  char line[256];
  snprintf(line,256,"configure:: CONFIGURE status: doing nothing now");

  ssize_t n = slowStream->send(line, strlen(line));
  if ( n < 0) error("configure:: ERROR writing to socket");
}


//---------------------------------------------------------
// process command run
void TDAQServerBase::run(){
  cout << "run:: Receiving instructions for call: RUN" << endl;
  cout << "run:: RUN == prepareForRun status of the machine" << endl;
  cout << "run:: prepareForRun status:: passing some parameters to server" << endl << endl;

  string readInfo = "run:: prepareForRun status:: passing some parameters to server.";

  ssize_t n = slowStream->send(readInfo.c_str(), readInfo.size());
  if (n < 0) error("run:: ERROR writing to socket");

  cout << endl << "TDAQServerBase::run  Go to run mode" << endl << endl;
  m_isRunning = true;

  p_server->GoToRun();    // GO TO RUN MODE

  p_producer = new std::thread(&TDAQServerBase::producer,this);  // this needs to be checked!!

  usleep(10000);// 10 ms

  // open the thread for data flux from sub-detector to DAQ
  cout << "run:: connecting the data thread..." << endl << endl; 
  p_dataFlux = new std::thread(&TDAQServerBase::dataFlux,this);
    
  //    cout << "run:: waiting for the thread to exit..." << endl << endl; 
  //    p_dataFlux->join();
  //    cout << "run:: thread finished..." << endl << endl; 
}


//---------------------------------------------------------
// process command stopdc
void TDAQServerBase::stopDC(){
  char line[256];
  cout << "stopdc:: Receiving instructions for call: STOPDC" << endl;
  cout << "stopdc:: STOPDC == passing some parameters to server" << endl;
  cout << "stopdc:: STOPDC status:: closing the data thread" << endl;
  // ?? this needs o be checked: the data server should be closed by the FAST data connection
  if( m_isRunning){
    p_server->StopDC();
    snprintf(line,256,"stopDC:: STOPDC status: got a stop sending data from fast connection");
    
    ssize_t n = slowStream->send(line, strlen(line));
    if ( n < 0) error("stopDC:: ERROR writing to socket");
    
  } else {
    
    snprintf(line,256,"stopdc:: STOPDC status: stop sending data");
    ssize_t n = slowStream->send(line, strlen(line));
    if (n < 0) error("stopdc:: ERROR writing to socket");
    
  }
  if ( p_dataFlux->joinable() )   p_dataFlux->join();
}


//---------------------------------------------------------
// process command publish
void TDAQServerBase::publish(){

  cout << "publish:: Receiving instructions for call: PUBLISH" << endl << endl;
  std::vector<uint32_t> param;
  loadVectorFromBuffer(param);
  std::vector<uint32_t> results;
  p_server->publish(param,results);  // here a vector<int> is needed!!

  //
  uint32_t* pvalues = new uint32_t[results.size()];
  for(unsigned int i=0; i<results.size(); i++){
    pvalues[i]= results[i];
  }

  size_t len = results.size()*sizeof(int);
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
    for(int i=0; i<numInts; i++){
      param.push_back(*p);
      p++;
    }
  } else {
    param.push_back(0);
  }
}

//---------------------------------------------------------
// apri thread per connessione veloce (dati)
void TDAQServerBase::dataFlux(){

  char buffer[256];
  int portno;
  // assigning arguments passed to the code
  // it's the next port wrt the slow connection
  portno = m_portno+1;

  fastAcceptor = new TCPAcceptor(portno);
  
  if( fastAcceptor->start()==0 ){
    //  while (1) { // no while: just ONE fast connection
    fastStream = fastAcceptor->accept();
    
    if (fastStream != NULL) {
      if ( m_verbose )  
      	cout << "run:: new connection"<< endl;
      
      do {

        // initialise timeout 
        int timeout = 10 * 1000;   // 10 ms waiting time

        ssize_t n = fastStream->receive(buffer, 255, timeout);
      	// ssize_t n = fastStream->receive(buffer, 255);

      	if (n < 0){
      	  error("dataFlux:: ERROR reading from socket");
      	} else {
      	  if ( m_verbose )  
            cout << "dataFlux:: Here is the message: " << buffer << endl;
	  
          if ( strncmp(buffer, "DATA", 4)==0   ) {
            if ( m_verbose ) {
      	      cout << "dataFlux:: Receiving instructions for call: DATA\n" <<endl;
      	      cout << "dataFlux:: DATA:: sending raw events to TDAQ\n" << endl;
  	        }	  
          // sub-detector-dependent parameters
          readProducerData ();	  
          }
  	  
          if (n < 0) error("dataFlux:: ERROR writing to socket");
        }
	
      } while ( strncmp(buffer, "DATA",4)==0  );
      
      if ( m_verbose ) {
      	cout<<"dataFlux:: Receiving instructions for call: " << buffer << endl;
      	cout<<"dataFlux:: END: stop sending the data to the client." << endl;
      }
      
      delete fastStream;
      fastStream = NULL;
    }
  
    // stop the producer!
    pthread_mutex_lock(&m_lock);
    m_isRunning = false;  
    p_server->StopDC();
    
    //    pthread_cond_broadcast(&m_cond); // wake up consumer
    pthread_mutex_unlock(&m_lock);
    usleep(10000);
    if(p_producer->joinable() ) p_producer->join();

    delete fastAcceptor;
    fastAcceptor = NULL;

  } else {
    cout<<"dataFlux:: listen on DATA port failed"<<endl;
  }

}

  
//-----------------------------------------------------------------
// Thread for reading data from FIFO
// It fills the circular buffer
// Called during run only!
//
void *TDAQServerBase::producer(){
  // this is a thread created after a call to goto run mode!
  std::vector<uint32_t> evt;
  m_wordsread = 0;
  m_bytesWaiting = 0;
  // if( m_verbose )  printf("PRODUCER: starting cycle, bytes already available %d \n", p_server->bytesAvailable() );
  
  while(m_isRunning){
    m_bytesWaiting = p_server->bytesAvailable();
	
    if( m_bytesWaiting==0 || m_cb->size()>m_cb->capacity()/2 ){      
      usleep(20*1000);  // 0.02 s sleep    A better idea?
    } else  {
      // here an event is waiting for us!!
      // if( m_verbose )  printf("PRODUCER: continuing cycle, bytes already available %d \n", p_server->bytesAvailable() );
      
      p_server->readData(evt, 2000); // read a chunk of data (max 2000?) from FIFO
      m_wordsread += evt.size();

      // thread safe insertion in the buffer
      pthread_mutex_lock(&m_lock);
      // if( m_verbose ){
      //   printf("PRODUCER: data in circular buffer before inserting event: %d words\n", m_cb->size());
      //   printf("PRODUCER: data to store: %d words\n", (int)evt.size());
      // }
      // check there is enough space in the buffer
      while (m_cb->capacity()<m_cb->size()+evt.size()) { //this needs to be checked!!
        // if( m_verbose ) printf("PRODUCER: inside conditional wait\n");
        pthread_cond_wait(&m_cond, &m_lock);
      }
      // if( m_verbose ) printf("PRODUCER: outside conditional wait\n");
      
      // actual writing of data in the circular buffer
      m_cb->write(evt);
      // if( m_verbose )   printf("PRODUCER: data in circular buffer now: %d words\n", m_cb->size());
	  
      pthread_mutex_unlock(&m_lock);
      // if( m_verbose )  printf("PRODUCER: after unlock\n") ;

      evt.erase(evt.begin(),evt.end());
    }
  }  
  //  here when out of a run: 
  printf("PRODUCER: end of run\n");
  usleep(20*1000);  // 0.02 s sleep

  //  clean fifo, evts and check!
  int n=1000;
  while( n>0 && p_server->bytesAvailable()>0){
    p_server->readData(evt, 2000); // read a chunk of data (max 2000?) from FIFO
    evt.erase(evt.begin(),evt.end());
    n--;
  }
  if( p_server->bytesAvailable()>0 ){
    printf("\n\nWARNING: Data are still available after 1000 reading!!\n\n");
  }
  return NULL;  // no meaning in the return data
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
			 << toread << " bytes"<<endl;
  m_cb->read(evt,toread);
  
  //  pthread_cond_broadcast(&m_cond); // wake up consumer
  pthread_mutex_unlock(&m_lock);

  uint32_t *bufTmp = new uint32_t[ evt.size() ];
  for ( unsigned int k = 0; k < evt.size(); k++ ) {
    bufTmp[k] = evt.at(k);
  }
  if ( m_verbose )  cout << "dataFlux:: before sending" << endl;
  

  // send event per event to DAQ
  size_t len = evt.size()*4 ;
  int n = fastStream->send( (const char * ) bufTmp, len ); 
  if ( m_verbose )  cout << "dataFlux:: n = " << n 
			 << "   evt.size() = " << evt.size()*4 << endl;
  
  evt.erase(evt.begin(),evt.end());
}


void TDAQServerBase::error(const char *msg){
  perror(msg);
  exit(1);  // ?? so bad ??
}
