#define VERSION "1.0.0"
#define VERDATE "2013-11-16"
#define PROGMONIKER "MR"

/*

  Serves characters to the master on EMMA bus, communicating speed and direction values 
  
  NOTE that Minion uses digital pins 3 and 10!
     
  Arduino connections (digital pins):
     0 RX - reserved for serial comm - to master TX
     1 TX - reserved for serial comm - to master RX
     4 indicator (blinky) LED
     9 status LED shows message receipt
 
*/

#define VERBOSE 2

#define BAUDRATE 115200

// pin definitions

// we'll blink the status LED
#define pdSTATUSLED       4
// and the indicator LED will change state with every LF detected
#define pdINDICATORLED    9

#define LOOP_DELAY_ms 100

// for Minion communications
#include <JeeLib.h>

//node ID of Rx (range 0-30)
#define idNodeSelf 10
#define idNodeOther 11
//network group (can be in the range 1-250).
#define network 210
//Freq of RF12B can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. Match freq to module
#define freq RF12_915MHZ

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

typedef struct { int speed, steering ; } Payload;
Payload rxPacket;

void setup () {

  delay ( 10000LU );
  
  Serial.begin ( BAUDRATE );
  Serial1.begin ( BAUDRATE );

  rf12_initialize ( idNodeSelf, freq, network ); //Initialize RFM12 with settings defined above

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
    snprintf ( strBuf, bufLen, "%s: Minion receiver v%s (%s)\n", PROGMONIKER, VERSION, VERDATE );
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
  
  int RFSpeed, RFSteering;
  static unsigned long lastFlashAt_ms = 0LU;
  
  if ( rf12_recvDone () ) {
    if ( rf12_crc == 0 && ( rf12_hdr & RF12_HDR_CTL ) == 0 ) {
      int node_id = (rf12_hdr & 0x1F); //extract nodeID from payload
      if (node_id == idNodeOther) { //check data is coming from node with the correct ID
        
        rxPacket = * ( Payload * ) rf12_data; // Extract the data from the payload
        
        RFSpeed = rxPacket.speed;
        RFSteering = rxPacket.steering;
        
        snprintf ( strBuf, bufLen, "%4d %4d \x0a", RFSpeed, RFSteering );
        if ( Serial ) Serial.print ( strBuf );  // echo for console
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

  #define flashInterval  500UL
  
  if ( ( millis() - lastFlashAt_ms ) > flashInterval ) {
    digitalWrite ( pdSTATUSLED, 1 - digitalRead ( pdSTATUSLED ) );
    lastFlashAt_ms = millis();
  }

}
