
#include <ezLCD.h>
#include <SoftwareSerial.h>

#define LED_PIN 13

EzLCD3_SW lcd( 10, 11 );

int xPos = 35; // horizontal position
int yPos = 50; // vertical position
int width = 250;
int height = 120;
int option = 4; // 1=left,2=disabled,3=right,4=center.

void setup()
{
  Serial.begin(9600);
  lcd.begin( EZM_BAUD_RATE );

  lcd.string( 1, "Left Align");
  lcd.string( 2, "Disabled");
  lcd.string( 3, "Right Align");
  lcd.string( 4, "Center Align");
  
  lcd.fontw( 1, "sans24" );
  lcd.theme( 1, 155, 152,  3, 3, 1, 0, 1, 0, 0, 0 );
  lcd.cls( 0 );
 
  lcd.groupBox(1, xPos, yPos, width, height, option,  1, 1 );
}

void loop()
{

}

