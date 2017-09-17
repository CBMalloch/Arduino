#include <ezLCD.h>
#include <SoftwareSerial.h>

EzLCD3_SW lcd(10, 11); // create lcd object using pins 10 & 11

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.cls(); 
  lcd.image("logo200.gif",10,10); 
  delay(2000);
  lcd.cls(); 
  lcd.image("logo200.gif",10,10,1); 
}

void loop(){}

