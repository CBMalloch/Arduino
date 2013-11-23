#define VERSION "1.1.1"
#define VERDATE "2013-11-23"
#define PROGMONIKER "MS"

/*

  Serves data to the Minion receiver, communicating speed and direction values 
       
   NOTE that Minion uses digital pins 3 and 10!

  Arduino connections (digital pins):
     4 indicator (blinky) LED
     9 status LED shows message sending
    A0 speed (will produce 0-100)
    A1 steering (will produce -10-10)
    A2 forward / reverse ( 0 is forward, 1023 is reverse )
 
*/

#define VERBOSE 2

#define BAUDRATE 115200

// pin definitions

// we'll blink the status LED
#define pdSTATUSLED       4
// and the indicator LED will change state with every LF detected
#define pdINDICATORLED    9
#define paSPEED          A0
#define paSTEERING       A1
#define paDIRECTION      A2

#define LOOP_DELAY_ms 100

// for Minion communications
#include <JeeLib.h>

//node ID of Rx (range 0-30)
#define idNodeSelf  11
#define idNodeOther 10
//network group (can be in the range 1-250).
#define network 210
//Freq of RF12B can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. Match freq to module
#define freq RF12_915MHZ

#define SENDINTERVAL_ms 500LU

byte nPinDefs;
#define PINDEF_ITEMS 3
// ( input/output mode ( 1 input ); digital pin; coil # )

/* 
  NOTE: pdESTOP and pdINTERRUPT are to be *open drain* on the bus
  so than any processor can pull them down. Thus the specification of each
  as INPUT. In this mode, setting the pin to HIGH or 1 makes it high-Z
  and enables an internal 50K pullup resistor; setting the pin to LOW or 0
  pulls it strongly to ground, thus asserting the active-low signals.
*/

short pinDefs [ ] [ PINDEF_ITEMS ] = {  
                                       { 0,  pdSTATUSLED      , -1 },
                                       { 0,  pdINDICATORLED   , -1 }
                                      };
                                      
#define bufLen 60
char strBuf [ bufLen + 1 ];
int bufPtr;

typedef struct { int direction, speed, steering ; } Payload;
Payload txPacket;

void setup () {

  delay ( 4000LU );
  
  Serial.begin ( BAUDRATE );

  rf12_initialize ( idNodeSelf, freq, network );

  nPinDefs = sizeof ( pinDefs ) / sizeof ( pinDefs [ 0 ] );
  for ( int i = 0; i < nPinDefs; i++ ) {
    if ( pinDefs [i] [0] == 1 ) {
      // input pin
      pinMode ( pinDefs [i] [1], INPUT );
      digitalWrite ( pinDefs [i] [1], 1 );  // enable internal pullups
    } else {
      // output pin
      pinMode ( pinDefs [i] [1], OUTPUT );
      digitalWrite ( pinDefs [i] [1], 0 );
    }
  }
  
  if ( Serial ) {
    bufPtr = 0;  
    snprintf ( strBuf, bufLen, "%s: Minion sender v%s (%s)\n", PROGMONIKER, VERSION, VERDATE );
    Serial.print ( strBuf );


    Serial.print("Node: ");
    Serial.print(idNodeSelf);
    Serial.print(" Freq: ");
    if (freq == RF12_433MHZ) Serial.print("433Mhz");
    if (freq == RF12_868MHZ) Serial.print("868Mhz");
    if (freq == RF12_915MHZ) Serial.print("915Mhz");
    Serial.print(" Network: ");
    Serial.println (network);
    Serial.print ( "Listening for node: " );
    Serial.println ( idNodeOther );
  }
    
}
  
