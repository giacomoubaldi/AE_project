/*****************************/
/*                           */
/* Date: June 2020           */
/* Author: M. Villa          */
/*****************************/

/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/

/* Base (and actual) class for the network comunication and handling of the actual server interface
 */

#include "TDAQServerStandAlone.hh"
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
#include <time.h>
using namespace std;

//---------------------------------------------------------
// constructor: it prepares the environment for a call to runServer()
TDAQServerStandAlone::TDAQServerStandAlone(std::vector<std::string> & arguments, 
			       DAQServerInterface* servInt) : TDAQServerBase(arguments, servInt){
	
  if (arguments.size() < 2) {
    fprintf(stderr,"slowInfo:: ERROR, no port provided\n");
    exit(1);
  }

  m_numEvent = 0;
  m_maxEvent = m_portno; // port number (first argument) is the number of read cycles
  m_Nwords = 0;
}



//---------------------------------------------------------
TDAQServerStandAlone::~TDAQServerStandAlone(){
  
}



//---------------------------------------------------------
// actual call to run the server
// open the slow connection (configure, monitoring, start the running mode)
void TDAQServerStandAlone::runServer(){  // contains code from slowInfo and dostuff


   // Here a procedural style - one run job is done! No IO connections
  if( p_server->initialize()!=0 ){
    error("slowInfo:: ERROR on DAQ initialization ");
  }

   // step 1 
   std::cout<<std::endl<<"Step 1 -  going to config mode "<<std::endl;
   configure();
   
   // step 2
   std::cout<<std::endl<<"Step 2 -  going to run mode "<<std::endl;
   run();
   
   // step 3
   std::cout<<std::endl<<"Step 3 -  going to stop mode "<<std::endl;
   stopDC();
   
   // writeout statistics
  //************************************
  
  p_server->shutdown();
}


//---------------------------------------------------------
// process command configure
void TDAQServerStandAlone::configure(){

  cout << "configure:: Receiving instructions for call: CONFIGURE" << endl;

  std::vector<uint32_t> param;
  param.push_back(0x00000002);  // register 2
  param.push_back(0x00eade04);  // value of register 2 to configure
//  param.push_back(m_headerEB);  // value of register 2 to configure
  param.push_back(0x00000003);  // register 3
  param.push_back(0x00000001);   // value of register 3 to configure
//  param.push_back(m_busyDE0);   // value of register 3 to configure
  param.push_back(0x00000004);  // register 4
  param.push_back(0x00000044); // value of register 4 to configure
//  param.push_back(m_minClocks); // value of register 4 to configure
  param.push_back(0x00000005);  // register 5 - enable RAM interface
  param.push_back(0x00000001); // value of register 4 to configure
//  param.push_back(m_minClocks); // value of register 4 to configure
  p_server->configure(param);  // here a vector<int> is needed!!
}


//---------------------------------------------------------
// process command run
void TDAQServerStandAlone::run(){
  std::vector<uint32_t> param;

  m_eventsOut = 0;
  m_runNumber = 0;

  cout << "run:: Receiving instructions for call: RUN  " <<  m_runNumber <<endl;
  cout << "run:: RUN == prepareForRun status of the machine" << endl;
  cout<< "words available: "<<p_server->wordsAvailable()<<endl;
  m_isRunning = true;
    
  p_producer = new std::thread(&TDAQServerStandAlone::producerAndConsumer,this);  // this needs to be checked!!
  
  usleep(10000);// 10 ms
    
  string readInfo = "run:: prepareForRun status:: passing some parameters to server.";
							  
  cout << endl << "TDAQServerStandAlone::run  Go to run mode" << endl << endl;
  p_server->GoToRun(m_runNumber, param);    // GO TO RUN MODE
  
  //wait here till completion
  cout << "run:: sleeping 5000 seconds "<<endl;
  sleep(5000);
}


//---------------------------------------------------------
// process command stopdc
void TDAQServerStandAlone::stopDC(){
  cout << "stopdc:: Receiving instructions for call: STOPDC" << endl;
  cout << "stopdc:: STOPDC == passing some parameters to server" << endl;
  cout << "stopdc:: STOPDC status:: closing the data thread" << endl;

  p_server->StopDC();
  
  m_isRunning = false;
  cout << "Waiting for the threads to join" << endl << endl;
  if ( p_producer->joinable() )   p_producer->join();
  cout << "producer joined" << endl << endl;
  cout << "Threads finished" << endl << endl;
}


