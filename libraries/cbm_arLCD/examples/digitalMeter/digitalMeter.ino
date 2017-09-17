/*
 * digitalMeter.ino displays analog meter
 */
 
#include <ezLCD.h>
#include <SoftwareSerial.h>


EzLCD3_SW lcd( 10, 11 );

int xPos = 50;  // horizontal position
int yPos = 50;   // vertical position
int width = 100;
int height = 30;
int option = 14;  // 1=draw, 2=disabled, 3=ring, 4=accuracy
                  //  1=left, 2=disabled, 3=right, 4=center, 11=left framed, 12=disable framed,
                  //  13=right framed, 14=center framed, 6=redraw.
int digits = 3;  
int dp = 2;


void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.font( 0 );
  lcd.theme( 1, 155, 152, 3, 130, 0, 0, 1, 147, 153, 1 );
  lcd.cls( WHITE );
  lcd.color(BLACK);
  lcd.xy(30,85);
  lcd.print("ALog pin 0 volts");
  lcd.digitalMeter( 1, xPos,yPos, width, height, option, 0, digits, dp, 1);
}

void loop()
{
   float value = analogRead(0);
   float volts = (5.00 * value) / 1023.0;
   lcd.wvalue(1, value);
   delay(2000); //wait two seconds before updating   
}

