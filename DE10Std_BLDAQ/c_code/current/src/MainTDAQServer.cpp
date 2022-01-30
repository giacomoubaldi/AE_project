/*****************************/
/*                           */
/* Date: September 2017      */
/* Author: S. Biondi         */
/*****************************/

/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever;
   it handles one external connection at a time.
*/

#include <iostream>
#include <string>
#include <vector>
#include <time.h>
#include "SOCServerInterface.hh"
#include "SOCSimuServerInterface.hh"
#include "TDAQServerBase.hh"
//#include <signal.h>

using namespace std;

// main function calling the thread for slow connection:
// it will call the fast connection 
int main( int argc, char *argv[] ){

  vector<string> arguments;
  for ( int i = 0; i< argc; i++ ){
    arguments.push_back(string(argv[i]));
  }
  
  DAQServerInterface * p_servInt = NULL;
  if( arguments.size()<3){
    // the HPS-FPGA Handler that knows the TDAQ transitions
    p_servInt = new SOCServerInterface();
  } else {
    // the Server interface that produces SOC simulated data
    p_servInt = new SOCSimuServerInterface();
  }
  
  // main server object
  TDAQServerBase  tdaqServer(arguments, p_servInt);

  // start thread for sharing slow info: configure and monitoring
  cout << "Connecting the slow thread..." << endl << endl; 
  tdaqServer.runServer(); //actual running of the server; no return till end...

  return 0;

}
