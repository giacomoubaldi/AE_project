#ifndef SOCSIMUSERVINT_HH
#define SOCSIMUSERVINT_HH

#include "DAQServerInterface.hh"

class SOCSimuServerInterface : public DAQServerInterface {
public:
  SOCSimuServerInterface(int verb);
  virtual ~SOCSimuServerInterface();

  virtual int initialize();
  virtual int shutdown();
  virtual void configure(std::vector<uint32_t> & param);  
  virtual void publish(std::vector<uint32_t> & param,
	       std::vector<uint32_t> & results);		   
  virtual void GoToRun();
  virtual void StopDC();
  virtual uint32_t bytesAvailable();
  virtual void readData(std::vector<uint32_t>& evt, int maxValues=10000);


private:

  void generateEvents();
  bool m_isRunning;
  std::vector<uint32_t> events;
  int m_verbose;
  unsigned int m_nextEventNumber;
};

#endif
