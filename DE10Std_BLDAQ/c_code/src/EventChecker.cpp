/*****************************/
/*                           */
/* Date: July 2020           */
/* Author: M. Villa          */
/*****************************/

/* Check N events and possibly write main info on a text file
*/

#include "EventChecker.hh"
#include "FpgaInterface.h" 
#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

EventChecker::EventChecker(){
  m_runnumber = 0;
  isOpened = false;
  m_lastevt = 0xffffffff; 
  m_errors = 0;
  m_checked = 0;
}

EventChecker::~EventChecker(){
  if( isOpened )
     stopRun(); 
}

void EventChecker::startRun(std::string module, uint32_t runnumber, bool doWrite){
  m_runnumber = runnumber;
  if( isOpened )
     stopRun(); 
  isOpened = false;
  m_lastevt = 0xffffffff; 
  m_errors = 0;
  m_checked = 0;
  if( doWrite ){
    char filename[200];
    snprintf(filename, 199,"EventInfo_%s_%08d.txt", module.c_str(),runnumber);
    ofs.open(filename, std::ofstream::out );
	ofs<<std::endl<<"Event info for module "<<module<<" in run "<<runnumber<<std::endl<<std::endl;
	ofs<<"Seq EVT   HWNumber   BCO      DX          DBCO (us)     DBX (us)"<<std::endl;
    isOpened = true;
  }
}

void EventChecker::stopRun(){
  if( isOpened ){
     ofs.close(); 
	ofs<<std::endl<<"Total number of errors: "<<m_errors<<std::endl;
  }
  isOpened = false;
}

bool EventChecker::checkNEvents(std::vector<uint32_t> &events){
  bool rc = true;
  if( events.size()<8 ){
	 std::cout<<"Error on group size: "<<events.size()<<std::endl;
	 if( isOpened ) ofs<<"Error on group size: "<<events.size()<<std::endl;
	 m_errors++;
  } else {
    if( events.size()!=events[0] ){
	  std::cout<<"Error on group size!= evt[0]: "<<events.size()<<" .ne. "<<events[0]<<std::endl;
	  if( isOpened ) ofs<<"Error on group size != evt[0]: "<<events.size()<<" .ne. "<<events[0]<<std::endl;
	  m_errors++;
	}
	std::vector<uint32_t>::iterator begin, end;
	begin = events.begin()+1;
	uint32_t siz;
    uint32_t tsize=1;
    do {
	  siz = *begin;
	  tsize += siz;
  	  if( siz<7 || siz>20000 || tsize>events.size()){
	    std::cout<<"Error on event size:"<<siz<<std::endl;
	    if( isOpened ) ofs<<"Error on event size:"<<siz<<std::endl;
	    m_errors++;
        tsize = events.size(); // be ready to exit the loop!
	  } else {
	    end = begin+siz;
		rc = rc && checkEvent(begin, end);
		begin=end;
	  }	  
	} while (tsize<events.size());
  }
  if( !rc ) dumpEvent(events);
  return rc;
}

bool EventChecker::checkEvent(std::vector<uint32_t>::iterator begin, std::vector<uint32_t>::iterator end){
	
  std::stringstream  s;
  bool checkPassed = true;
  // check length, header, footer
  uint32_t siz=*begin;
  if( siz<7 || siz>20000 ){
    s<<" Size error:"<<siz<<" ";
	checkPassed = false;
  }
  uint32_t  head = *(begin+1);
  if( head!=Header_event ){
    s<<" Wrong header:"<<(std::hex)<<head<<" "<<(std::dec);
	m_errors++;
	checkPassed = false;
  }
  uint32_t  foot = *(begin+siz-1);
  if( foot!=Footer_event ){
    s<<" Wrong footer:"<<(std::hex)<<foot<<" "<<(std::dec);
    m_errors++;
	checkPassed = false;
  }
  uint32_t  evnum = *(begin+3);
  uint32_t  bco   = *(begin+4);
  uint32_t  bx    = *(begin+5);

  if( evnum!= m_checked ){
    // error on sequence numbering
	s<<" DAQSEQUENCE ";
    m_errors++;
	checkPassed = false;
  }
  if( m_lastevt != 0xffffffff ){
    if( evnum!=m_lastevt+1 ){
	  s<<" TRIGSEQUENCE ";	
      m_errors++;
	  checkPassed = false;
	}
  } else {
	m_lastbco = bco;
	m_lastbx = bx;
  }
  int64_t dbco = ((int64_t)bco)-((int64_t) m_lastbco);
  if( bco<m_lastbco ) dbco += 0x100000000;
  int64_t dbx = ((int64_t)bx)-((int64_t) m_lastbx);
  if( bx<m_lastbx ) dbx += 0x100000000;
  dbx /=50;
  stringstream line;
  line<<std::setw(8)<<m_checked<<std::setw(9)<<evnum<<std::setw(11)<<bco<<std::setw(11)<<bx<<std::setw(11)<<dbco<<std::setw(11)<<dbx<<" "<<s.str();
  if( isOpened ) ofs<<line.str()<<std::endl;
  if( !checkPassed ) std::cout<<line.str()<<std::endl;

  m_lastevt = evnum;
  m_lastbco = bco;
  m_lastbx = bx;
  m_checked++;
  return checkPassed;
}

// check event data
void EventChecker::dumpEvent(std::vector<uint32_t> & evt){
  if( evt.size()>3){
    std::cout<<"Event # "<<evt[3]<<",    size: "<<evt[0]<<std::endl;
    for(unsigned int i=0; i<evt.size(); ++i){
	  std::cout<<" "<<std::setw(9)<<(std::hex)<<" "<<evt[i]<<std::endl;
    }
    std::cout<<(std::dec)<<std::endl; 
  } else {
	std::cout<<"Event size too low: "<<evt.size()<<std::endl;
  }
}
