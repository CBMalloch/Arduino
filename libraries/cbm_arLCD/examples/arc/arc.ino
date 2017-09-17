#include <ezLCD.h>
#include <SoftwareSerial.h>

EzLCD3_SW lcd(10, 11); // create lcd object using pins 10 & 11

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.cls(); 


}

void loop(){
  int x = random(0,319); 
  int y = random(0,239);
  lcd.xy(x,y);
  lcd.color(random(1,200));
  lcd.arc(random(1,100), random(1,360), random(1,360) );
}
