#include <ezLCD.h>
#include <SoftwareSerial.h>

EzLCD3_SW lcd(10, 11); // create lcd object using pins 10 & 11

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.cls(); 
  int x = 160;
  int y = 120; 
  int size = 20;
  lcd.xy(x,y);
  for(int i=0; i < 100; i++)
  {
     lcd.color(i);  
     lcd.circle( size );  
     size += 4; 
     delay(100);
  } 
}

void loop(){}

