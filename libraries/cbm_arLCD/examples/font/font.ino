#include <ezLCD.h>
#include <SoftwareSerial.h>

EzLCD3_SW lcd(10, 11); // create lcd object using pins 10 & 11

const int nbrFonts = 10; 
char *fonts[] = {
   "kin", "sans","serif", "lcd", "blip", "core", "mac", "neur","olde", "squine"};
   
int nbrSizes[] = {4,4,4,4,2,2,2,2,2,2 }  ; 
char *sizes[] = { "72","48","36","24" }; 


char fontName[16];

void setup()
{
  lcd.begin( EZM_BAUD_RATE );
  lcd.cls(WHITE);  // clear screen to white
  lcd.color(BLACK);
}

void loop()
{
  // show the two predefined fonts:
  lcd.font(0); 
  lcd.println("This is font 0");   
  lcd.println("ABCDEFGHIJKlmnopqrst0123456789");
  lcd.println();
  lcd.font(1); 
  lcd.println("This is font 1");
  lcd.println("ABCDEFGHIJKlmnopqrst0123456789");
  delay(4000); 
  lcd.cls(WHITE); 
  
  // show the programmable (ezf file) fonts 
  for(int i=0; i < nbrFonts; i++)
  {
    for(int size = 0; size < nbrSizes[i]; size++ )
    {
      strcpy(fontName, fonts[i]);    // copy the base name
      strcat(fontName, sizes[size]); // append the size         
      lcd.font(fontName);       
      lcd.println(fontName);   
    } 
    delay(1000);
    lcd.cls(WHITE);
  }
}