//---------------------------------------------------------
// process command publish
void TDAQServerStandAlone::publish(){

  if ( m_verbose ) cout << "publish:: Receiving instructions for call: PUBLISH" << endl << endl;
  std::vector<uint32_t> param;
  std::vector<uint32_t> results;
  p_server->publish(param,results);  // here a vector<int> is needed!!
  if ( m_verbose ) cout << "publish:: got "<<param.size()<<" data to publish" << endl;
  //
  uint32_t* pvalues = new uint32_t[results.size()];
  for(unsigned int i=0; i<results.size(); i++){
    pvalues[i]= results[i];
  }

  if ( m_verbose ) cout << "publish:: end" << endl << endl;	
}

 
//---------------------------------------------------------
// questa proprio non mi piace:!! ?? da modificare
// function that creates a train of random number of random events
void TDAQServerStandAlone::readProducerData () {
  
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
  
  evt.clear();
}


//-----------------------------------------------------------------
// Thread for reading data from FIFO
// It fills the circular buffer
// Called during run only!
//
void TDAQServerStandAlone::producerAndConsumer(){

  time_t start_t, end_t;
  double diff_t;
  // this is a thread created after a call to goto run mode!
  // set-up
  std::vector<uint32_t> evt;
  evt.resize(250000);
  evt.clear();
  
  m_wordsread = 0;
  m_Nwords = 0;
  m_wordsWaiting = 0;
  if( m_verbose )  printf("PRODUCER: starting cycle, words already available %d \n", p_server->wordsAvailable() );
  

  if ( m_verbose )  
    cout << "run:: producerAndConsumer starting "<< endl;
  usleep(1000000);
      
  time(&start_t);
//  int looping = 0;
  // Main LOOP -------------------------------------
  while(m_isRunning){
	//looping++;
	//if( looping<10 || (looping%100)==0 )
	//	 printf("looping %d\n",looping,m_wordsWaiting);
    m_wordsWaiting = p_server->wordsAvailable();
    //if( m_verbose && m_wordsWaiting>0 ) printf("PRODUCER and CONS: wordsw: %d  \n", m_wordsWaiting);

    if( m_wordsWaiting ){  

      // read data from server
      p_server->readData(evt, 5000); // read a chunk of data (max 2000?) from FIFO
      m_wordsread += evt.size();
      //if ( m_verbose )  std::cout<<"PRODUCER:: evt.size() ="<<evt.size()<<std::endl;

	  if( evt.size()>0 ){
		  m_numEvent++;
		  if( (m_numEvent%1000)==0 ) publish();
          m_Nwords += evt.size();
		  if( m_numEvent<10 || (m_numEvent<100 && (m_numEvent%10)==0)
			  || (m_numEvent<10000 && (m_numEvent%100)==0)
			  || (m_numEvent<100000 && (m_numEvent%1000)==0)
			  || (m_numEvent%10000)==0 
	      			                                                  ){
			time(&end_t);
            diff_t = difftime(end_t, start_t);	
		    double ratekwps = m_Nwords/diff_t/1000.; // units kilo-words-per-second
			// here for some periodic statistics
			printf("Events: %06d    Total words: %10u   Word rate:  %f  kwps seconds:%f \n",m_numEvent, m_Nwords, ratekwps, diff_t);
		  }		  
	  }
	  if( m_numEvent>m_maxEvent ) m_isRunning = false;
      checker(evt);
      evt.clear();
      m_eventsOut++;
//      if( m_verbose ) printf("PRODUCER: usleep 50 - 2\n" );
      usleep(50);
    } else {
		// no data ... sleep longer
      usleep(500);
	}
  }
  
  // --------------------Ending : wait & clean
  //  here when out of a run: 
  if( m_verbose )    printf("PRODUCER: end of run\n");
  usleep(20*1000);  // 0.02 s sleep

  // clean data on remote 
  int n=1000;
  while( n>0 && p_server->wordsAvailable()>0){
    p_server->readData(evt, 25000); // read a chunk of data (max 2000?) from FIFO
    evt.clear();
    n--;
//    if ( m_verbose ) std::cout << "Producer: erase the event" << std::endl;
  }
  if( p_server->wordsAvailable()>0 )
    printf("\n\nWARNING: Data are still available after 1000 reading!!\n\n");

  // --------- end!
  printf("PRODUCER: joining\n");
}
