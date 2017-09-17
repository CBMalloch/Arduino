#include <ezLCD.h>
#include <SoftwareSerial.h>

EzLCD3_SW lcd(10, 11); // create lcd object using pins 10 & 11

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.color(WHITE);
}
int angle = 10;

void loop()
{
  lcd.cls();
  lcd.xy(160,120);
  lcd.pie(50, 0,angle); 
  delay(1000);  
  angle = angle + 30;
  if(angle > 360)
    angle = 10;
  lcd.cls();      
}
