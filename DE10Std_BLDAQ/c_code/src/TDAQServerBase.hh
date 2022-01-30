#ifndef TDAQSERVERBASE
#define TDAQSERVERBASE
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

/* Base class for the network comunication and handling of the actual server interface
*/

#include <string>
#include <vector>
#include <thread>
#include "DAQServerInterface.hh"
#include "EventCircularBuffer.h"
#include "EventChecker.hh"
#include "tcpacceptor.h"

class TDAQServerBase {
public:
  TDAQServerBase(std::vector<std::string> & arguments, DAQServerInterface* servInt);
  virtual ~TDAQServerBase();

  // actual call to run the server
  // open the thread for slow connection (configure, monitoring, start the running mode)
  virtual void runServer();

  // TDAQ transitions
  virtual void configure();
  virtual void publish();
  virtual void run();
  virtual void stopDC();
 
  // thread per connessione veloce (dati)
  //void dataFlux();  
  
  //-----------------------------------------------------------------
  // Thread for reading data from FIFO
  // It fills the circular buffer
  // Called during run only!
  //
  void producerAndConsumer();
 
  // copy data from producer to dataFlux
  void readProducerData ();

  //*************** TRYING AGAIN FORK **************************
  //  void dostuff (int sock);
  //************************************************************

protected:

  void error(const char *msg);

  // load data from exchanged buffer
  void loadVectorFromBuffer(std::vector<uint32_t> & param);
  // check event data
  void checker(std::vector<uint32_t> & evt);

  // check event data
  void dumpEvent(std::vector<uint32_t> & evt);

  // connection port
  std::string m_ipinterface;
  int m_portno;
  TCPStream*   slowStream;
  TCPAcceptor* slowAcceptor ;
  TCPStream*   fastStream;
  TCPAcceptor* fastAcceptor ;

  // local data server
  DAQServerInterface *p_server;



  // infos on DAQ items
  bool m_isRunning;
  EventCircularBuffer* m_cb;	
  uint64_t m_wordsread;
  uint32_t m_eventsOut;
  uint32_t m_numEvents;
  uint32_t m_wordsWaiting;
  uint32_t m_verbose;  
  uint32_t m_runNumber;
  uint32_t m_blockSize;  
  
  // shared buffer for slow connection
  int m_numChars;
  char m_slowBuffer[256];
  //  int m_newSockfd;
  
  // check N events
  EventChecker m_checker;
  uint32_t m_lastbco;
  uint32_t m_lastevt;
  uint32_t m_sentevents;

  // threads & sync
  std::thread * p_producer;
//  std::thread * p_dataFlux;
  
  pthread_mutex_t m_lock;
  pthread_cond_t m_cond;
};

#endif
