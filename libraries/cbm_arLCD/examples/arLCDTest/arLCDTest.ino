/**
*  arLCD test code v1.0 4/16/2013
*  (c)2013 EarthLCD
*  ken@earthlcd.com
*
*  !!!!! make sure you have the line below in your startup.ezm !!!!!
*  cfgio 9 touch_int low quiet
*
*  This test code uses touch interrupt of the GPU to Signal touch events
*  Using Interrupts offloads the arduino from having to poll for touch events
*  
*/
#include <ezLCD.h>
#include <SoftwareSerial.h>

#define LED_PIN 13

EzLCD3_SW lcd( 10, 11 );
volatile boolean ezLCDInt = false; // flag to indicate interrupt
int result;
int x1Pos = 0; // horizontal position for buttton 1 
int x2Pos = 270; // horizontal position for buttton 2 
int yPos = 0; // vertical position of both buttons
int width = 50;
int height = 50;
int radius = 10;
int alignment = 0; // 0=centered, 1=right, 2=left, 3=bottom, 4=top 
int option = 1; // 1=draw, 2=disabled, 3=toggle pressed, 4=toggle not pressed,
// 5=toggle pressed disabled, 6=toggle not pressed disabled.
int L =50;
int temp =0;
unsigned long valueAD;
unsigned long valueAD2;
void setup()
{
  Serial.begin(9600);
  lcd.begin( EZM_BAUD_RATE );
  showMainScreen();
  pinMode( LED_PIN, OUTPUT );
  digitalWrite( LED_PIN, LOW );
  attachInterrupt(0, ezLCDevent, LOW);  

}
void showMainScreen( void ) {

  lcd.cls( 0 );
  lcd.color(WHITE);
  lcd.light(50);
  lcd.xy(55,0);
  lcd.printString("ar");
  lcd.color(RED);
  lcd.printString("L");
  lcd.color(GREEN);
  lcd.printString("C");
  lcd.color(BLUE);
  lcd.printString("D");
  lcd.color(WHITE);  
  lcd.printString(" Hardware Test V1.0");

  lcd.xy(65,225);
  lcd.font(1);
  lcd.printString("arLCD Firmware Version - ");
  lcd.xy(70,50);
  lcd.font(0);
  lcd.printString("Slider Adjust Brightness");
  lcd.xy(30,115);
  lcd.font(0);
//  lcd.printString("Progress Bar Connected To AN0");
  lcd.fontw( 2, "2" );
  lcd.fontw( 7, "1" );
  lcd.fontw( 6, "2");
//  Theme [ID][EmbossDkColor][EmbossLtColor][TextColor0][TextColor1][TextColorDisabled][Color0][Color1][ColorDisabled][CommonBkColor][Fontw].
//           A    B    C  D  E  F   G   H    I  J  K
  lcd.theme( 0,   1,   2, 0, 0, 0,  3,  3,   1, 0, 2);
  lcd.theme( 1, 155, 152, 3, 3, 3,  4,  24,   5, 0, 2);
  lcd.theme( 2,   5,  20, 3, 3, 3,  4,  24,   5, 0, 2);
  lcd.theme( 3,   9,   3, 0, 0, 0,  8,  68,   9, 0, 2);
  lcd.theme( 4,   7,   3, 0, 0, 0,  6,  40,   6, 6, 2);
  lcd.theme( 5, 126, 118, 3, 3, 3, 35, 35,  36, 0, 2);
  lcd.theme( 6, 111, 106, 3, 3, 3, 104, 107, 101, 0, 6);
  lcd.theme( 7,  58,  48, 3, 3, 3, 14, 14,  54, 0, 7);
 
  lcd.string( 1, "UL" ); // stringId 1
  lcd.string( 2, "UR" ); // stringId 2
  lcd.string( 3, "LL" ); // stringId 1
  lcd.string( 4, "LR" ); // stringId 2
  lcd.string( 5, "%" );
  lcd.string( 6,"Upgrade Firmware");
  lcd.string( 7,"Calibrate Touch");
  lcd.string( 8,"Continue");
  lcd.button( 1, x1Pos, yPos, width, height, option, alignment, radius, 1, 1 );
  lcd.button( 2, x2Pos, yPos, width, height, option, alignment, radius, 2, 2 );
  lcd.button( 3, x1Pos, yPos+190, width, height, option, alignment, radius, 1, 3 );
  lcd.button( 4, x2Pos, yPos+190, width, height, option, alignment, radius, 2, 4 );
  lcd.slider( 5, 20, 80, 280, 30, 5, 100, 1, 50, 0 );
  lcd.staticText(7, 200, 225, 60, 15, 1, 7, 66);
  lcd.button( 8, 60, 35, 200, 35, option, alignment, radius, 2, 6 ); 
//  lcd.progressBar( 8, 60, 140, 200, 35, option, (analogRead(0)/10),100, 6 ,5);
  lcd.button( 9, 60, 140, 200, 35, option, alignment, radius, 2, 7 );   
}
void printInfo( void ) {
        lcd.font(0);
        lcd.color(BLACK);
        lcd.fill(1);
        lcd.rect(115,200,120,20);
        lcd.xy(115,200);
        lcd.color(AQUA);
}
void loop()
{
  if(  ezLCDInt )                 //do nothing till ezLCDInt becomes true
  {
    ezLCDInt = false;             //reset ezLCDInt flag
    temp = lcd.wstack(LIFO);
//    Serial.println(temp,HEX);
    switch ( temp )   //switch based on last widget pressed
    {
      case 0x104:                 //widget #1 pressed
        printInfo();
        lcd.printString("UL Pressed");
        break;
      case 0x102:                 //widget #1 cancel
        printInfo();
        lcd.printString("UL Cancel");
        break;
      case 0x101:                 //widget #1 Released
        printInfo();
        lcd.printString("UL Released");
        break;
      case 0x204:
        printInfo();
        lcd.printString("UR Pressed");
        break;
      case 0x202:                 
        printInfo();
        lcd.printString("UR Cancel");
        break;
      case 0x201:
        printInfo();      
        lcd.printString("UR Released");
        break;
      case 0x304:
        printInfo();
        lcd.printString("LL Pressed");
        break;
      case 0x302:                 
        printInfo();
        lcd.printString("LL Cancel");
        break;
      case 0x301:
        printInfo();     
        lcd.printString("LL Released");
        break;
      case 0x404:
        printInfo();     
        lcd.printString("LR Pressed");
        break;
      case 0x402:                 
        printInfo();
        lcd.printString("LR Cancel");
        break;
      case 0x401:
        printInfo();   
        lcd.printString("LR Released");
        break;
      case 0x804:
        lcd.light(100);
        lcd.cls(BLACK);
        lcd.color(YELLOW);
        lcd.xy(60,0);
        lcd.printString("Firmware Upgrade Info");
        lcd.color(RED);
        lcd.xy(100,40);
        lcd.printString("!!!! Warning !!!!");
        lcd.color(YELLOW);
        lcd.xy(0,60);
        lcd.printString("   If you press YES on the next screen");
        lcd.xy(0,80);
        lcd.printString(" you will HAVE to upgrade the firmware");
        lcd.xy(0,100);
        lcd.printString("      Use ezLCD-3xx Firmware Loader");
        lcd.xy(0,120);
        lcd.color(BLUE);
        lcd.printString("           Download at earthlcd.com");
        lcd.wstack(CLEAR);
        lcd.button( 1, 85, 190, 150, 40, option, alignment, radius, 2, 8 );
        while(lcd.wstack(LIFO) != 0x104);
        lcd.cls(BLACK);
        result = lcd.choice( "\"Are You Sure ?\"", 1 );  
        Serial.println(result,DEC);
        if ( result == 1 )  lcd.ezLCDUpgrade(); 
        showMainScreen(); 
        lcd.wstack(CLEAR);       
        break;
      case 0x904:
        lcd.calibrate();
        showMainScreen(); 
        break;

      case 0x501 ... 0x503:                   //slider pressed 
        lcd.wstack(CLEAR); 
        L = lcd.getWidgetValue(5);  //get slider value
        lcd.light(L);               //set light to slider value
        break;
   
      default:
        lcd.wstack(CLEAR); 
        break;
    }     

  }
/*
  valueAD = 0;
  for(int temp=0;temp<500;temp++)
  valueAD += analogRead(0);
  valueAD = (valueAD/500); 
  if( valueAD2 != valueAD) lcd.wvalue(8, valueAD/10);
   valueAD2 = valueAD;
   */
}

/**
*  ezLCDevent we get here from a touch press event and all we do is set 
*  ezLCDInt flag true
*/

void ezLCDevent( void ) {
  ezLCDInt = true;
}

