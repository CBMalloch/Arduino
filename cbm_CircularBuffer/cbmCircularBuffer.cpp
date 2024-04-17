/*

// see the .h file for why we've disabled everything in this file...




#include <cbmCircularBuffer.h>

template <typename T>
cbmCircularBuffer<T>::cbmCircularBuffer () {
}

template <typename T>
cbmCircularBuffer<T>::cbmCircularBuffer ( size_t bufSize ) {
  cbmCircularBuffer::_init ( bufSize );
}

template <typename T>
cbmCircularBuffer<T>::~cbmCircularBuffer () {
  free ( _array );
}

template <typename T>
void cbmCircularBuffer<T>::reset () {
  // left points to the first occupied spot
  // right points to the last occupied spot
  _left = 0;
  _right = -1;
  // _n is total values added, including those overwritten
  _n = 0UL;
}

template <typename T>
bool cbmCircularBuffer<T>::_init ( size_t bufSize ) {
  bool OK = true;
  _array = (T *) malloc ( bufSize * sizeof ( T ) );
  if (_array == NULL) {
    _size = 0;
    // Serial.printf ( "cbmCircularBuffer: Memory not allocated.\n" );
    OK = false;
  } else {
    _size = bufSize;
    cbmCircularBuffer::reset ();
  }    
  return OK;
}

template <typename T>
unsigned long cbmCircularBuffer<T>::bufSize () {
  return _size;
}

template <typename T>
unsigned long cbmCircularBuffer<T>::count () {
  return _n;
}

template <typename T>
unsigned long cbmCircularBuffer<T>::firstIndex () {
  return _n <= _size ? 0 : _n - _size;
}

template <typename T>
unsigned long cbmCircularBuffer<T>::store ( T x, bool overwrite ) {
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
    Serial.printf ( "cbmCircularBuffer: must not add to full buffer\n" );
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
void cbmCircularBuffer<T>::entries ( T out[] ) {
  size_t ptr = _left;
  for ( int i = 0; i < _size; i++ ) {
    out [ i ] = ( i < _n ) ? _array [ ptr ] : 0;
    ptr++;
    if ( ptr >= _size ) ptr = 0;
  }
}

*/
