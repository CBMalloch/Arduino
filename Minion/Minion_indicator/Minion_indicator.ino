//Modified 9/21/12 by Nate and Jake @minioncity.com
//to make a simple chat program
//Simple RFM12B wireless demo - Receiver - no ack
//Glyn Hudson openenergymonitor.org GNU GPL V3 12/4/12
//Credit to JCW from Jeelabs.org for RFM12
 
#include <JeeLib.h>

#define pdLED 10
 
//node ID of Rx (range 0-30)
#define myNodeID 20
const int Rx_NodeID=30; //emonTx node ID

//network group (can be in the range 1-250).
#define network 210
//Freq of RF12B can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. Match freq to module
#define freq RF12_915MHZ

#define BAUDRATE 115200
 
typedef struct { int c1,c2 ; } Payload; // create structure - a neat way of packaging data for RF comms
Payload rxPacket;
Payload txPacket;
 
 
void setup() {
 
  rf12_initialize ( myNodeID, freq, network ); //Initialize RFM12 with settings defined above
  pinMode ( pdLED, OUTPUT );
  digitalWrite ( pdLED, 0 );
  Serial.begin ( BAUDRATE );
  // while (!Serial) {
    // ; // wait for serial port to connect. Needed for Leonardo only
  // }

  if ( Serial ) Serial.println("RF12B demo Receiver - Simple demo");

  if ( Serial ) {
    Serial.print("Node: ");
    Serial.print(myNodeID);
    Serial.print(" Freq: ");
    if (freq == RF12_433MHZ) Serial.print("433Mhz");
    if (freq == RF12_868MHZ) Serial.print("868Mhz");
    if (freq == RF12_915MHZ) Serial.print("915Mhz");
    Serial.print(" Network: ");
    Serial.println (network);
    Serial.print ( "Listening for node: " );
    Serial.println ( Rx_NodeID );
  }
}
 
void loop() {

  static unsigned long lastRecvAt_ms = 0LU;
 
  if ( rf12_recvDone () ) {
    if ( rf12_crc == 0 && ( rf12_hdr & RF12_HDR_CTL ) == 0 ) {
      int node_id = (rf12_hdr & 0x1F); //extract nodeID from payload
      if (node_id == Rx_NodeID) { //check data is coming from node with the correct ID
        rxPacket = * ( Payload * ) rf12_data; // Extract the data from the payload
        char temp1=rxPacket.c1;
        char temp2=rxPacket.c2;
        if ( Serial ) Serial.print(temp1);
        if ( Serial ) Serial.print(temp2);
        if ( temp1 == 0x41 && temp2 == 0x42 ) {
          digitalWrite ( pdLED, 1 );
          lastRecvAt_ms = millis ();
        }
      } else {
        if ( Serial ) Serial.print ( "Discarding packet from " );
        if ( Serial ) Serial.println ( node_id );
      }
    } else {
      if ( Serial ) Serial.println ( "Nonzero CRC or header control is 1" );
    }
  } //End of receive code
  
  if ( ( millis () - lastRecvAt_ms ) > 2000LU ) {
    digitalWrite ( pdLED, 0 );
  }
 
}//end of void LOOP()