void loop () {
  
  int speed, steering, direction;
  static unsigned long lastFlashAt_ms = 0LU,
                       lastSendAt_ms = 0LU;
  #define EMMAFORWARDDIR  1
  #define EMMAREVERSEDIR -1
  
  // map(value, fromLow, fromHigh, toLow, toHigh); constrain (x, a, b)
  // direction = ( analogRead ( paDIRECTION ) > 512 ) ? EMMAREVERSEDIR : EMMAFORWARDDIR;
  direction = ( analogRead ( paDIRECTION ) < 2 ) ? EMMAREVERSEDIR : EMMAFORWARDDIR;
  // speed = int ( float ( 1023 - analogRead ( paSPEED ) ) / 1023.0 * 100.0 + 0.5 );
  speed = constrain ( map ( analogRead ( paSPEED ), 1023, 450, 0, 100 ), 0, 100 );
  // steering = int ( float ( analogRead ( paSTEERING ) - 512 ) / 512.0 * 10.0 + 0.5 );
  /*
  steering = analogRead ( paSTEERING );
  if ( steering < 495 ) {
    // 0 to 495 => -1 to -10
    steering = -1 - ( steering / 55 );
  } else if ( steering > 526 ) {
    // 527 - 1023 => 1 to 10
    steering = 1 + ( steering / 55 );
  } else {
    // dead band in middle
    steering = 0;
  }
  */
  steering = constrain ( map ( analogRead ( paSTEERING ) - 50, 200, 700, -10, 10 ), -10, 10 );
  if ( abs ( steering ) < 2 ) steering = 0;


  
  rf12_recvDone ();
  /*
  
  not expecting to want to receive anything...
  
  if ( rf12_recvDone () ) {
    if ( rf12_crc == 0 && ( rf12_hdr & RF12_HDR_CTL ) == 0 ) {
      int node_id = (rf12_hdr & 0x1F); //extract nodeID from payload
      if (node_id == idNodeOther) { //check data is coming from node with the correct ID
      
        // the data is in rf12_data; its length (beyond the length byte) is in byte zero
        // the two data words are in bytes 1-2 and 3-4
        
        int packetLength = rf12_data [ 0 ];
        
        if ( packetLength == 4 ) {
          memcpy ( ( unsigned char * ) &RFSpeed,    ( unsigned char * ) rf12_data + 1, 2 );
          memcpy ( ( unsigned char * ) &RFSteering, ( unsigned char * ) rf12_data + 3, 2 );
        }
        
        snprintf ( strBuf, bufLen, "4d\04d\0\x0a", RFSpeed, RFSteering );
        Serial1.print ( strBuf );
        
        digitalWrite ( pdINDICATORLED, 1 - digitalRead ( pdINDICATORLED) );
        
      } else if ( Serial ) {
        Serial.print ( "Discarding packet from " );
        Serial.println ( node_id );
      }
    } else {
      if ( Serial ) Serial.println ( "Nonzero CRC or header control is 1" );
    }
  } //End of receive code
  */
  
  if ( ( ( millis() - lastSendAt_ms ) > SENDINTERVAL_ms ) ) {
    int i=0;
    
    if ( rf12_canSend () ) {
      if ( Serial ) Serial.println ( "can send" );
      
      while ( ! rf12_canSend () && i < 10 ) {
        rf12_recvDone();
        if ( Serial ) Serial.print ( "recv done " ); Serial.println ( i );
        i++;
      }
      
      txPacket.direction = direction;
      txPacket.speed     = speed;
      txPacket.steering  = steering;
      
      if ( Serial ) {
        Serial.print ( "Direction: " ); Serial.print ( direction );
        Serial.print ( " Speed: " ); Serial.print ( speed );
        Serial.print ( "; Steering: " ); Serial.println ( steering );
      }
      
      rf12_sendStart ( 0, &txPacket, sizeof txPacket );

      if ( Serial ) Serial.println( "send start" );
      rf12_sendWait ( 0 );
      if ( Serial ) Serial.println ( "send wait" );

      lastSendAt_ms = millis();
      digitalWrite ( pdINDICATORLED, 1 - digitalRead ( pdINDICATORLED) );
    }
  }


  #define flashInterval  500UL
  
  if ( ( millis() - lastFlashAt_ms ) > flashInterval ) {
    digitalWrite ( pdSTATUSLED, 1 - digitalRead ( pdSTATUSLED ) );
    lastFlashAt_ms = millis();
  }

}
