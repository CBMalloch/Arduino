#ifndef FFT_h
#define FFT_h

#define FFT_VERSION "1.000.000"

#include <Arduino.h>

/*

Here is the header of the file from which I extracted this:

 ***********    ARDUINO VOICE RECOGNITION    ***********
 *                 
 * January 24, 2012
 *
 * Anatoly Kuzmenko ( k_anatoly@hotmail.com )
 * http://arduinovoicerecognition.blogspot.com
 *
 * SOFTWARE COMPILES USING Arduino 0022 IDE (Tested on Linux OS only).
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *
 * Copyright (C) 2012 Anatoly Kuzmenko.
 * All Rights Reserved.
 ********************************************************************

  !!! INCLUDE TO WIRING.H (now Arduino.h)  TO BE COMPILED WITH ARDUINO IDE
    C:\Program Files\arduino-1.0.1\hardware\arduino\cores\arduino\Arduino.h

typedef union _CMPLX {
  int16_t intl;
  int8_t  hb[2];
} CMPLX;

Here are my additional notes:

	NOTE from cbm: will load, but not run, on ATmega 168
 
*/










/*

in .h and .cpp:

using namespace FFT;

in main:

FFT::foo = 9;

*/

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
//#pragma GCC optimize (always_inline)

#define FFT_VERSION "1.002.000"

// 64 : 4 kHz = 16 msec. time sampling period.
#define FFT_SIZE   	64

// Quantity of Time Frames (64/4kHz = 16 msec)
#define TIME_SZ         64
// Quantity of Bands on Spectrogram
#define NBR_FRQ         16

extern uint8_t FFT_record_to_EEPROM;       // Switch "RECORD" to EEPROM
extern uint8_t FFT_print_ADC;              // Switch "Print ADC"
extern uint8_t FFT_print_FFT;              // Switch "Print FFT"
extern uint8_t FFT_print_prefiltered;      // Switch "Print Pre-Filter"
extern uint8_t FFT_print_filtered;         // Switch "Print Post-Filter"
extern uint8_t FFT_play_from_EEPROM;       // Switch "PLAY" from EEPROM

void FFT ( CMPLX * fr, int16_t fft_n );
int freeRam ();
void FFTsetup();
void FFTloop();
void  FFT_print_spg1();
void  FFT_print_spg2();

#endif
