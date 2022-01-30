#ifndef SOCSERVINT_HH
#define SOCSERVINT_HH

/*
  This class provides the interface between the MainTDAQServer and the 
  low level calls to the FPGA Interface.
  
*/

#include "DAQServerInterface.hh"
#include "FpgaInterface.h"

class SOCServerInterface : public DAQServerInterface {
public:
  SOCServerInterface(unsigned int verbose=0);
  virtual ~SOCServerInterface();

  virtual int initialize();
  virtual int shutdown();
  virtual void configure(std::vector<uint32_t> & param);  
  virtual void publish(std::vector<uint32_t> & param,
	       std::vector<uint32_t> & results);
  virtual void GoToRun(uint32_t /*runNumber*/, std::vector<uint32_t> & /*param*/);
  virtual void StopDC();
  virtual uint32_t wordsAvailable();
  virtual void readData(std::vector<uint32_t> & evt, int maxValues=10000);

private:

  void sendGoTo(int stat);
  void sendWriteReg(uint32_t reg, uint32_t value);

  // main attribute of the class
  FpgaInterface fpga;

  unsigned int m_nextEventNumber;
  unsigned m_verbose;
};

#endif
