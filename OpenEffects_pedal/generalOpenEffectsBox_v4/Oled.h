#ifndef Oled_h
#define Oled_h

#define Oled_VERSION "0.000.002"

#include <Arduino.h>
#include <Print.h>

#include <Audio.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

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