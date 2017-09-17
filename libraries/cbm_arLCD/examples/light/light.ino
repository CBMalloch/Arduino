/*
 * light.ino displays backlight control
 */
 
#include <ezLCD.h>
#include <SoftwareSerial.h>


EzLCD3_SW lcd( 10, 11 );

void setup()
{
  lcd.begin( EZM_BAUD_RATE ); 
  lcd.cls(WHITE); // clear screen to white
  lcd.color(BLACK);
  lcd.font(1); 
}

void loop()
{

  lcd.println("Backlight in steps of 10%");
  // set level in steps of 10%
  for(int i=0; i <= 100; i += 10)
  {
    lcd.light(i);  // set the backlight level  
    lcd.print("Backlight set to ");
    lcd.println(i);
    delay(2000); 
  }
  lcd.cls(WHITE); // clear screen to white
}

