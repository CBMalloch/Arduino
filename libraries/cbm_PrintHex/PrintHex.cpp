#include <string.h>
#include <PrintHex.h>

char printHex_buf [ 9 ] = "";

void convertHex ( char* destBuf, int lenDestBuf, unsigned long value, int nBytes ) {
  destBuf [ nBytes + 0 ] = '\0';
  int v;
  if ( nBytes > lenDestBuf ) {
    // error! buffer overrun possibility
    strncpy ( destBuf, "ERROR", lenDestBuf );
    destBuf [ lenDestBuf ] = '\0';
    return;
  }
  for ( int i = nBytes - 1; i >= 0; i-- ) {
    v = value & 0x0000000f;
    value = value >> 4;
    destBuf [ i ] = "0123456789abcdef" [ v ];
  }
  if ( ! ( value == 0 || ( ( value == 0xffffffff >> ( nBytes * 4 ) ) && ( v & 0x8 ) ) ) ) {
    // value was too big to translate
    #ifdef PRINTHEX_VERBOSE
      out.print ( "right-shifted: 0x" ); out.println ( 0xffffffff >> ( nBytes * 4 ), HEX );
      out.print ( "v & 0x08: 0x" ); out.println ( v & 0x08, HEX );
    #endif
    strncpy ( destBuf, "********", nBytes );
    destBuf [ nBytes ] = '\0';
  }
  return;
}

void printHex ( unsigned long value, int nBytes, Stream& out ) {
  convertHex ( printHex_buf, 8, value, nBytes );
  // printer->print ( "0x" ); printer->print ( printHex_buf );
  out.print ( "0x" ); out.print ( printHex_buf );
}

void hexDump ( byte * theBuf, int theLen, int lineLen, Stream& out ) {
  const int pBufLen = 8;
  char pBuf [ pBufLen ];
  int pos = 0;  // pos is number of bytes of data printed, not line position...
  for ( int i = 0; i < theLen; i++ ) {
    if ( pos == 0 ) {
      out.print ( "  0x" );
      convertHex ( pBuf, pBufLen, ( unsigned long ) i, 4 );
      out.print ( pBuf );
      out.print ( ": " );
    }
    
    convertHex ( pBuf, pBufLen, theBuf [ i ], 2 );
    out.print ( pBuf ); 
    pos++;
    
    if ( ! ( ( i + 1 ) % 2 ) ) {
      // space between groups
      out.print ( " " );
    }
    
    if ( ! ( ( i + 1 ) % 16 ) ) {
      // extra space between blocks
      out.print ( " " );
    }
    
    if ( pos >= lineLen ) {
      out.println ();
      pos = 0;
    }
  }
}

void serialHexDump ( byte theByte, int lineLen, Stream& out ) {
  const int pBufLen = 2;
  char pBuf [ pBufLen ];
  
  static int pos = 0;
  
  if ( pos == 0 ) {
    out.print ( "  --: " );
  }
  
  convertHex ( pBuf, pBufLen, theByte, 2 );
  out.print ( pBuf ); 
  pos++;
  
  if ( ! ( pos % 2 ) ) {
    // space between groups
    out.print ( " " );
  }
  
  if ( ! ( pos % 16 ) ) {
    // extra space between blocks
    out.print ( " " );
  }
  
  if ( pos >= lineLen ) {
    out.println ();
    pos = 0;
  }
}