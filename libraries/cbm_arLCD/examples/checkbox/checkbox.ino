#include <ezLCD.h>
#include <SoftwareSerial.h>

EzLCD3_SW lcd(10, 11); // create lcd object using pins 10 & 11

int xPos = 30;  // horizontal position for widget 
int yPos = 30;   // vertical position for widget
int width = 225;
int height = 50;
int option = 1; // 1=draw unchecked, 2=disabled, 3=draw checked, 4=redraw


void setup()
{
  Serial.begin(9600);
  lcd.begin( EZM_BAUD_RATE );
  lcd.fontw( 1, "sans24" );
  lcd.cls(BLACK);
  lcd.string( 1, "Flash LED Faster" );   // stringId 1
  lcd.checkbox( 1, xPos, yPos, width, height, option, 1, 1 );
  
  pinMode(LED_BUILTIN, OUTPUT );
  digitalWrite( LED_BUILTIN, LOW );
}

int rate = 500; // blink delay 
  
void loop()
{  
  if( lcd.isChecked(1) )     // if checkbox 1 is checked
    rate = 200;  // reduce delay
 else  // if checkbox 1 is unchecked
    rate = 500;      
  blink(rate);
}

void blink(int rate)
{
  digitalWrite( LED_BUILTIN, HIGH ); // turn LED on
  delay(rate);
  digitalWrite( LED_BUILTIN, LOW );  // turn LED off
  delay(rate);
}