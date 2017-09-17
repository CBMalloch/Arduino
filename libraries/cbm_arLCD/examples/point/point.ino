#include <ezLCD.h>
#include <SoftwareSerial.h>

EzLCD3_SW lcd(10, 11); // create lcd object using pins 10 & 11

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.cls(); 
}

void loop()
{
  int color = random(0,168); 
  lcd.color(color);
  int x = random(0,319); 
  int y = random(0,239);
  lcd.point(x,y);  
}

