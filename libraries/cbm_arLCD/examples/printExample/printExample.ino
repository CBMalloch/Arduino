#include <ezLCD.h>
#include <SoftwareSerial.h>

EzLCD3_SW lcd( 10, 11 );

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.cls(BLACK);
  lcd.color(WHITE);
  lcd.font( 0 );
  lcd.println("hello, world!");
  lcd.write(65);
  lcd.println();
  lcd.println(65);
  lcd.println(65,DEC);
  lcd.println(65,HEX);
  lcd.println(65,OCT);
  lcd.println(65,BIN);
  lcd.println(3.14);
  lcd.println();
}

void loop() {}

