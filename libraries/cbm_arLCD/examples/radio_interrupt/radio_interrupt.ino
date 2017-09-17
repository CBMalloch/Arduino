/*
 * radio_interrupt.ino
 * radio buttons select the blink rate of an LED
 * interrupts are used to indicate button presses
 */

#include <ezLCD.h>
#include <SoftwareSerial.h>

#define LED_PIN 13

EzLCD3_SW lcd( 10, 11 );
volatile boolean ezLCDInt = false;

int xPos = 25;  // horizontal position
int yPos = 50;   // vertical position
int width = 250;
int height = 35;
int option = 5; // 0=remove, 1=draw, 2=disabled, 3=checked,
// 4=draw first unchecked, 5=draw first checked


void setup()
{
  Serial.begin(9600);
  lcd.begin( EZM_BAUD_RATE );
  lcd.fontw( 1, "sans24" );
  lcd.theme( 1, 9, 3, 0, 0, 0, 8, 8, 8, 1, 1 );
  lcd.cls(BLACK);
  lcd.color(WHITE);
  lcd.xy(20,30);
  lcd.string( 1, "STOP" );  // stringId 1
  lcd.string( 2, "Slow" );  // stringId 2
  lcd.string( 3, "Fast" );  // stringId 3
  lcd.radioButton( 1, xPos, yPos, width, height, 5, 1,1 );
  lcd.radioButton( 2, xPos, yPos + 50, width, height, 1, 1, 2 );
  lcd.radioButton( 3, xPos, yPos + 100, width, height, 1, 1, 3 );

  pinMode( LED_PIN, OUTPUT );
  digitalWrite( LED_PIN, LOW );
  attachInterrupt(0, ezLCDhandler, LOW);  
}

int rate = 0; // blink delay, 0 stops blinking 

int selected = 0;
int prevSelected = 0;

void loop()
{
  if(ezLCDInt) 
  {
    ezLCDInt = false;
    digitalWrite( LED_PIN, LOW );  // LED off
    if( lcd.isChecked(1) ) {     // if radioButton 1 is checked
      Serial.println("1 checked");
      rate = 0;  // stop
    }
    else if( lcd.isChecked(2) ) {    // if radioButton 2 checked       
      Serial.println("2 checked");
      rate = 500;  // slow
    }
    else if( lcd.isChecked(3) ) {   // if radioButton 3 is checked
      Serial.println("3 checked");
      rate = 200;  // fast
    }
  }
  blink();  
}

void blink()
{  
  digitalWrite( LED_PIN, HIGH ); // turn LED on
  delay(rate);
  digitalWrite( LED_PIN, LOW );  // turn LED off
  delay(rate);
}


void ezLCDhandler( void ) 
{
  ezLCDInt = true;
}


