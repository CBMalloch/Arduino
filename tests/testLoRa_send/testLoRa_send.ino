#define PROGNAME  "testLoRa_send"
#define VERSION   "0.1.3"
#define VERDATE   "2017-11-26"

/*
  <https://learn.adafruit.com/adafruit-huzzah32-esp32-feather/using-with-arduino-ide>
  <https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/mac.md>
  
  With 5-second-ly wake-ups, runs about 10 hours on full 500mAh battery,
  without disabling the LCD panel.
*/

#define HAS_OLED

#include <SPI.h>
#include <LoRa.h>
#ifdef HAS_OLED
  #include <U8x8lib.h>
#endif

#define BAUDRATE 115200
#define VERBOSE 12
#define LORA_FREQ_BAND 433E6

const int pdLoRa_SS  = 18;    // CS chip select
const int pdLoRa_RST = 14;    // reset
const int pdLoRa_DI0 = 26;    // IRQ interrupt request

#ifdef HAS_OLED
  const int SX1278_SCK  =  5;   // clock
  const int SX1278_MISO = 19;   // master in slave out
  const int SX1278_MOSI = 27;   // master out slave in

  const int U8X8_SCL = 15;      // I2C clock
  const int U8X8_SDA =  4;      // I2C data
  const int U8X8_RST = 16;      // I2C reset
#endif

const int pdBlink = 25;       // LED - not 13!
const int pdSleep = 17;
const int paBat   = 13;       // reads half the battery voltage?
  // in centivolts / count, perhaps


#ifdef HAS_OLED
  U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8( U8X8_SCL, U8X8_SDA, U8X8_RST );
  // or U8G2_ST7920_128X64_1_SW_SPI(rotation, clock, data, cs [, reset])
  // U8G2_R0
#endif

const unsigned long sleepPeriod_ms = 5000UL;

RTC_DATA_ATTR int bootCount = 0;

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}

void setup () {

  pinMode ( pdSleep, INPUT_PULLUP );
  
  Serial.begin ( BAUDRATE );
  while ( !Serial && millis () < 2000 );
  
  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  #ifdef HAS_OLED
    SPI.begin ( SX1278_SCK, SX1278_MISO, SX1278_MOSI, pdLoRa_SS );
  
    u8x8.begin();
    u8x8.setFont ( u8x8_font_chroma48medium8_r );
    u8x8.drawString( 0, 1, "LoRa Sender" );
  #endif
  
  LoRa.setPins ( pdLoRa_SS, pdLoRa_RST, pdLoRa_DI0 );
  if ( ! LoRa.begin ( LORA_FREQ_BAND ) ) {
    Serial.println ( "LoRa init failed!" );
    #ifdef HAS_OLED
      u8x8.drawString( 0, 1, "LoRa init failed!" );
    #endif
    while ( 1 );
  }
  
  if ( Serial ) Serial.println ( PROGNAME " v" VERSION " " VERDATE " cbm" );
  #ifdef HAS_OLED
    u8x8.drawString( 0, 1, PROGNAME );
    u8x8.drawString( 0, 2, "v" VERSION " cbm" );
    u8x8.drawString( 0, 3, VERDATE );
  #endif
  delay ( 100 );

}

void loop() {

  static unsigned long lastXMitAt_ms = 0UL;
  const unsigned long xMitDelay_ms = 500UL;
  
  int counts = 0;
  
  // lastXMitAt_ms will be zero first time through whether or not we sleep
  
  if ( ! lastXMitAt_ms || ( millis() - lastXMitAt_ms ) > sleepPeriod_ms ) {
  
    // Serial.println ( "Packet number: " + String ( bootCount ) );
    
    counts = analogRead ( paBat );
    
    // send packet
    LoRa.beginPacket();
    // LoRa.print ( "cbm " );
    LoRa.print ( bootCount );
    LoRa.print ("; bat: " );
    LoRa.print ( counts );
    LoRa.endPacket();
    
    #ifdef HAS_OLED
      char bc [ 10 ];
      snprintf ( bc, 10, "%5d", bootCount );
      u8x8.drawString ( 0, 5, bc );
    #endif

    bootCount++;
    
    lastXMitAt_ms = millis();
    
  }


  if ( digitalRead ( pdSleep ) ) {
  
    // **************************** go to bed *********************************

    Serial.print ( "Process took " ); Serial.print ( millis() ); Serial.println ( " ms." );
    Serial.println ( "Entering sleep mode" );
    // digitalWrite ( pdThrobber, 1 );  // turns blue light OFF

    // ESP.deepSleep ( sleepPeriod_us, WAKE_RF_DEFAULT );
    esp_sleep_enable_timer_wakeup ( sleepPeriod_ms * 1000ULL );
    esp_deep_sleep_start ();

    while ( 1 ) {
      Serial.println ( millis() );
      delay ( 1 );
    }

    // testing shows it takes precisely 100ms to go to sleep...
    delay ( 100 );

    // should never reach here; when deepSleep wakes back up, it does a reset.
    // but it actually keeps going for a little bit 
    //   -- probably deepSleep initiates sleep asynchronously and takes a little time
    Serial.println ( "Waking back up");
  
  }
      
}
