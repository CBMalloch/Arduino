#include <string.h>
#include <PrintHex.h>

void convertHex ( char* destBuf, int lenDestBuf, unsigned long value, int nNybbles, Stream& out ) {
  /*
    Converts nNybbles ( up to 8 ) half-bytes of a single unsigned long value into hex
    Works from the right, but doesn't change endian-ness
  */
  
  int v;
    
  // sanitize
  if ( nNybbles > lenDestBuf ) {
    // error! buffer overrun possibility
    strncpy ( destBuf, "ERROR", lenDestBuf );
    destBuf [ lenDestBuf ] = '\0';
    return;
  }
  
  // convert into output buffer
  for ( int i = nNybbles - 1; i >= 0; i-- ) {
    v = value & 0x0000000f;
    value = value >> 4;
    destBuf [ i ] = "0123456789abcdef" [ v ];
    // out.print ( "->" ); out.print ( destBuf [ i ] ); out.print ( "\n" );
  }
  destBuf [ nNybbles + 0 ] = '\0';
  
  if ( ! ( value == 0 || ( ( value == 0xffffffff >> ( nNybbles * 4 ) ) && ( v & 0x8 ) ) ) ) {
    // value was too big to translate
    #ifdef PRINTHEX_VERBOSE
      out.print ( "right-shifted: 0x" ); 
      out.print ( 0xffffffff >> ( nNybbles * 4 ), HEX );
      out.print ( "\n" );
      out.print ( "v & 0x08: 0x" ); 
      out.print ( v & 0x08, HEX );
      out.print ( "\n" );
    #endif
    strncpy ( destBuf, "********", nNybbles );
    destBuf [ nNybbles ] = '\0';
  }
  return;
}

// don't use if you can use printf ( "0h%04x" ) or something like it
void printHex ( unsigned long value, int nNybbles, Stream& out ) {
  char printHex_buf [ 9 ] = "";
  convertHex ( printHex_buf, 8, value, nNybbles, out );
  out.print ( "0x" ); out.print ( printHex_buf );
}


// NOTE hexdumping can be dangerous - need to preserve byte order
// even if little-endian

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
    
    if ( ! ( ( i + 1 ) % 8 ) ) {
      // extra space between blocks
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