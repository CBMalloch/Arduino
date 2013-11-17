//Modified 9/21/12 by Nate and Jake @minioncity.com
//to make a simple chat program
//Simple RFM12B wireless demo - Receiver - no ack
//Glyn Hudson openenergymonitor.org GNU GPL V3 12/4/12
//Credit to JCW from Jeelabs.org for RFM12
 
#include <JeeLib.h>
 
//node ID of Rx (range 0-30)
#define myNodeID 30
const int Rx_NodeID=20; //emonTx node ID
//network group (can be in the range 1-250).
#define network 210
//Freq of RF12B can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. Match freq to module
#define freq RF12_915MHZ
 
typedef struct { int c1,c2 ; } Payload; // create structure - a neat way of packaging data for RF comms
Payload rxPacket;
Payload txPacket;
 
 
void setup() {
 
 rf12_initialize ( myNodeID, freq, network ); //Initialize RFM12 with settings defined above
 Serial.begin(9600);
 while (!Serial) {
  ; // wait for serial port to connect. Needed for Leonardo only
 }
 
Serial.println("RF12B demo Receiver - Simple demo");
 
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
 
void loop() {
 
  if ( rf12_recvDone () ) {
    if ( rf12_crc == 0 && ( rf12_hdr & RF12_HDR_CTL ) == 0 ) {
      int node_id = (rf12_hdr & 0x1F); //extract nodeID from payload
      if (node_id == Rx_NodeID) { //check data is coming from node with the correct ID
        rxPacket = * ( Payload * ) rf12_data; // Extract the data from the payload
        char temp1=rxPacket.c1;
        char temp2=rxPacket.c2;
        Serial.print(temp1);
        Serial.print(temp2);
      } else {
        Serial.print ( "Discarding packet from " );
        Serial.println ( node_id );
      }
    } else {
      Serial.println ( "Nonzero CRC or header control is 1" );
    }
  } //End of receive code

  if (Serial.available()>0) {
    if (rf12_canSend()) {
      txPacket.c1=Serial.read();
      txPacket.c2=Serial.read();
      int i=0;
      while (!rf12_canSend() && i<10) {
        rf12_recvDone();
        i++;
      }
      rf12_sendStart ( 0, &txPacket, sizeof txPacket );

      char send1=txPacket.c1;
      char send2=txPacket.c2;
      Serial.print ( send1 );
      Serial.print ( send2 );
      delay(10); 
    }
  }
 
}//end of void LOOP()