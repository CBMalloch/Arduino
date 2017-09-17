/*
 * touch.ino example for touch functions
 */

#include <ezLCD.h>
#include <SoftwareSerial.h>


EzLCD3_SW lcd( 10, 11 );

void setup()
{
  lcd.begin( EZM_BAUD_RATE ); 
  lcd.font(0); 
  lcd.cls(WHITE); // clear screen to white
  lcd.color(BLACK);
  lcd.println("Touch the Screen");
}

void loop()
{
  if(lcd.touchS() == 1) // if is touched
  {
    lcd.cls(WHITE); // clear screen to white
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
    lcd.xy(x,y);
    lcd.circle(25);
  }
}


