/*
 * colorId.ino assigns a color to an id
 */
 
#include <ezLCD.h>
#include <SoftwareSerial.h>


EzLCD3_SW lcd( 10, 11 );

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.cls();                      // clear the LCD  
  lcd.colorId(168, 222, 126, 93); // peach
  lcd.color(168);                 // set color 168
  lcd.fill(true);                  
  lcd.rect(0,0,200,200);          // draw filled rectangle 
}

void loop(){}

