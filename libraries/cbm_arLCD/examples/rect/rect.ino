#include <ezLCD.h>
#include <SoftwareSerial.h>

EzLCD3_SW lcd(10, 11); // create lcd object using pins 10 & 11

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.cls(); 
  int x=2;
  int y=2; 
  int width = 300;
  int height = 200;
  for(int i=0; i < 100; i++)
  {
     lcd.color(i);  
     lcd.rect(x,y,width, height );  //draw line from the previous xy location
     x = x + 2;
     y = y + 2;
     width = width -4;
     height = height -4;
     delay(100);
  } 
}

void loop(){}

