#ifndef SOCSERVINT_HH
#define SOCSERVINT_HH


#include "DAQServerInterface.hh"
#include "FpgaInterface.h"


class SOCServerInterface : public DAQServerInterface {
public:
  SOCServerInterface();
  virtual ~SOCServerInterface();

  virtual int initialize();
  virtual int shutdown();
  virtual void configure(std::vector<uint32_t> & param);  
  virtual void publish(std::vector<uint32_t> & param,
	       std::vector<uint32_t> & results);		   
  virtual void GoToRun();
  virtual void StopDC();
  virtual uint32_t bytesAvailable();
  virtual void readData(std::vector<uint32_t> & evt, int maxValues=10000);

private:

  void sendGoTo(int stat);
  void sendWriteReg(uint32_t reg, uint32_t value);

  // main attribute of the class
  FpgaInterface fpga;

  unsigned int m_nextEventNumber;
};

#endif
