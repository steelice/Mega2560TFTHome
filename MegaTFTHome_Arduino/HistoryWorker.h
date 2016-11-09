// header defining the interface of the source.
#ifndef _HISTORYWORKER_H_
#define _HISTORYWORKER_H_

// include Arduino basic header.
#include <Arduino.h>


template<typename T, unsigned int SIZE>
class HistoryWorker {
  public:
    // init the stack (constructor).
    HistoryWorker ();

    // clear the stack (destructor).
    ~HistoryWorker ();

    // push an item to the stack.
    void push (const T i);

    bool isFull() const;

    T reset();
    T get() const;
    T each();

    void clear();

    T getMax() const;
    T getMin() const;

    unsigned int size() const;
    unsigned int getWritePosition() const;
    unsigned int getReadPosition() const;


  private:

  	T data[SIZE];

  	unsigned int filledSize = 0;
    unsigned int readPosition = 0;
    unsigned int writePosition = 0;
    bool eachFullCycle = false;

    T empty;
};


// init the stack (constructor).
template<typename T, unsigned int SIZE>
HistoryWorker<T, SIZE>::HistoryWorker () {
  empty = {0, 0};
  clear();
}

// clear the stack (destructor).
template<typename T, unsigned int SIZE>
HistoryWorker<T, SIZE>::~HistoryWorker () {
  readPosition = writePosition = 0;
}


template<typename T, unsigned int SIZE>
void HistoryWorker<T, SIZE>::push(const T i){
	data[writePosition] = i;

  writePosition++;
  if(writePosition >= SIZE) writePosition = 0;
  if(filledSize < SIZE) filledSize++;
}

template<typename T, unsigned int SIZE>
bool HistoryWorker<T, SIZE>::isFull() const{
  return filledSize >= SIZE;
}

template<typename T, unsigned int SIZE>
T HistoryWorker<T, SIZE>::reset(){
  eachFullCycle = false;
  if(filledSize == 0){
    readPosition = 0;
  }else{
    // if(isFull()){
    //   readPosition = (writePosition == SIZE - 1) ? 0 : writePosition + 1;
    // }else{
    //   readPosition = 0;
    // }
    readPosition = isFull() ? writePosition : 0;
  }

  return get();
}

template<typename T, unsigned int SIZE>
T HistoryWorker<T, SIZE>::get() const{
  if(filledSize == 0) return empty;
  return data[readPosition];
}

template<typename T, unsigned int SIZE>
T HistoryWorker<T, SIZE>::each(){

  if(eachFullCycle){ // мы прошли весь массив
    reset();
    return empty;
  }
	if(filledSize == 0) return empty; // массив ещё пустой
  T cur = get();
  readPosition++;
  
  if((readPosition >= filledSize) && ((writePosition == 0) || !isFull()))
    eachFullCycle = true;
  else if(readPosition == writePosition)
    eachFullCycle = true;
  else if(readPosition >= SIZE)
    readPosition = 0;

  return cur;
}

template<typename T, unsigned int SIZE>
void HistoryWorker<T, SIZE>::clear() {
  readPosition = writePosition = filledSize = 0;
  for(int i = 0; i < SIZE; i++) data[i] = empty;
}

template<typename T, unsigned int SIZE>
unsigned int HistoryWorker<T, SIZE>::size() const{
  return filledSize;
}

template<typename T, unsigned int SIZE>
unsigned int HistoryWorker<T, SIZE>::getWritePosition() const{
  return writePosition;
}

template<typename T, unsigned int SIZE>
unsigned int HistoryWorker<T, SIZE>::getReadPosition() const{
  return readPosition;
}

template<typename T, unsigned int SIZE>
T HistoryWorker<T, SIZE>::getMax() const {
  if(!filledSize) return empty;
  T m = reset();
  T n = get();
  while(n.time != 0) {
    if(m.value < n.value) m = n;
    n = each();
  }

  return m;
}

template<typename T, unsigned int SIZE>
T HistoryWorker<T, SIZE>::getMin() const {
  if(!filledSize) return empty;
  T m = reset();
  m = {0, 1000};

  T n = get();
  while(n.time != 0) {
    if(m.value > n.value) m = n;
    n = each();
  }

  return m;
}


#endif // _HISTORYWORKER_H_