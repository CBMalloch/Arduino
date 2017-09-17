/*
 * color.ino displays pre-defined colors
 */
 
#include <ezLCD.h>
#include <SoftwareSerial.h>


EzLCD3_SW lcd( 10, 11 );

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.cls();
  lcd.fill(true);
  for(int i=0; i < 16; i++)
  {
    lcd.color(i);    
    lcd.rect(i*16,0, 16, 100);
  }
  for(int i=16; i < 168; i++)  
  {
    lcd.color(i);    
    int pos = (i-16);
    lcd.rect(pos*2,120, 2, 100);
  }  
}

void loop(){}

