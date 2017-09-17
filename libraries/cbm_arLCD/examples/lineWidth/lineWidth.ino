#include <ezLCD.h>
#include <SoftwareSerial.h>

EzLCD3_SW lcd(10, 11); // create lcd object using pins 10 & 11

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.cls();
  lcd.color(GREEN);   
  lcd.lineWidth(1);
  for(int i=0; i < 30; i++)
  {
     int x = random(0,319); 
     int y = random(0,239);
     lcd.line(x,y);  //draw line from the previous xy location
     delay(100);
  }  
  lcd.lineWidth(3);
  for(int i=0; i < 30; i++)
  {
     int x = random(0,319); 
     int y = random(0,239);
     lcd.line(x,y);  //draw line from the previous xy location
     delay(100);
  }
}

void loop(){ }

