// #include <SoftwareSerial.h>
// for Teensy, which has Serial2
// so put the ESP8266 onto Serial2 pins 10 (TX) and 11 (RX)

#define SSID "m5"
#define PASS "m5launch"

#define BAUDRATE 115200

#define DST_IP "220.181.111.85" //baidu.com

// SoftwareSerial dbgSerial(10, 11); // RX, TX
//   was used for terminal
// The regular serial was used for the ESP8266
// Now regular Serial for terminal
//   and Serial2 for ESP8266
void setup() {
  // Open serial communications and wait for port to open:
  Serial2.begin(BAUDRATE);
  Serial2.setTimeout(5000);

  // dbgSerial.begin(9600); //can't be faster than 19200 for softserial
  // dbgSerial.println("ESP8266 Demo");
  Serial.begin ( BAUDRATE );
  delay ( 5000 );
  Serial.println ( "ESP8266 Demo on Serial2" );

  //test if the module is ready
  Serial2.println("AT+RST");
  delay(1000);
  if(Serial2.find("ready")) {
    Serial.println("Module is ready");
  } else {
    Serial.println("Module have no response.");
    while(1);
  }
  delay(1000);
  //connect to the wifi
  boolean connected=false;
  for(int i=0;i<5;i++) {
    if ( connectWiFi() ) {
      connected = true;
      break;
    }
  }
  if (!connected){ while(1); }
  delay(5000);
  //print the ip addr
  /*
    Serial2.println("AT+CIFSR");
    Serial.println("ip address:");
    while (Serial2.available()) Serial.write(Serial2.read());*/
  //set the single connection mode
  Serial2.println("AT+CIPMUX=0");
}
void loop()
{
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += DST_IP;
  cmd += "\",80";
  Serial2.println(cmd);
  Serial.println(cmd);
  if(Serial2.find("Error")) return;
  cmd = "GET / HTTP/1.0\r\n\r\n";
  Serial2.print("AT+CIPSEND=");
  Serial2.println(cmd.length());
  if(Serial2.find(">")) {
    Serial.print(">");
  } else {
    Serial2.println("AT+CIPCLOSE");
    Serial.println("connect timeout");
    delay(1000);
    return;
  }
  Serial2.print(cmd);
  delay(2000);
  //Serial2.find("+IPD");
  while (Serial2.available()) {
    char c = Serial2.read();
    Serial.write(c);
    if (c=='\r') Serial.print('\n');
  }
  Serial.println ("====");
  delay(1000);
}

boolean connectWiFi() {
  Serial2.println("AT+CWMODE=1");
  String cmd="AT+CWJAP=\"";
  cmd+=SSID;
  cmd+="\",\"";
  cmd+=PASS;
  cmd+="\"";
  Serial.println(cmd);
  Serial2.println(cmd);
  delay(2000);
  if(Serial2.find("OK")) {
    Serial.println("OK, Connected to WiFi.");
    return true;
  } else {
    Serial.println("Can not connect to the WiFi.");
    return false;
  }
}
