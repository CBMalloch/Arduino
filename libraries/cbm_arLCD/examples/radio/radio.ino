/*
 * radio.ino
 * radio buttons select the blink rate of an LED
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
  lcd.string( 1, "Off" );   // stringId 1
  lcd.string( 2, "Slow" );  // stringId 2
  lcd.string( 3, "Fast" );  // stringId 3
  lcd.radioButton( 1, xPos, yPos, width, height, 5, 1,1 );
  lcd.radioButton( 2, xPos, yPos + 50, width, height, 1, 1, 2 );
  lcd.radioButton( 3, xPos, yPos + 100, width, height, 1, 1, 3 );

  pinMode( LED_PIN, OUTPUT );
  digitalWrite( LED_PIN, LOW );
  Serial.println("ready");
}

int rate = 0; // blink delay, 0 is off 

int selected = 0;
int prevSelected = 0;

void loop()
{
  for(int i=1; i <= 3; i++)
  {   
    boolean checked = lcd.isChecked(i);
    if(checked )   // if radioButton 1 is checked
        selected = i; // store the selected widget
  }

  if(selected != prevSelected)  
  {
    Serial.println(selected);    
    prevSelected = selected;
    if(selected == 1)
      rate = 0; // LED is off
    else if(selected == 2)
      rate = 500; // LED blinks slow
    else if(selected == 3)
      rate = 200; // LED blinks fast    
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



