/*
 * analogMeter.ino displays analog meter
 */
 
#include <ezLCD.h>
#include <SoftwareSerial.h>


EzLCD3_SW lcd( 10, 11 );

int xPos = 50;  // horizontal position
int yPos = 30;   // vertical position
int width = 200;
int height = 200;
int option = 1; // 1=draw, 2=disabled, 3=ring, 4=accuracy
int type = 0; //  0=full, 1=half, 2=quarter.

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.cls( WHITE );
  lcd.fontw( 1, "0" );
  lcd.string( 1, "ALOG_0" ); // stringId 1
  lcd.theme( 1, 155 ,152, 3, 3, 3, 1, 4, 5, 0, 16 );
  lcd.analogMeter( 1, xPos,yPos, width, height, option, 0, 0, 1023, 1, 1, type );
}

void loop()
{
   int value = analogRead(0);
   lcd.wvalue(1, value);
 }
