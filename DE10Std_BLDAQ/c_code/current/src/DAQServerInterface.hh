#ifndef DAQSERVINT_HH
#define DAQSERVINT_HH

/* This class provides the basic methods to be interfaced 
   with a remote DAQ controlling system, that asks for transitions
*/
// include this for uint32_t on ARM
// #include "hwlib.h"       commented for simulation-only test
#include <stdint.h>      // added for simulation-only test
#include <vector>

class DAQServerInterface {
public:
  DAQServerInterface(){};
  virtual ~DAQServerInterface();

  virtual int initialize(){ return 0;};
  virtual int shutdown(){return 0;};

  virtual void configure(std::vector<uint32_t> & param){};

  virtual void GoToRun(){};
  virtual void StopDC(){};
  virtual void publish(std::vector<uint32_t> & param,
	       std::vector<uint32_t> & results){};

  virtual uint32_t bytesAvailable(){ return 0;};
  virtual void readData(std::vector<uint32_t> & evt, int maxValues=10000){};
  
private:

};

#endif
