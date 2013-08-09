 /*
	RS485.cpp is a library for Arduino which facilitates communications using the RS485 protocol
  implemented on top of either the built-in Arduino Serial object or the SoftwareSerial library 
  included with the Arduino software IDE distribution.
    
	Copyright (c) 2013 Charles B. Malloch, PhD
	Arduino RS485 is free software: you can redistribute it and/or modify it under the terms of the 
	GNU General Public License as published by the Free Software Foundation, either version 3 of the License, 
	or (at your option) any later version.

	Arduino RS485 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
	See the GNU General Public License for more details.
  
	To get a copy of the GNU General Public License see <http://www.gnu.org/licenses/>.
  
  Written by Charles B. Malloch, PhD  2013-02-20
 
*/

#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#endif

#include <RS485.h>

// #define TESTMODE 1
#undef TESTMODE


RS485::RS485() {
}

RS485::RS485(
              Stream *port,
              short pdTalkEnable
            ) {
	Init ( port, pdTalkEnable );
}

void RS485::Init(
                  Stream *port,
                  short pdTalkEnable
                ) {
                
  _port = port;
  // what pin do we use to enable the RS485 send? -1 means don't use any pin
  _pdTalkEnable = pdTalkEnable;
  /*
    // How long should an unprocessed message live ?
  _message_TTL = 10;
  */
  
}

void RS485::Send ( unsigned char * buf, short nChars ) {
  if ( _pdTalkEnable >= 0 ) digitalWrite ( _pdTalkEnable, HIGH );
  for ( int i = 0; i < nChars; i++ ) {
    _port->write ( buf[i] );
  }
  _port->flush();
  if ( _pdTalkEnable >= 0 ) digitalWrite ( _pdTalkEnable, LOW );
  return;
}

int RS485::Receive ( unsigned char * buf, unsigned int bufLen, long receive_timeout_ms ) {

  // returns the number of characters received

  static unsigned long lastCharReceivedAt_ms;
  unsigned int bufPtr = 0;
  
  errorFlag = 0;
  
  // capture characters from stream
  lastCharReceivedAt_ms = millis();
  while ( ( receive_timeout_ms < 0 ) || ( millis() - lastCharReceivedAt_ms ) < receive_timeout_ms ) {
    
    if ( _port->available() ) {
    
      if ( bufPtr < bufLen ) {
        // grab the available character
        buf [ bufPtr++ ] = _port->read();
        lastCharReceivedAt_ms = millis();
      } else {
        // error - buffer overrun
        errorFlag = 1;
        return ( bufPtr );
      }
    }  // character available
  }
  
  /*
  if ( ( ( millis() - lastCharReceivedAt_ms ) > _message_TTL_ms ) && bufPtr485 != 0 ) {
    // kill old messages to keep from blocking up with a bad message piece
    #ifdef TESTMODE
      _port->println ( "timeout" );
    #endif
    receive_status = -81;  // timeout
    bufPtr = 0;
  }
  */

  return ( bufPtr );
}
