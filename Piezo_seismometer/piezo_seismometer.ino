/*
                                                     
                 power 9VDC                          
                                                     
                                                     
                    ┌───────────┐                    
  piezo ──► amp ───►│ATmega     │ ──► ESP-01 AP-STA  
                    │  fast loop│       MQTT         
                    └───────────┘       HTML         
                                                     
   ( asciiflow.com )
*/

#define PROGNAME "piezo_seismometer"
#define VERSION "0.2.1" 
#define VERDATE "2024-05-20"
#define PROGMONIKER "PSEISMO"

#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

#include <cbmThrobber.h>


#define SECOND_ms  ( 1000UL )
#define MINUTE_ms  ( 60UL * SECOND_ms )
#define HOUR_ms    ( 60UL * MINUTE_ms )

// ***************************************
// ***************************************
#pragma mark -> vars MAIN PARAMETERS

#define BAUDRATE 115200
#define VERBOSE 2
#define TESTING false

#define EXPECT_TEXT_TO_MQTT
#undef EXPECT_SERIAL_PLOTTER

// const float low_cutoff = 0.01;
// const float high_cutoff = 40.0;


// ***************************************
// ***************************************

/********************************** GPIO **************************************/
#pragma mark -> vars GPIO

#define INVERSE_LOGIC_OFF 1
#define INVERSE_LOGIC_ON  0

const int pdThrobber = 3;
const int pdConnecting = pdThrobber;
const int pdAlarm = 4;
const int pdSDCS = 10;

const int paSignal = A0;
const int paPot = A3;

/******************************* Global Vars **********************************/
#pragma mark -> vars global

RTC_DS1307 rtc;
DateTime now;
// Sd2Card card;

const int pBufLen = 100;
char pBuf [ pBufLen ];
const int dBufLen = 25;
char dBuf [ dBufLen ];
 
#define throbberIsInverted_P false
#define throbberPWM_P        true
//                         pin            inverted_P         PWM_P
cbmThrobber throbber ( pdThrobber, throbberIsInverted_P, throbberPWM_P );

/**************************** Function Prototypes *****************************/
#pragma mark -> function prototypes

void initializeGPIO ();
void initializeTime ();
void initializeSD ();
void printForMQTT ( char topicEnd[], char value[], bool retain = false );
void logAppend ( char filename[], char entry[] );

/******************************************************************************/

void setup() {

  
  initializeGPIO();
  initializeTime();
  initializeSD();

  #ifdef EXPECT_TEXT_TO_MQTT
    // no longer need to connect
  #endif
  
  #ifndef EXPECT_SERIAL_PLOTTER
    snprintf ( pBuf, pBufLen, "\n\n# %s v%s %s cbm, starting at %04d-%02d-%02d %02d:%02d:%02dZ\n", 
      PROGNAME, VERSION, VERDATE, 
      now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second() );
    Serial.print ( pBuf );
  #endif

}

void loop() {
  const unsigned long reportingInterval_ms = 2UL * SECOND_ms;
  static unsigned long lastReportAt_ms = 0UL;
  
  const unsigned long alarmInterval_ms = 500UL; // 5UL * SECOND_ms;
  static unsigned long lastAlarmAt_ms = 0UL;

  int threshold = analogRead ( paPot ) / 2;  
  int counts = analogRead ( paSignal );
  
  static unsigned long maxCountsForPrinting = 0UL;
  maxCountsForPrinting = ( (unsigned long)counts > maxCountsForPrinting ) ? (unsigned long) counts : maxCountsForPrinting;
  static unsigned long maxCountsForLogging = 0UL;
  maxCountsForLogging = ( (unsigned long)counts > maxCountsForLogging ) ? (unsigned long) counts : maxCountsForLogging;
  
  if ( ( millis() - lastReportAt_ms ) > reportingInterval_ms ) {
    #ifdef EXPECT_TEXT_TO_MQTT
      // e.g. { "command": "send", "topic": "test", "value": "banana_n", "retain": "0" }
      
      snprintf ( pBuf, pBufLen, "%d", maxCountsForPrinting );
      printForMQTT ( "energy", pBuf );
      snprintf ( pBuf, pBufLen, "%d", threshold );
      printForMQTT ( "threshold", pBuf );
      printForMQTT ( "triggered", maxCountsForPrinting > threshold ? "1" : "0" );
    #else
      snprintf ( pBuf, pBufLen, "%d, %5lu", threshold, maxCountsForPrinting );
      Serial.println ( pBuf );
    #endif
    // snprintf ( pBuf, pBufLen, "        %5lu, %5lu", maxCountsForPrinting, maxCountsForLogging );
    // Serial.println ( pBuf );
    maxCountsForPrinting = 0UL;
    lastReportAt_ms = millis();
  }
  
  if ( maxCountsForLogging > threshold ) {
    const unsigned long logPace_ms = 30UL * SECOND_ms;
    static unsigned long lastLogEventAt_ms = 0UL;
    
    static unsigned long nNotReported = 0;
    if ( ( lastLogEventAt_ms == 0UL ) 
      || ( ( millis() - lastLogEventAt_ms ) > logPace_ms ) ) {
      snprintf ( pBuf, pBufLen, "%lu events with maximum counts %5lu", 
          ( nNotReported + 1UL ), maxCountsForLogging );
      logAppend ( "seismo.log", pBuf );
      nNotReported = 0;
      maxCountsForLogging = 0UL;
      lastLogEventAt_ms = millis();
      lastAlarmAt_ms = millis();
      // Serial.println ( pBuf );
    } else {
      if ( counts > threshold ) {
        // too soon - don't add a report, but note how many times this happens
        lastAlarmAt_ms = millis();
        nNotReported++;
      }
    }
  }
  
  digitalWrite ( pdAlarm, ( ( millis() - lastAlarmAt_ms ) < alarmInterval_ms ) );
  
  delay ( 2 );
  throbber.throb ( 500 );
}

