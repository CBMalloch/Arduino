#include <ezLCD.h>
#include <SoftwareSerial.h>

EzLCD3_SW lcd(10, 11); // create lcd object using pins 10 & 11

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.cls();
  lcd.color(RED);   
  for(int i=0; i < 100; i++)
  {
     int x = random(0,319); 
     int y = random(0,239);
     lcd.line(x,y);  //draw line from the previous xy location
     delay(100);
  }
   lcd.cls();  
}

void loop()
{  
  int color = random(0,168); 
  lcd.color(color);
  //draw line specifying all coordinates
  int x = random(0,200); 
  int y = random(0,100);
  int length = random(20, 100);
  lcd.line(x,y, x+length, y+length);  
}

