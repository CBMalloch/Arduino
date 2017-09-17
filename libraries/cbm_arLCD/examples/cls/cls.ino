#include <ezLCD.h>
#include <SoftwareSerial.h>

EzLCD3_SW lcd(10, 11); // create lcd object using pins 10 & 11

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.cls();  // clear screen to black
  lcd.rect(0,0,100,100);
  delay(1000); 
  lcd.cls(RED); // clear screen to red 

}

void loop(){ }
