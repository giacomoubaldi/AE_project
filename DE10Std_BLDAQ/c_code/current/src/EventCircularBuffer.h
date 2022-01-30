//
//         Event Circular Buffer
// (built on top of a generic circular_buffer)
//  push and pop (write and read) are thread safe
//  A minimalistic interface is used: you cannot do everything! 
//
#ifndef EVENTCIRCULARBUFFER
#define EVENTCIRCULARBUFFER

#include "circular.h"
#include <pthread.h>
#include <vector>


class EventCircularBuffer {

public:

  EventCircularBuffer(int capacity) : cb(capacity){
	pthread_mutex_init(&lock, NULL);
  };

  ~EventCircularBuffer(){
          // what to put here??
  }
  
  // pop_front() returns the first element and 
  // removes it from the buffer
  uint32_t pop_front() {
    pthread_mutex_lock(&lock);	  
    uint32_t rc = cb.front();
    cb.pop_front();
    pthread_mutex_unlock(&lock);
    return rc;	
  };
  
  void push_back(uint32_t value){
    pthread_mutex_lock(&lock);	  
    cb.push_back(value);
    pthread_mutex_unlock(&lock);	  	  
  };
  
  // thread safe version of the insertion of a vector in the buffer
  // Ensure you have enough space in the buffer.
  void write(std::vector<uint32_t> & values){
    pthread_mutex_lock(&lock);	  
    for(std::vector<uint32_t>::iterator it=values.begin(); it!=values.end(); ++it){
      cb.push_back(*it);
    }
    pthread_mutex_unlock(&lock);	  	  
  }
  
  // thread safe version of the reading of N data from the buffer
  // removes N data from it. You have to be sure there are N data in the buffer
  void read(std::vector<uint32_t> & dest, unsigned int nvalues){
    pthread_mutex_lock(&lock);	  
    for(unsigned int i=0; i<nvalues; ++i){
      dest.push_back(cb.front());
      cb.pop_front();
    }
    pthread_mutex_unlock(&lock);	  	  
  }
  
  // quick access functions 
  bool empty() const { return cb.empty();}
  unsigned int size() const { return cb.size();}
  unsigned int capacity() const { return cb.capacity();}

private:

  // the buffer: made of 32 bit unsigned ints
  circular_buffer<uint32_t> cb;

  pthread_mutex_t lock;
};

#endif
