/*
	cbmCircularBuffer.h - library for maintaining a circular buffer
	Created by Charles B. Malloch, PhD, April 13, 2024
	Released into the public domain
*/

#ifndef CircularBuffer_h
#define CircularBuffer_h

// #warning GETTING .h file

#define CircularBuffer_VERSION "1.001.001"
// 2023-05-30 1.000.000 created

#include <stdlib.h>
#include "Arduino.h"

template <typename T>
class CircularBuffer
{
  public:
    CircularBuffer();
    CircularBuffer ( size_t bufSize );
    ~CircularBuffer();
    unsigned long count ();   // returns total number of points added, including overwritten
    unsigned long bufSize ();
    unsigned long firstIndex ();
    // overwrite is for future use when we have implemented unshift, shift, push, and pop
    unsigned long store ( T x, bool overwrite = true );
    void reset ();
    void entries ( T out[] );
  protected:
    // anything that needs to be available only to:
    //    the class itself
    //    friend functions and classes
    //    inheritors
    // this-> reportedly not needed
  
  private:
    size_t _size;
    size_t _left, _right;
    unsigned long _n;                         // total entries, including overwritten
    T * _array;
    bool _init ( size_t bufSize );
  
};

//******************************************************************************
/*
  Note: putting the function defs into the .cpp file as usual doesn't work
  
  The reason is that a .cpp file is separately compiled, and it makes the 
  compiler unhappy to try to compile a templated function before knowing
  what the eventual type will be.
  
  Thus we've included the functions here where they'll be included in the source,
  and removed them from the .cpp file.
  
  See <https://stackoverflow.com/questions/36039/templates-spread-across-multiple-files>
*/
//******************************************************************************


// #include <CircularBuffer.h>

template <typename T>
CircularBuffer<T>::CircularBuffer () {
}

template <typename T>
CircularBuffer<T>::CircularBuffer ( size_t bufSize ) {
  CircularBuffer::_init ( bufSize );
}

template <typename T>
CircularBuffer<T>::~CircularBuffer () {
  free ( _array );
}

template <typename T>
void CircularBuffer<T>::reset () {
  /*
    left points to the first occupied spot
    right points to the last occupied spot
  */
  _left = 0;
  _right = -1;
  // _n is total values added, including those overwritten
  _n = 0UL;
}

template <typename T>
bool CircularBuffer<T>::_init ( size_t bufSize ) {
  bool OK = true;
  _array = (T *) malloc ( bufSize * sizeof ( T ) );
  if (_array == NULL) {
    _size = 0;
    // Serial.printf ( "CircularBuffer: Memory not allocated.\n" );
    OK = false;
  } else {
    _size = bufSize;
    CircularBuffer::reset ();
  }    
  return OK;
}

template <typename T>
unsigned long CircularBuffer<T>::bufSize () {
  return _size;
}

template <typename T>
unsigned long CircularBuffer<T>::count () {
  return _n;
}

template <typename T>
unsigned long CircularBuffer<T>::firstIndex () {
  return _n <= _size ? 0 : _n - _size;
}

template <typename T>
unsigned long CircularBuffer<T>::store ( T x, bool overwrite ) {
  if ( ( _n >= _size ) && overwrite ) {
    // full but OK to overwrite
    // bump right
    // Serial.printf ( "Overwriting: " );
    _right++;
    if ( _right >= _size ) _right -= _size;
    // bump left
    _left++;
    if ( _left >= _size ) _left -= _size;
    // leave n alone
    // store the item
    _array [ _right ] = x;
    _n++;
    // Serial.printf ( "n: %2d; L: %2d; R: %2d; val: %2.2f =? %2.2f\n", _n, _left, _right, _array [ _right ], x );
  } else if ( _n >= _size ) {
    Serial.printf ( "CircularBuffer: must not add to full buffer\n" );
  } else {
    // not full
    // Serial.printf ( "Storing: " );
    // bump right
    _right++;
    if ( _right >= _size ) _right -= _size;
    // store the item
    _array [ _right ] = x;
    _n++;
    // Serial.printf ( "n: %2d; L: %2d; R: %2d; val: %2.2f =? %2.2f\n", _n, _left, _right, _array [ _right ], x );
  }
  return _n;
}

template <typename T>
void CircularBuffer<T>::entries ( T out[] ) {
  size_t ptr = _left;
  for ( int i = 0; i < _size; i++ ) {
    out [ i ] = ( i < _n ) ? _array [ ptr ] : 0;
    ptr++;
    if ( ptr >= _size ) ptr = 0;
  }
}



#endif