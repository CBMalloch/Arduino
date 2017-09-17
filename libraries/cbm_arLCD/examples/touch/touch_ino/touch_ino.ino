/*
 * light.ino displays backlight control
 */
 
#include <ezLCD.h>
#include <SoftwareSerial.h>


EzLCD3_SW lcd( 10, 11 );

void setup()
{
  lcd.begin( EZM_BAUD_RATE ); 
  lcd.font(0); 
  lcd.cls(WHITE); // clear screen to white
  lcd.println("Touch the Screen");
}

void loop()
{

  int x = lcd.touchX();
  if(x > 0){
    lcd.print("X touch =  ");
    lcd.println(x);
  }
  int y = lcd.touchY();
  if(y > 0){
    lcd.print("Y touch =  ");
    lcd.println(y);
  }  
  delay(2000);
}

