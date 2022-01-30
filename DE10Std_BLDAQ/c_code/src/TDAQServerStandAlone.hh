#ifndef TDAQSERVERSTANDALONE
#define TDAQSERVERSTANDALONE
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

#include "TDAQServerBase.hh"

class TDAQServerStandAlone : public TDAQServerBase {
public:
  TDAQServerStandAlone(std::vector<std::string> & arguments, DAQServerInterface* servInt);
  virtual ~TDAQServerStandAlone();

  // actual call to run the server
  // open the thread for slow connection (configure, monitoring, start the running mode)
  virtual void runServer();

  // TDAQ transitions
  virtual void configure();
  virtual void publish();
  virtual void run();
  virtual void stopDC();
 
  //
  void producerAndConsumer();
 
  // copy data from producer to dataFlux
  void readProducerData ();

  //*************** TRYING AGAIN FORK **************************
  //  void dostuff (int sock);
  //************************************************************

protected:

  uint32_t m_numEvent;
  uint32_t m_maxEvent;
  uint32_t m_Nwords;
};

#endif