// *****************************************************************************
// **************************** Initializations ********************************
// *****************************************************************************

void initializeGPIO () {

  #ifdef pdConnecting
    pinMode ( pdConnecting, OUTPUT );
    indicateConnecting ( 1 );
  #endif
  #ifdef pdThrobber
    pinMode ( pdThrobber, OUTPUT ); 
    digitalWrite ( pdThrobber, throbberIsInverted_P );
  #endif

  analogReference(DEFAULT);
  
  pinMode ( pdAlarm, OUTPUT );
  pinMode ( pdSDCS, OUTPUT );
  
  Serial.begin ( BAUDRATE ); 
  while ( !Serial && ( millis() < 5000 ) ) {
    indicateConnecting ( -1 );
    delay ( 200 );  // wait a little more, if necessary, for Serial to come up
    yield ();
  }
  // Serial.print ( F ( "\n\n# GPIO initialized\n" ) );
  // if ( Serial ) hardware_sleep_p = false;   // failsafe to disallow sleep lockup

  indicateConnecting ( 0 );
}

void initializeTime () {
  if ( ! rtc.begin() ) {
    Serial.println ( F( "Couldn't find RTC" ) );
    Serial.flush();
    while ( 1 ) delay ( 10 );
  }

  if ( ! rtc.isrunning( )) {
    Serial.println( F( "RTC is NOT running!" ) );
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust ( DateTime ( F(__DATE__), F(__TIME__) ) );
  }
  // rtc.adjust ( 1715984960 );
  now = rtc.now();
  // Serial.print ( F ( "# Unix secs since 1980: " ) ); Serial.println ( now.unixtime() );
  
}

void initializeSD () {
  // Serial.print ( F ( "\n# Initializing SD card..." ) );
  if ( ! SD.begin ( pdSDCS ) ) {
  // if ( ! card.init ( SPI_FULL_SPEED, pdSDCS ) ) {
  // if ( ! card.begin ( pdSDCS ) ) {
    Serial.println ( F ( "No SD card" ) );
    printForMQTT ( "telemetry/error", "No SD card", 1 );    delay ( 10 );
    snprintf ( dBuf, dBufLen, "%04d-%02d-%02d %02d:%02d:%02dZ", 
      now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()
    );
    printForMQTT ( "telemetry/errorAt", dBuf, 1 );
    while ( 1 ) delay ( 10 );
  } else {
    // Serial.println ( F ( "OK" ) );
  }
}

// *****************************************************************************
// ********************************** util *************************************
// *****************************************************************************

void logAppend ( char filename[], char entry[] ) {

  File dataFile = SD.open ( filename, FILE_WRITE );
  const int pBufLen = 100;
  char pBuf [ pBufLen ];

  snprintf ( dBuf, dBufLen, "%04d-%02d-%02d %02d:%02d:%02dZ", 
    now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()
  );

  // if the file is available, write to it:
  if ( dataFile ) {
    dataFile.seek ( EOF );           // append
    
    now = rtc.now();
    snprintf ( pBuf, pBufLen, "%s: %s", dBuf, entry );
    dataFile.println ( pBuf );
    dataFile.close();
  } else {
    Serial.print ( dBuf );
    Serial.print ( F ( ": error " ) );
    // Serial.print ( SD.errorCode() ); 
    Serial.print (  F ( " opening file " ) ); 
    Serial.println ( filename );
    
    
    snprintf ( pBuf, pBufLen, "Could not open file %s", filename );
    printForMQTT ( "telemetry/error", pBuf, 1 );    delay ( 10 );
    printForMQTT ( "telemetry/errorAt", dBuf, 1 );

  }
}


void printForMQTT ( char topicEnd[], char value[], bool retain ) {
  Serial.print ( F ( "{\"command\": \"send\", \"topic\": \"seismo/piezo/" ) );
  Serial.print ( topicEnd );
  Serial.print ( F ( "\", \"value\": \"" ) );
  Serial.print ( value );
  Serial.print ( F ( "\"" ) );
  if ( retain ) {
    Serial.print ( F ( ", \"retain\": \"1\"" ) );
  }
  Serial.println ( F ( "}" ) );
  delay ( 50 );
}

void indicateConnecting ( int value ) {
  #if defined ( pdConnecting )
    // 1 turns on; 0 turns off; -1 inverts
    // inverse true iff pdConnecting is inverse logic
    bool inverse = true;
    if ( value == -1 ) {
      value = 1 - digitalRead ( pdConnecting );
    } else {
      value != inverse;
    }
    digitalWrite ( pdConnecting, value != inverse );
  #endif
}

// *****************************************************************************
// *****************************************************************************
// *****************************************************************************
