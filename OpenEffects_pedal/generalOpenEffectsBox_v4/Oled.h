#ifndef Oled_h
#define Oled_h

#define Oled_VERSION "0.000.002"

#include <Arduino.h>
#include <Print.h>

#include <Audio.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/*! \brief Wrapper for hardware interface -OLED-.

  The OpenEffects Project hardware includes a small OLED graphical display.
  
  I have abstracted some common display elements into this class, so that 
  any other class using the display doesn't have to redundantly define them.
  
  _NOTE:_ Sending information to the OLED is reasonably quick, but calling 
  display() ( which actually does the rendering and sends the data to the
  display controller ) takes close to a second. While this runs on the Teensy 
  and doesn't interrupt the flow of data through the i2s chip, it does affect
  the Teensy's control of some things ( such as tone sweeps ) and its ability 
  to react expeditiously to control inputs.
  
*/

class Oled : public Adafruit_SSD1306 {

  public:
    Oled ();  // constructor
    void setVerbose ( int verbose );
    
    #define _Oled_VERBOSE_DEFAULT 12
    void init ( int verbose = _Oled_VERBOSE_DEFAULT );
    
    void displayCommon ( int mode, int subMode );
    
    /*
    void display ();
  
    void clearDisplay();
    void setTextColor ( unsigned long color );
    
    void setTextSize ( int textSize );
    void setCursor ( int col, int row );
    // void print ( const char *str );
*/


  protected:
  
  private:
    int _verbose;
        
    // _oled itself is owned by and contained within Oled.cpp
    
    void displaySplash ();

};

#endif