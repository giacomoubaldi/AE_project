/*******************************/
/*                             */
/* Date: December 2017         */
/* Author: S. Biondi           */
/* Reason: test with sim only  */
/*******************************/

/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever;
   it handles one external connection at a time.
*/

#include <iostream>
#include <string>
#include <vector>
#include <time.h>
// #include "SOCServerInterface.hh"
#include "SOCSimuServerInterface.hh"
#include "TDAQServerBase.hh"

using namespace std;

// main function calling the thread for slow connection:
// it will call the fast connection 
int main( int argc, char *argv[] ){

  vector<string> arguments;
  for ( int i = 0; i< argc; i++ ){
    arguments.push_back(string(argv[i]));
  }
  int verbose = 0;
  if( argc>1 ) verbose=atoi(arguments[1].c_str());

  DAQServerInterface * p_servInt = NULL;
  // the Server interface that produces SOC simulated data
  p_servInt = new SOCSimuServerInterface(verbose);
  
  // main server object
  TDAQServerBase  tdaqServer(arguments, p_servInt);

  // start thread for sharing slow info: configure and monitoring
  cout << "Connecting the slow thread..." << endl << endl; 
  tdaqServer.runServer(); //actual running of the server; no return till end...

  return 0;

}
