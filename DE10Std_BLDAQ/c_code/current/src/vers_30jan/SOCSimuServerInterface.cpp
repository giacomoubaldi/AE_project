
#include "SOCSimuServerInterface.hh"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */
#include <iostream>

SOCSimuServerInterface::SOCSimuServerInterface(int verb) : m_verbose(verb){
  m_isRunning=false;
  m_nextEventNumber = 0;
  srand( (unsigned int) time(NULL) );
};

SOCSimuServerInterface::~SOCSimuServerInterface(){
};

int SOCSimuServerInterface::initialize(){
  // here the FPGA FSM is in the CONFIG status    
  return 0; // OK
};

int SOCSimuServerInterface::shutdown(){
  return 0;
}

void SOCSimuServerInterface::configure(std::vector<uint32_t> & param){
};

void SOCSimuServerInterface::publish(std::vector<uint32_t> & param,
       std::vector<uint32_t> & results){
	results = param; // just for the moment
}

void SOCSimuServerInterface::GoToRun(){
  m_nextEventNumber = 0;
  m_isRunning=true;
  generateEvents();
}

void SOCSimuServerInterface::StopDC(){
  m_isRunning=false;
  m_verbose = false;
};  


uint32_t SOCSimuServerInterface::bytesAvailable(){
  uint32_t rc;
  if( m_isRunning ){
    rc = events.size(); // something greater than zero
  } else{
    rc = 0; // no data available  
  }
  return rc;
}

void SOCSimuServerInterface::readData(std::vector<uint32_t>& evt, int /*maxValues*/){ // ?? maxValues not used
  // if ( m_verbose )  std::cout << "SOCSimuServerInterface::readData copying " << events.size() << " bytes " << std::endl;
  evt = events;
  events.erase(events.begin(),events.end());

  generateEvents(); // produce the next bunch
}


//  private member functions
void SOCSimuServerInterface::generateEvents(){

  unsigned int totalEvents =  m_nextEventNumber ;
  unsigned int nEvents = 0;
  //  std::vector<uint32_t> evt;

  // generating a random number of events from 0 to 10
  while ( nEvents < 1 ){ 
    nEvents = rand() %10 +1;
    if ( m_verbose && (nEvents%100)==0 )  std::cout << "SOCSimuServerInterface::generateEvents  nEvents = " << nEvents << std::endl;
  }

  // creating a train of events
  for ( unsigned int i = 0; i<=nEvents; i++ ) {
    
    //if ( m_verbose )  std::cout << "SOCSimuServerInterface::generateEvents  event number = " << i << std::endl;

    // generateEvents( totalEvents, evt );
     uint32_t nWords = (uint32_t)(rand() %25 +5);
    //if ( m_verbose )  std::cout << "SOCSimuServerInterface::generateEvents  nWords = " << nWords << std::endl;

    //if ( m_verbose )  std::cout << "SOCSimuServerInterface::generateEvents  events size = " << events.size() << std::endl;
            
    // filling randomly the event
    
    // Event header
    events.push_back(nWords);
    events.push_back( 0xeadebaba);
    events.push_back( (uint32_t)totalEvents ) ;

    // Event data
    for ( unsigned int j = 0; j<nWords-5; j++ ) {   
      uint32_t value = 
	0xaa000000 | ((totalEvents% 256)<<16) | (uint32_t)(rand() %20 +1); 
      events.push_back( value );
    }
    
    // Event trailer
    events.push_back(0xfafefafe);
    events.push_back(0xbacca000);

    if ( m_verbose ){
      unsigned int base = events.size()-nWords;
      for ( unsigned int j = 0; j<nWords; j++ ) {  
    //     std::cout << "SOCSimuServerInterface::generateEvents  events.at(" 
		  // << base+j << ") = " << (std::hex)<<events.at(base+j) 
		  // << std::endl;
      }
    }

    // if ( m_verbose )  std::cout << "SOCSimuServerInterface::generateEvents   events.size() = " << events.size()*4 << std::endl;
        
    totalEvents++;
  } 
  m_nextEventNumber = totalEvents;
}

