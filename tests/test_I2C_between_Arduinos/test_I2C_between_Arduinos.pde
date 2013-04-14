#define VERSION "0.1.6"
#define VERDATE "2011-11-01"
#define PROGMONIKER "I2C_ARD"

/* 
  Test using I2C to allow three (for example) Arduinos to communicate
  
  Master will have address 0
  Slave 1                  1
  Slave 2                  2
  Addresses will be two bits on digital pins 12 and 11 - 5V = 1, 12 is msb
  
  Protocol:
    Phase 1:
      Master sends message to slave; waits until slave repeats it back or timeout
    Phase 2: 
      Master sends occasional command to slave; slave confirms receipt
      Master occasionally requests data from a slave; slave provides
*/

#include <Wire.h>

#define BAUDRATE 19200
#define LOOPDELAY 100

#define bufLen 60
char strBuf[bufLen + 1];

#define msgBufLen 20
char msgBuf[msgBufLen + 1];

#define pinLED 13
#define pinAddrmsb 12
#define pinAddrlsb 11

// SCL is A5; use a 1.5K pullup
// SDA is A4; use a 1.5K pullup
// note that grounds need to be common, as well!

byte myAddr;
byte I2CReady;

#define I2CBufLen 20
char I2CBuf[I2CBufLen];
byte I2CBufPtr = 0;

void setup() {
  pinMode(pinLED, OUTPUT);      // sets the digital pin as output
  Serial.begin(BAUDRATE);
	snprintf(strBuf, bufLen, "%s: I2C between Arduinos v%s (%s)\n", 
	  PROGMONIKER, VERSION, VERDATE);
  Serial.print(strBuf);
  
  pinMode(pinAddrmsb, INPUT); digitalWrite(pinAddrmsb, HIGH);
  pinMode(pinAddrlsb, INPUT); digitalWrite(pinAddrlsb, HIGH);
  myAddr = digitalRead(pinAddrmsb) * 2 + digitalRead(pinAddrlsb);
  
  // for (int i = 0; i < myAddr + 5; i++) {
    // digitalWrite(pinLED, HIGH);
    // delay(500);
    // digitalWrite(pinLED, LOW);
    // delay(250);
  // }
  Serial.print ("My address is: ");
  Serial.println(myAddr, HEX);
  
  I2CReady = 0;
  Wire.begin(myAddr);        // join i2c bus (address optional for master)
  
  // for request system - but request requires number of bytes, blocks
  if (myAddr) {
    Wire.onRequest(requestEvent);
  }
  Wire.onReceive(receiveEvent);
} 

void loop() {
  static unsigned long lastBlink = 0;
  if (myAddr == 0) {
    doMasterStuff();
  } else {
    doSlaveStuff();
  }
    
  if (millis() > lastBlink + 1000) {
    digitalWrite(pinLED, 1 - digitalRead(pinLED));
    lastBlink = millis();
  }
}

void doSlaveStuff () {
  if (1 || (myAddr == 2)) {
    if (I2CReady) {
      printI2CMessage();
      int retAdd = msgBuf[0];
      I2CSend(retAdd, &msgBuf[1]);
      I2CReady = 0;
    }
  }
  delay (LOOPDELAY);
}

void doMasterStuff () {
  unsigned long tStart;
  #define masterTimeout 250
  
  if (1) {
    I2CSend(1, "AbC");
    
    tStart = millis();
    while (!I2CReady && millis() < tStart + masterTimeout) {
      digitalWrite(pinLED, 1 - digitalRead(pinLED));
    }
    if (I2CReady) {
      printI2CMessage();
      I2CReady = 0;
    } else {
      Serial.println ("Timeout");
    }
  }
  
  // -------------------------------

  if (1) {
    char replyBuf[8];
    int replyPtr;
    
    Wire.onReceive(0);
    I2CReady = 0;
    Serial.println ("Sending wire request to 1");
    Wire.requestFrom(1, 5);
    replyPtr = 0;
    while (Wire.available()) {   // slave may send less than requested
      replyBuf[replyPtr++] = Wire.receive(); // receive a byte as character
    }
    replyBuf[replyPtr] = '\0';
    Serial.print ("Reply: ");
    Serial.println (replyBuf);

    /**/
    Wire.onReceive(receiveEvent);
  }

  // -------------------------------

  if (1) {
    I2CSend(2, "dEfG");
    
    tStart = millis();
    while (!I2CReady && millis() < tStart + masterTimeout) {
      digitalWrite(pinLED, 1 - digitalRead(pinLED));
    }
    if (I2CReady) {
      printI2CMessage();
      I2CReady = 0;
    } else {
      Serial.println ("                    Timeout");
    }
  }
 
  delay(2000);
}

void I2CSend (byte address, char msg[]) {
  // return address and length byte will be synthesized and sent
  // before the message...
  for (int i = 0; i < 20 * (address - 1); i++) {
    Serial.print (" ");
  }
  Serial.print ("I2CSend to ");
  Serial.print (address, HEX);
  Serial.print ("; msg = '");
  Serial.print (msg);
  Serial.print ("' (len ");
  Serial.print (strlen(msg));
  Serial.print (")\n");
  Wire.beginTransmission(address);
  Wire.send(byte(0xff & (strlen(msg) + 1)));  // payload length
  Wire.send(myAddr);                          // return address
  for (int i = 0; i < strlen(msg); i++) {
    Wire.send(msg[i]);
  }
  Wire.endTransmission();
}

void printI2CMessage() {
  for (int i = 0; i < 20 * (int(msgBuf[0]) - 1); i++) {
    Serial.print (" ");
  }
  // got a message from somebody; it's in msgBuf
  Serial.print ("Message from ");
  Serial.print (msgBuf[0], HEX);
  Serial.print (" '");
  Serial.print (&msgBuf[1]);
  Serial.print ("' (len ");
  Serial.print (strlen(&msgBuf[1]));
  Serial.print (")\n");
}

void receiveEvent(int howMany) {
  char c;
  while (!I2CReady && Wire.available() && (I2CBufPtr < I2CBufLen)) {
    c = Wire.receive(); // receive byte as a character
    I2CBuf[I2CBufPtr++] = c;
    Serial.print (" -> ");
    Serial.println (c, HEX);
  }
  if ((I2CBufPtr > 0) && (I2CBufPtr > I2CBuf[0])) {
    // have received a complete message: len of payload incl senderAdd, senderAdd, msg
    // copy that message to the dock (strBuf)
    // cannot use strncpy here, since the message may contain zero bytes!
    memcpy(msgBuf, &I2CBuf[1], I2CBuf[0]);
    msgBuf[I2CBuf[0]] = '\0';  // add the terminating null
    // move anything that remains down to the beginning
    if (I2CBufPtr > I2CBuf[0] + 1) {
      memcpy(I2CBuf, &I2CBuf[I2CBuf[0] + 1], I2CBufPtr - I2CBuf[0] - 1);
      I2CBufPtr -= I2CBuf[0] + 1;
    } else {
      I2CBufPtr = 0;
    }
    I2CReady = 1;
  } else {
    Serial.print ("Incomplete message: '");
    Serial.print (msgBuf);
    Serial.print ("'\n");
  }
}

void requestEvent() {
  char string[8] = "ACK  ";
  Wire.send (string);
  Serial.print ("Got wire request; sent ");
  Serial.println (string);
}
