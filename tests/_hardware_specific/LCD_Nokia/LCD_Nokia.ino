/*
 * PCD8544 - Interface with Philips PCD8544 (or compatible) LCDs.
 *
 * Copyright (c) 2010 Carlos Rodrigues <cefrodrigues@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
/*

cbm notes:

I modded the PCD8544 library and the SD library according to 
http://arduino.cc/forum/index.php?topic=53843.0
in order to be able to run SD and the Nokia simultaneously
and harmoniously.

The mods consisted of slowing down SD from half speed to quarter speed to avoid outpacing
the Nokia, and modding PCD8544 to use hardware output rather than software output.

*/

/*
 * To use this sketch, connect the eight pins from your LCD thus:
 *
 * Pin 1 -> +3.3V (rightmost, when facing the display head-on)
 * Pin 2 -> Arduino digital pin 3
 * Pin 3 -> Arduino digital pin 4
 * Pin 4 -> Arduino digital pin 5
 * Pin 5 -> Arduino digital pin 7
 * Pin 6 -> Ground
 * Pin 7 -> 10uF capacitor -> Ground
 * Pin 8 -> Arduino digital pin 6
 *
 * Since these LCDs are +3.3V devices, you have to add extra components to
 * connect it to the digital pins of the Arduino (not necessary if you are
 * using a 3.3V variant of the Arduino, such as Sparkfun's Arduino Pro).
 
 * According to this web page <http://www.avdweb.nl/arduino/hardware-interfacing/nokia-5110-lcd.html>
 *
 *   one can drive the Nokia with 5V. Here is the setup:
 *
 *    Pin 1 -> +5V (rightmost, when facing the display head-on) (VCC)
 *    Pin 2 -> Ground
 *    Pin 3 -> 10uF capacitor (SCE - chip select)
 *    Pin 4 -> RST reset
 *    Pin 5 -> D/C data/command
 *    Pin 6 -> DN/MOSI master out slave in
 *    Pin 7 -> SCLK clock
 *    Pin 8 -> LED backlight

 */

// #include <SD.h>            // secure digital card for logging (SPI) 
#include <PCD8544.h>

#define BAUDRATE 115200

#define pdNOKIA_RST  7
#define pdNOKIA_DC   5
#define pdNOKIA_DN   4
#define pdNOKIA_SCLK 3
#define pdNOKIA_CS   6

// A custom glyph (a smiley)...
static const byte glyph[] = { B00010000, B00110100, B00110000, B00110100, B00010000 };

static PCD8544 lcd ( pdNOKIA_SCLK, pdNOKIA_DN, pdNOKIA_DC, pdNOKIA_RST, pdNOKIA_CS );


void setup() {
  /*
  #define pdDCS_SD      10
  pinMode(pdDCS_SD, OUTPUT);
  // see if the card is present and can be initialized:
  SD.begin(pdDCS_SD);
  */
  
  Serial.begin (BAUDRATE);
  
  Serial.println ("Ready");
  
  // PCD8544-compatible displays may have a different resolution...
  lcd.begin(84, 48);
  Serial.println (1);
  
  // Add the smiley to position "0" of the ASCII table...
  lcd.createChar(0, glyph);
  Serial.println (2);
  
  // findBestContrast(20, 100);
  Serial.println (3);
}


void loop() {
  // Just to show the program is alive...
  static int counter = 0;

  // Write a piece of text on the first line...
  lcd.setCursor(0, 0);
  lcd.print("Hello, World!");

  // Write the counter on the second line...
  lcd.setCursor(2, 1);
  lcd.print(counter, DEC);
  lcd.write(' ');
  lcd.write(0);  // write the smiley
  
  // Try random access
  lcd.setCursor(6*5,2);
  lcd.write('X');
  lcd.setCursor(6*5,3);
  lcd.print("XYZ");
  lcd.print(" ABC");

  delay(500);  
  counter++;
}

/*
void findBestContrast(int contrastMin, int contrastMax) {
  for(unsigned char contrast = contrastMin; contrast <= contrastMax; contrast++) {
    lcd.clear();
    lcd.setContrast(contrast);
    lcd.print ("Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor ");
    lcd.print (contrast);
    Serial.println (contrast);
    delay(100);
  }
}

*/

/* EOF - HelloWorld.ino */
