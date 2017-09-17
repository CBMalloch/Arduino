#include <ezLCD.h>
#include <SoftwareSerial.h>

#define LED_PIN 13

EzLCD3_SW lcd(10, 11); // create lcd object using pins 10 & 11

void setup()
{
  Serial.begin(9600);
  lcd.begin( EZM_BAUD_RATE );
  lcd.fontw( 0, "sans24" );
  lcd.theme( 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 0 );
  
  pinMode( LED_PIN, OUTPUT );
  digitalWrite( LED_PIN, LOW );
}
  
void loop()
{
  lcd.cls(BLACK);
 
  int result = lcd.choice( "\"Got Milk\"", 0 );  
  Serial.println(result);  
  if(result == 1 )
     digitalWrite( LED_PIN, HIGH ); // turn LED on
  else if(result == 0 ) 
     digitalWrite( LED_PIN, HIGH ); // turn LED on

  delay(2000);
}

