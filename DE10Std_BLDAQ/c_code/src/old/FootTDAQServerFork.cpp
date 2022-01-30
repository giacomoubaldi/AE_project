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

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <string>
#include <sstream>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <vector>

// used in thread
#include <thread>
#include <condition_variable>
#include <queue>
#include <chrono>
#include <cstdlib>

// used in randomisation
#include <time.h>
#include <chrono>
#include "ServerInterface.hh"

using namespace std;

std::thread dataFluxThread;
bool verbose;
bool simulation;
pthread_t prod;

// Maximum event size allowed (in words)
const uint32_t MaxEventSize  = 50000;

// structure to hold data shared between producer, consumer and main
struct SharedData {
  SOCInterface *fpga;
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

SharedData *sd;   // memoria globale!!



void dostuff (int, int, int, vector<string>, bool );
void simulationEvent ( int, int );
void readProducerData( int, int);

void error(const char *msg)
{
  perror(msg);
  exit(1);
}


// open the thread for slow connection (configure, monitoring, start the running mode)
void slowInfo(int argc, vector<string> arguments) {

  SOCInterface Fpga;
  //************************************
  if( Fpga.initialize()!=0 ){
    error("slowInfo:: ERROR on FPGA initialization ");
  }

  sd = new SharedData;
  // Prepare shared Data Object
  EventCircularBuffer cb(250000); // 1MB memory
  sd->fpga = &Fpga;
  sd->cb = &cb;
  sd->verbosity = verbosity;
  sd->isRunning = false;

  //----------------------------------------
  int sockfd, newsockfd, portno, pid;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;

  if (argc < 2) {
    fprintf(stderr,"slowInfo:: ERROR, no port provided\n");
    exit(1);
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("slowInfo:: ERROR opening socket");

  bzero((char *) &serv_addr, sizeof(serv_addr));

  // assigning arguments passed to the code
  portno = atoi( (arguments.at(1)).c_str() );
  verbose = atoi( (arguments.at(2)).c_str() );
  simulation = atoi( (arguments.at(3)).c_str() );

  if ( verbose )  cout << "slowInfo:: portno = " << portno << endl;
  
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  int b = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

  if ( verbose )  cout << "slowInfo:: binding = " << b << endl;

  if (b < 0) 
    error("slowInfo:: ERROR on binding");

  if ( verbose )  cout << "slowInfo:: before listen" << endl;

  int l = listen(sockfd,5);

  if ( verbose ) { 
    cout << "slowInfo:: sockfd = " << sockfd << endl;
    cout << "slowInfo:: listening = " << l << endl;
  }

  clilen = sizeof(cli_addr);
  
  if ( verbose )  cout << "slowInfo:: before accept" << endl;

  while (1) {
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) 
      error("slowInfo:: ERROR on accept");
    signal(SIGCHLD, SIG_IGN);
    pid = fork();
    if (pid < 0)
      error("slowInfo:: ERROR on fork");
    if (pid == 0)  {
      close(sockfd);
      dostuff(newsockfd, sockfd, argc, arguments, verbose);
      exit(0);
    }    else close(newsockfd);
  }

  close(sockfd);
  //********************************
  Fpga.shutdown();
}


// apri thread per connessione veloce (dati)
void dataFlux( int argc, vector<string> arguments){

  int sockfd = -9;
  int newsockfd = -9;
  int portno = 0;
  socklen_t clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  int n;

  if (argc < 2) {
      fprintf(stderr,"dataFlux:: ERROR, no port provided\n");
      exit(1);
  }

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
      error("dataFlux:: ERROR opening socket");
  bzero((char *) &serv_addr, sizeof(serv_addr));
  
  // assigning arguments passed to the code
  portno = atoi( (arguments.at(1)).c_str() );
  portno++;
  verbose = atoi( (arguments.at(2)).c_str() );
  simulation = atoi( (arguments.at(3)).c_str() );
  
  if ( verbose )  cout << "dataFlux:: portno = " << portno << endl;

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if ( verbose )  cout << "dataFlux:: before binding" << endl;

  int b = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
  if ( verbose )  cout << "dataFlux:: binding = " << b << endl;
  if (b < 0) 
      error("dataFlux:: ERROR on binding");
  
  if ( verbose ) {
    cout << "dataFlux:: before listen" << endl;
    cout << "dataFlux:: sockfd = " << sockfd << endl;
  }

  int l = listen(sockfd,5);

  if ( verbose )  cout << "dataFlux:: listening = " << l << endl;

  clilen = sizeof(cli_addr);

  if ( verbose )  cout << "dataFlux:: before accept" << endl;

  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
  if (newsockfd < 0) 
    error("dataFlux:: ERROR on accept");
  bzero(buffer,256);
  if ( verbose )  cout << "dataFlux:: before read" << endl;

  int result=0;
  string param = "";
  
  sd->eventsOut =0;          // events read out
  sd->eventsOK  =0;          // events with good tail

  do {
    bzero(buffer,256);
    param = "";
    // making the socket not blockable: setting a timeout for reading function
    fd_set tempset;
    FD_ZERO(&tempset);
    FD_SET(newsockfd, &tempset);

    // initialise timeout struct
    struct timeval timeout;
    timeout.tv_sec  = 1;
    timeout.tv_usec = 1 * 1000;
    result = select(newsockfd + 1, &tempset, NULL, NULL, &timeout);

    // Check status: making the socket not blockable, setting a timeout for reading function
    if (result == -1) {
      cout << "dataFlux:: Something went wrong!" << endl;
      exit(1);
    } else if ( result == 0 ) {
      cout << "dataFlux:: Timeout: waiting for the DAQ to free some space." << endl;
    } else if (result > 0 && FD_ISSET(newsockfd, &tempset)) {
      n = read(newsockfd,buffer,255);
    }

    if (n < 0){
      error("dataFlux:: ERROR reading from socket");
    } else {
      if ( verbose )  cout << "dataFlux:: Here is the message: " << buffer << endl;

      param.resize(strlen(buffer));
      for( unsigned int i=0; i < strlen(buffer); i++ ) {  
        param.at(i) = buffer[i];
      }
      
      if ( param.find("DATA") != string::npos ) {
  
        if ( verbose ) {
          cout << "dataFlux:: DATA status: giving data to the TDAQ" << endl;
          cout << "dataFlux:: Receiving instructions for call: DATA\n" << endl;
          cout << "dataFlux:: DATA:: sending raw events to TDAQ\n" << endl;
        }

        if ( simulation )   simulationEvent( n, newsockfd );
        else {
          // sub-detector-dependent part
	  readProducerData ( n, newsockfd );
        }
        
  
      if (n < 0) error("dataFlux:: ERROR writing to socket");
      } 
    }
  } while ( result==0 || param.find("DATA") != string::npos  );
  
  if ( param.find("DATA") == string::npos ) {
    
    if ( verbose ) {
      cout << "dataFlux:: Receiving instructions for call: " << param << endl;
      cout << "dataFlux:: END: stop sending the data to the client." << endl;
    }
    
    close(newsockfd);
    close(sockfd);
  }

}




//-----------------------------------------------------------------
// Thread for reading data from FIFO
// It fills the circular buffer
// Called during run only!
//
void *producer(void *arg){
  //  SharedData *sd = (SharedData*) arg;
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




// main function calling the thread for slow connection:
// it will call the fast connection 
int main( int argc, char *argv[] ){

  vector<string> arguments;
  arguments.resize(argc);

  for ( unsigned int i = 0; i<arguments.size(); i++ )
    arguments.at(i) = "";

  for ( int i = 0; i< argc; i++ ){
    string tmp = argv[i];
    arguments.at(i) = tmp;
  }

  // start thread for sharing slow info: configure and monitoring
  cout << "Connecting the slow thread..." << endl << endl; 
  std::thread slowInfoThread(slowInfo, argc, arguments );

  this_thread::sleep_for(chrono::seconds(3));
  slowInfoThread.join();

  return 0;

}


/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connection has been established.
 *****************************************/
void dostuff (int sock, int sockfd, int argc, vector<string> arguments, bool verbose)
{
  int n;
  char buffer[256];

  bzero(buffer,256);
  n = read(sock,buffer,255);
  if (n < 0) error("slowInfo:: ERROR reading from socket");

  if ( verbose )  cout << "slowInfo:: Here is the message: " << buffer << endl;

  string param = "";
  param.resize(strlen(buffer));
  for( unsigned int i=0; i < strlen(buffer); i++ ) {
      param.at(i) = buffer[i];
  }

  // configuring settings : CONFIGURE == keep the parameters
  if ( param.find("CONFIGURE") != string::npos ) { 
      cout << "slowInfo:: Receiving instructions for call: CONFIGURE" << endl << endl;
      Fpga.configure(param);  // here a vector<int> is needed!!

      n = write(sock,"slowInfo:: CONFIGURE status: doing nothing for now",50);
    if (n < 0) error("slowInfo:: ERROR writing to socket");
  }

  // reading settings : PUBLISH == send to DAQ the sub-detector info for online monitor
  else if ( param.find("PUBLISH") != string::npos ){ 
    cout << "slowInfo:: Receiving instructions for call: PUBLISH" << endl << endl;


???
    std::vector<int> results;
    Fpga.publish(param,results);  // here a vector<int> is needed!!

    //
    int* pvalues= new int[results.size()];
    int* p=pvalues;
    for(int i=0; i<results.size(); i++){
      pvalues[i]= results[i];
    }

    n = write(sock, (char*) pvalues, results.size()*sizeof(int));

    if (n < 0) error("slowInfo:: ERROR writing to socket");
  }

  // telling server to prepare for data flux
  else if ( param.find("RUN") != string::npos ){ 

    cout << "slowInfo:: Receiving instructions for call: RUN" << endl;
    cout << "slowInfo:: RUN == prepareForRun status of the machine" << endl;
    cout << "slowInfo:: prepareForRun status:: passing some parameters to server" << endl << endl;

    string readInfo = "slowInfo:: prepareForRun status:: passing some parameters to server.";
    n = write(sock, readInfo.c_str(), readInfo.size());

    if (n < 0) error("slowInfo:: ERROR writing to socket");

    printf("\n\nGo to run mode\n\n");
    sd->isRunning = true;
    pthread_create(&prod, NULL, &producer, (void*) (&sd));
    fpga.GoToRun();    // GO TO RUN MODE
    usleep(10000);// 10 ms
    
    // open the thread for data flux from sub-detector to DAQ
    cout << "slowInfo:: connecting the data thread..." << endl << endl; 
    dataFluxThread = std::thread(dataFlux, argc, arguments );
    dataFluxThread.join();
  }

  // stop the acquisition : STOPDC == pass the parameter to detector for stop-sending-data call 
  else if ( param.find("STOPDC") != string::npos ) {

    cout << "slowInfo:: Receiving instructions for call: STOPDC" << endl;
    cout << "slowInfo:: STOPDC == passing some parameters to server" << endl;
    cout << "slowInfo:: STOPDC status:: closing the data thread" << endl;
    fpga.StopDC();
    n = write(sock,"slowInfo:: STOPDC status: stop sending data",50);
    if (n < 0) error("slowInfo:: ERROR writing to socket");
  
    if ( dataFluxThread.joinable() )   dataFluxThread.join();
  }

}

// function that creates a train of random number of random events
void readProducerData ( int n, int newsockfd ) {
  
  std::vector<uint32_t> evt;

  pthread_mutex_lock(&sd->lock);
  
  uint32_t toread=(sd->cb->size() <2000 ? sd->cb->size() : 2000); //read no more than 2000 ints
  sd->cb->read(evt,toread);
  
  //  pthread_cond_broadcast(&sd->cond); // wake up consumer
  pthread_mutex_unlock(&sd->lock);

  uint32_t *bufTmp = new uint32_t[ evt.size() ];
  for ( unsigned int k = 0; k < evt.size(); k++ ) {
    bufTmp[k] = evt.at(k);
  }

  // send event per event to DAQ
  n = write(newsockfd, bufTmp, evt.size()*4 ); 
  if ( verbose )  cout << "dataFlux:: n = " << n << "   evt.size() = " << evt.size()*4 << endl;

  evt.erase(evt.begin(),evt.end());

}


// function that creates a train of random number of random events
void simulationEvent ( int n, int newsockfd ) {

  unsigned int totalEvents = 0;
  srand( time(NULL) );
  unsigned int nEvents = 0;
  vector<uint32_t> evt;

  // generating a random number of events from 0 to 10
  while ( nEvents < 1 ){ 
    nEvents = rand() %10 +1;
    if ( verbose )  cout << "dataFlux:: nEvents = " << nEvents << endl;
  }

  // creating a train of events
  for ( unsigned int i = 0; i<=nEvents; i++ ) {
    
    if ( verbose )  cout << "dataFlux:: event number = " << i << endl;

    // generateEvents( totalEvents, evt );
     uint32_t nWords = (uint32_t)(rand() %25 +5);
    if ( verbose )  cout << "dataFlux:: nWords = " << nWords << endl;
    evt.resize(nWords);
    if ( verbose )  cout << "dataFlux:: evt size = " << evt.size() << endl;
            
    // filling randomly the event
    for ( unsigned int j = 0; j<nWords; j++ ) {   
      evt.at(j) = 0xaa000000 | ((totalEvents% 256)<<16) | (uint32_t)(rand() %20 +1);

      if ( j == 0 )   evt.at(j) = nWords;
      
      if ( j == 1 ) {
        evt.at(j) = 0xeadebaba;
      }
      
      if ( j == 2 )   evt.at(j) = (uint32_t)totalEvents;

      if ( j == nWords-2 ) {
        evt.at(j) = 0xfafefafe;
      }
      
      if ( j == nWords-1 ) {
        evt.at(j) = 0xbacca000;
      }
      
      if ( verbose )  cout << "dataFlux:: evt.at(" << j << ") = " << (std::hex)<<evt.at(j) << endl;
    }


    uint32_t *bufTmp = new uint32_t[ evt.size() ];
    for ( unsigned int k = 0; k < evt.size(); k++ ) {
      bufTmp[k] = evt.at(k);
    }

    // send event per event to DAQ
    n = write(newsockfd, bufTmp, evt.size()*4 ); 
    if ( verbose )  cout << "dataFlux:: n = " << n << "   evt.size() = " << evt.size()*4 << endl;
    
    // clear the vector of words of the event -> ready to store words of the next even
    evt.clear(); 
    
    totalEvents++;
  } 

}
