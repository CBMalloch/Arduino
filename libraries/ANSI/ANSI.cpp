#include <Arduino.h>
#include <ANSI.h>

void clr2EOL () {
  // 0 clears to end of line; 1 clears to beginning of line; 2 clears whole line
  Serial.print ( "\x1b[0K" );
}

void cls () {
  // 0 clears down to end of screen; 1 clears up to top of screen; 2 clears whole screen
  Serial.print ( "\x1b[2J" );
}

void curpos ( int row, int col ) {
  Serial.print ( "\x1b[" );
  Serial.print ( row );
  Serial.print ( ";" );
  Serial.print ( col );
  Serial.print ( "H" );
}

const char *version()
{
	return "1.000.000";
}