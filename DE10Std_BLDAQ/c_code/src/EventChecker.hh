//
//  Upper level class to deal with events
//
#ifndef EVENTCHECKER
#define EVENTCHECKER

#include <vector>
#include <fstream>
#include <string>


class EventChecker {

public:

  EventChecker();
  ~EventChecker();
  
  // Open a text file
  void startRun(std::string module,uint32_t runnumber, bool doWrite);

  // Write an entry
  void writeEventInfo(std::vector<uint32_t>::iterator begin, std::vector<uint32_t>::iterator end);
	
  // check a single event
  bool checkEvent(std::vector<uint32_t>::iterator begin, std::vector<uint32_t>::iterator end);

  // check N events	
  bool checkNEvents(std::vector<uint32_t> &events);

  // close files
  void stopRun();
  
  // dump vector
  void dumpEvent(std::vector<uint32_t> & evt);

	
private:

  std::ofstream ofs;
  bool isOpened;
  uint32_t m_runnumber;
  uint32_t m_errors;
  // check N events
  uint32_t m_lastbco;
  uint32_t m_lastevt;
  uint32_t m_lastbx;
  uint32_t m_checked;
};

#endif
