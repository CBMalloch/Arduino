#include <ezLCD.h>
#include <SoftwareSerial.h>

EzLCD3_SW lcd(10, 11); // create lcd object using pins 10 & 11

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.cls(); 
  int x=50;
  int y=50; 
  int size = 100;
  for(int i=0; i < 100; i++)
  {
     lcd.color(i);  
    lcd.ellipse(x, y, size );
     x = x + 2;
     y = x;
     size += 4; 
     delay(100);
  } 
}

void loop(){}

