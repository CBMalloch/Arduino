 
#include <ezLCD.h>
#include <SoftwareSerial.h>


EzLCD3_SW lcd( 10, 11 );

int xPos = 25;  // horizontal position
int yPos = 50;   // vertical position
int width = 250;
int height = 35;
int option = 1; // 1=draw horizontal, 2=horizontal disabled, 3=vertical,
                // 4=vertical disabled, 5=redraw horizontal,
                // 6=redraw horizontal disabled, 7=redraw vertical,
                // 8=redraw vertical disabled
int value = 0;
int max= 100;

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.fontw( 1, "0" );
  lcd.theme( 1, 155, 152, 3, 3, 1, 0, 1, 0, 0, 0 );
  lcd.cls(BLACK);
  lcd.color(WHITE);  
  lcd.string(1,"%");  //set string 1 to % for progress bar
  lcd.progressBar( 1, xPos, yPos, width, height, option, value,max, 1 ,1);
}


void loop()
{
  value = value + 10;
  if(value <= 100)
     lcd.wvalue(1, value);
  delay(500);
}

