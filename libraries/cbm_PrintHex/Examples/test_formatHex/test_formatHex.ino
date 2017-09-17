#define BAUDRATE 115200

void formatHex ( unsigned char * x, char * buf, unsigned char len = 2 );

void setup () {
  short s;
  int i;
  float f;
  long l;
  char buf[11];
  
  Serial.begin ( BAUDRATE );
  
  i = 2; formatHex ( ( unsigned char * ) &i, buf, 2 );
  Serial.print ( i ); Serial.print ( " -> " ); Serial.println ( buf );
  
  i = 3; formatHex ( ( unsigned char * ) &i, buf );
  Serial.print ( i ); Serial.print ( " -> " ); Serial.println ( buf );
  
  s = 2; formatHex ( ( unsigned char * ) &s, buf, 1 );
  Serial.print ( s ); Serial.print ( " -> " ); Serial.println ( buf );
  
  l = 2; formatHex ( ( unsigned char * ) &l, buf, 4 );
  Serial.print ( l ); Serial.print ( " -> " ); Serial.println ( buf );
  
  f = 2.0; formatHex ( ( unsigned char * ) &f, buf, 4 );
  Serial.print ( f ); Serial.print ( " -> " ); Serial.println ( buf );
  
  f = 3.0; formatHex ( ( unsigned char * ) &f, buf, 4 );
  Serial.print ( f ); Serial.print ( " -> " ); Serial.println ( buf );
}

void loop () { }

void formatHex ( unsigned char * x, char * buf, unsigned char len ) {
  // len is in bytes
  // buf must be at least len * 2 + 3 characters long
  char h[] = {'0', '1', '2', '3', '4', '5', '6', '7',
              '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  buf[0] = '0'; buf[1] = 'x';
  for ( int i = 0; i < len; i++ ) {
    unsigned short theByte, nybbleIdx;
    memcpy ( &theByte, x + ( len - i - 1 ), 1 );
    buf [ i * 2 + 2 ] = h [ ( theByte >> 4 ) & 0x000f ];
    buf [ i * 2 + 3 ] = h [ ( theByte >> 0 ) & 0x000f ];
    buf [ i * 2 + 4 ] = '\0';
  }
}

