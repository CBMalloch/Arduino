 
#include <ezLCD.h>
#include <SoftwareSerial.h>

#define LED_PIN 13

EzLCD3_SW lcd( 10, 11 );

int xPos = 25;  // horizontal position
int yPos = 50;   // vertical position
int width = 250;
int height = 35;
int option = 5; // 1=draw horizontal, 2=horizontal disabled, 3=vertical,
                // 4=vertical disabled, 5=horizontal slider,
                // 6=horizontal slider disabled, 7=vertical slider,
                //  8=vertical slider disabled
int max= 500;
int resolution = 5;
int value = 200;


void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.fontw( 1, "sans24" );
  lcd.theme( 1, 9, 3, 0, 0, 0, 8, 8, 8, 1, 1 );
  lcd.cls(BLACK);
  lcd.color(WHITE);  
  lcd.slider( 1, xPos, yPos, width, height, option, max, resolution, value,1 );

  pinMode( LED_PIN, OUTPUT );
  digitalWrite( LED_PIN, LOW );
}

void loop()
{
  int rate = lcd.getWidgetValue(1); 
  blink(rate);
}

void blink(int rate)
{  
  digitalWrite( LED_BUILTIN, HIGH ); // turn LED on
  delay(rate);
  digitalWrite( LED_BUILTIN, LOW );  // turn LED off
  delay(rate);
}
