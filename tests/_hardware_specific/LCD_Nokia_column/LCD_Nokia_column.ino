/*
  light meter - read light intensity using a photo sensor and display it on a PCD8544 LCD.
 
  Modified by Charles B. Malloch, PhD
  to use a phototransistor rather than a thermometer
  min voltage drop is 1.08 across the transistor
  we now use a logarithmic output scaling by function map_log
  
*/
 
#define PROGMONIKER F( "Light_Meter" )
#define VERSION F( "v1.3.0" )
#define VERDATE F( "2014-10-12" )

#define BAUDRATE 9600
#define PRINT_INTERVAL_MS 20
#define BLINK_INTERVAL_MS 500
#define DISPLAY_INTERVAL_MS 250

#include <PCD8544.h>
#include <log_pot.h>

Log_pot exp_display;

static const byte sensorPin = 3;
static const byte ledPin = 13;

// The dimensions of the LCD (in pixels)...
static const byte LCD_WIDTH = 84;
static const byte LCD_HEIGHT = 48;

// The number of lines for the temperature chart...
static const byte CHART_HEIGHT = 5;

// A custom "degrees" symbol...
static const byte DEGREES_CHAR = 1;
static const byte degrees_glyph[] = { 0x00, 0x07, 0x05, 0x07, 0x00 };

// A bitmap graphic (10x2) of a thermometer...
static const byte THERMO_WIDTH = 10;
static const byte THERMO_HEIGHT = 2;
static const byte thermometer[] = { 0x00, 0x00, 0x48, 0xfe, 0x01, 0xfe, 0x00, 0x02, 0x05, 0x02,
                                    0x00, 0x00, 0x62, 0xff, 0xfe, 0xff, 0x60, 0x00, 0x00, 0x00};

#define pdNOKIA_RST  7
#define pdNOKIA_DC   5
#define pdNOKIA_DN   4
#define pdNOKIA_SCLK 3
#define pdNOKIA_CS   6

static PCD8544 lcd ( pdNOKIA_SCLK, pdNOKIA_DN, pdNOKIA_DC, pdNOKIA_RST, pdNOKIA_CS );
float fullScale_V = 5.0;
float minScale_V  = 1.08;  // min voltage; used for reading a phototransistor

void setup() {
  lcd.begin(LCD_WIDTH, LCD_HEIGHT);
  Serial.begin ( BAUDRATE );
  
  // Register the custom symbol...
  lcd.createChar(DEGREES_CHAR, degrees_glyph);
  
  pinMode(ledPin, OUTPUT);

  // The internal 1.1V reference provides for better
  // resolution from the LM35, and is also more stable
  // when powered from either a battery or USB...
  // analogReference(INTERNAL);
  // fullScale_V = 1.1;
  // minScale_V = 0.0;
  
  exp_display.init ( 0.8 );
  
  Serial.print ( F ( "\n\n\n" ) );
  Serial.print ( PROGMONIKER );
  Serial.print ( F ( " " ) );
  Serial.print ( VERSION );
  Serial.print ( F ( " (" ) );
  Serial.print ( VERDATE );
  Serial.print ( F ( ")" ) ); 

}


void loop() {
  // Start beyond the edge of the screen...
  static byte xChart = LCD_WIDTH;
  static unsigned long lastDisplayAt_ms = 0UL;
  static unsigned long lastPrintAt_ms = 0UL;
  static unsigned long lastBlinkAt_ms = 0UL;
  
  // Read the voltage
  float V = fullScale_V * analogRead ( sensorPin )  / 1024.0;

  if ( ( millis() - lastBlinkAt_ms ) > DISPLAY_INTERVAL_MS ) {
    
    // Print the temperature (using the custom "degrees" symbol)...
    lcd.setCursor(0, 0);
    lcd.print("V: ");
    lcd.print( V, 2);
    lcd.print("V ");
    
    float scaled_V = ( V - minScale_V ) / ( fullScale_V - minScale_V );
    // exp_display will transform an x value in (0,1) to a y value in (0,1).
    int colHeight = constrain ( map ( 1024 * exp_display.value ( scaled_V ), 0, 1024, 0, CHART_HEIGHT * 8 ), 0, CHART_HEIGHT * 8 );
    
    lcd.setCursor ( 0, 1 );
    lcd.clearLine ();
    lcd.print ( colHeight );

    // Draw the thermometer bitmap at the bottom left corner...
    lcd.setCursor(0, LCD_HEIGHT/8 - THERMO_HEIGHT);
    lcd.drawBitmap(thermometer, THERMO_WIDTH, THERMO_HEIGHT);

    // Update the temperature chart...  
    lcd.setCursor(xChart, 1);
    //lcd.drawColumn ( CHART_HEIGHT, map ( temp * 1000, minScale_V * 1000, fullScale_V * 1000, 0, CHART_HEIGHT*8 ) );  // ...clipped to the 0-45C range.
    lcd.drawColumn ( CHART_HEIGHT, colHeight );  // ...clipped to the 0-45C range.
    lcd.drawColumn ( CHART_HEIGHT, 0 );         // ...with a clear marker to see the current chart position.
  
    // Wrap the chart's current position...
    if (xChart >= LCD_WIDTH) {
      xChart = THERMO_WIDTH + 2;
    }
    xChart++;
    lastDisplayAt_ms = millis();
  }

  if ( ( millis() - lastPrintAt_ms ) > PRINT_INTERVAL_MS ) {
    // for graphing to "Bluetooth Graphics" on tablet, format is "Evalue1,value2,value3...\n"
    Serial.print ( "E" ); Serial.print ( V );
    Serial.println ();
    lastPrintAt_ms = millis();
  }

  if ( ( millis() - lastBlinkAt_ms ) > BLINK_INTERVAL_MS ) {
   digitalWrite( ledPin, 1 - digitalRead ( ledPin ) );  
   lastBlinkAt_ms = millis();
  }
  
}
 
/* EOF - Thermometer.ino  */

