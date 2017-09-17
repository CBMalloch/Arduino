// cbmNetworkInfo.h
#ifndef cbmnetworkinfo_h
#define cbmnetworkinfo_h

// for stuff to include into your Arduino program, see Google drive / Stationery / ESP8266_MQTT.cpp

#if ARDUINO >= 100
 #include "Arduino.h"
 // #include "Print.h"
#else
 #include "WProgram.h"
#endif
  
#include <ESP8266WiFi.h>

// enum doesn't work -- it's evaluated AFTER the preprocessor!
// enum { M5, CBMM5, CBMBELKIN, CBMDDWRT };
#define M5                   0
#define CBMM5                1
#define CBMDATACOL          10
#define CBMDDWRT            20
#define CBMDDWRT3           25
#define CBMDDWRT3GUEST      35
#define CBMBELKIN           40
#define CBM_RASPI_MOSQUITTO 60

#define cbm_username "cbmalloch"
#define cbm_key      ""

/************************* local MQTT Setup ***************************/

#define CBM_MQTT_SERVER      "mosquitto"
#define CBM_MQTT_SERVERPORT  1883
#define CBM_MQTT_USERNAME    cbm_username
#define CBM_MQTT_KEY         cbm_key

/************************* CloudMQTT MQTT Setup ************************/

#define cmq_username "rwbbtmcr"
#define cmq_key      "nAcI9L39ZhXF"

/************************* Adafruit MQTT Setup ************************/

#define aio_username "cbmalloch"
#define aio_key      "0b64fe97c019e00d1a8bd2ddc7b03ebe6ea5468f"

/**********************************************************************/

void describeESP ( char* chipName );

class cbmNetworkInfo {

  public:

    cbmNetworkInfo ( ); // Constructor
    
    void init ( int wifi_locale );
    void setCredentials ( int wifi_locale );
    void setNetworkBaseIP ( int wifi_locale );
    void describeESP ( char* chipName, HardwareSerial *strSerial = &Serial );
    void describeNetwork ( HardwareSerial *strSerial = &Serial );

    IPAddress ip;
    // IPAddress dns  ( 151, 203,   0,  84 );  //   shown in Ethernet doc, but not used by ESP8266
    IPAddress gw;
    IPAddress mask;
    IPAddress dns;
    
    #define CBM_NI_STRLEN 20
    char ssid [ CBM_NI_STRLEN ];
    char password [ CBM_NI_STRLEN ];
    
    unsigned long chipId;
    #define CBM_NI_CHIPNAME_STRLEN 12
    char chipName [ CBM_NI_CHIPNAME_STRLEN ];
    
    void printIP ( IPAddress ip, HardwareSerial *strSerial = &Serial );
    
	private:
    void fixIP ( );

};
  
#endif
