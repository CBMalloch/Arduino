/** ezlcd3xx.h
 * Header for the ezlcd3xx Arduino library for ezLCD-3xx devices by EarthLCD.com
 * The library communicates with the ezLCD via a hardware and/or serial port
 */
 
/*
 * Created by Sergey Butylkov for EarthLCD.com
 * Arduino Library version by Michael Margolis
 *
 * Project home: http://arduino-ezlcd3xx.googlecode.com
 *
 * Copyright (C) 2012 Sergey Butylkov
 * Copyright (C) 2012 EarthLCD.com a dba of Earth Computer Technologies, Inc.
 * 
 * This work is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version. This work is distributed in the hope that it will be 
 * useful, but without any warranty; without even the implied warranty of 
 * merchantability or fitness for a particular purpose. See the GNU Lesser 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

  /* 
  * Initial release Feb 2012
  * Arduino Library version 12 Oct 2012
  */
    
#ifndef _EZLCD3XX_H
#define _EZLCD3XX_H

#define EZLCD_PROCESSING_PRIMITIVES // subset of Processing style functions

#include <SoftwareSerial.h>
#include "Print.h"

#define EZM_BAUD_RATE 38400  //defualt rate for coms link to GPU, this must match startup.ezm

class Stream;       /* Forward declaration of Stream to be used
                     * in the EzLCD3 abstract class */
                   
/* The following are preprocessor directives. The affect how the
 * the library code is being compiled. */


/**
 * Set to \c 1 to enable debugging the library over the hardware serial port.
 * When doing so, make sure you are using \c EzLCD3_SW class and not using
 * the hardware serial in any other way.
 * Set to \c 0 to disable serial debugging, which frees up the hardware serial
 * port and additional processing, and results in smaller code. Keep this at
 * \c 0 when you are using EzLCD3_HW class.
 */
#define EZLCD_SERIALDEBUG 0

/**
 * Set to \c 1 to globally declare numeric constants used by the \c EzLCD3
 * class. 
 * Set to \c 0 to declare the numeric constants as enums local to the \c EzLCD3
 * class
 */
#define EZLCD_GLOBALENUMS 1

/* The following are constants used by the EzLCD3 library. These can be
 * declared in a global scope (easier to use for novice developers, but can
 * cause problems in larger programs), or they can be limited to the scope of
 * the class (will require \c EzLCD3::CONSTANT syntax). See \c
 * EZLCD_GLOBALENUMS above. */

 /** Enum for standard colors */
enum { BLACK,GRAY,SILVER,WHITE,RED,MAROON,YELLOW,OLIVE,LIME,GREEN,AQUA,TEAL,BLUE,NAVY,FUCHSIA,PURPLE };

/** Enum values for orientation */
#define EZLCD_ENUM_ORIENTATION \
enum { HORIZONTAL=0, VERTICAL=1 };

/** Enum values for alignment */
#define EZLCD_ENUM_ALIGNMENT \
enum { CENTER=0x100, TOP=0x200, RIGHT=0x400, BOTTOM=0x800, LEFT=0x1000 };

/** Enum values for convenient combined alignment */
#define EZLCD_ENUM_COMBALIGN \
enum { LEFTTOP = TOP | LEFT, RIGHTTOP = TOP | RIGHT };             \
enum { TOPLEFT = LEFTTOP, TOPRIGHT = RIGHTTOP };                   \
enum { LEFTBOTTOM = BOTTOM | LEFT, RIGHTBOTTOM = BOTTOM | RIGHT }; \
enum { BOTTOMLEFT = LEFTBOTTOM, BOTTOMRIGHT = RIGHTBOTTOM };

/** Enum for widget and picture options */
//#define EZLCD_ENUM_WIDGETS \
//enum { DRAW=0x1, DISABLED=0x4, PRESSED=0x2000, TOGGLE=0x8 };       \
//enum { REMOVE=0x10, REDRAW=0x20, HDISABLED=0x40, VDISABLED=0x80 }; \
//enum { CHECKED=0x2000, SELECTED=0x2000, FIRST=0x4000 };            \
//enum { FIRSTANDSELECTED = FIRST | SELECTED };                      \
//enum { RING=0x2000, ACCURACY=0x4000 };                             \
//enum { CLOCKWISE=0x2000, COUNTERCLOCKWISE=0x4000 };                \
//enum { NONE=0, DOWNSIZE=0x1, BOTH = DOWNSIZE | CENTER };           \

#define EZLCD_ENUM_WIDGETS \
enum { NONE=0, DOWNSIZE=0x1, BOTH = DOWNSIZE | CENTER };  


/** Enum for choice */
#define EZLCD_ENUM_CHOICE \
enum { YES=1, NO=0, CANCEL=-1 };

/* Declares enums in the global scope when \c EZLCD_GLOBALENUMS is \c 1 */
#if EZLCD_GLOBALENUMS
EZLCD_ENUM_ORIENTATION
EZLCD_ENUM_ALIGNMENT
EZLCD_ENUM_COMBALIGN
EZLCD_ENUM_WIDGETS
EZLCD_ENUM_CHOICE
#endif

#define FIFO 0
#define LIFO 1
#define CLEAR 2

/**
 * Communicates with the Arduino via software and/or hardware serial port. This
 * is a virtual class so the user cannot instantiate it. Instead the user will
 * instantiate an \c EzLCD3_HW or \c EZLCD3xx_SW classes. See the end of this
 * header file.
 */
class EzLCD3 : public Print
{   
public:  
    /** Class constructor */
    EzLCD3();

    /** Class destructor */
    ~EzLCD3();
	
	void debugWrite(char b);  // DEBUG ONLY, NOT FOR RELEASE


    /**
     * Initialize communication at the specified baud rate and wait for ezLCD
     * to get ready to accept commands. Implementation depends on hardware vs
     * software serial.
     * \param[in]  baud    Baud rate
     */
    virtual void begin( long baud )=0;
    /* TODO: Specify number of times ping is attempted */

	/* required for classes derived from Print
	*/
	 virtual size_t write(uint8_t);

    /**
     * Reset ezLCD and re-establish communication.
     */
    void reset();

    /**
     * Clear screen to a specific color id and clear widgets.
     * \param[in] id    Numeric value for the color. 0 (black) by default.
     */
    void cls( int id=0 );

    /**
     * Clear screen to a specific color and clear widgets.
     * \param[in] color    Null-terminated string describing color.
     */
    void cls( const char *color );

    /**
     * turn off widget output used for polling and interrupts .
     * 
     */
    void wquiet(void);
    /**
     * Gets one value off widget stack.
     * \param[in] cmd = FIFO will return in first in first out order
     * \param[in] cmd = LIFO will return in last in first out order
     * \param[in] cmd = CLEAR will clear the stack
     * \retval value one unsigned int off stack \n
     * high byte will be widget ID low byte status\n
     * \c lcd.wstack(LIFO) will return the last widget pushed and state 
     */
    unsigned int wstack( int cmd );

    /**
     * Returns whether the class was configured for echo
     * \retval  true     The class is configured for echo.
     * \retval  false    The class is configured for NO echo
     */
    bool echo() { return m_echo; }

    /**
     * Enable or disable echo
     * \param[in]  val    \c true to enable echo. \c false to disable echo.
     */
    void echo( bool val );
    
    void ezLCDUpgrade( void );

    /**
    * Return current brightness setting.
    * \return    Brightness in 0-100.
    */
    int light();

    /**
     * Set brightness.
     * \param[in]  brightness    Brightness in 0-100.
     */
     void light( int brightness );
	 
   /**
     * Set brightness & timeout
     * \param[in]  brightness Brightness in 0-100.
	 * \param[in]  timeout    Timeout value in minutes before dimming
     */
     void light( int brightness, unsigned long timeout );
    

	/**
     * Set brightness, timeout, dimmed brightness
     * \param[in]  brightness Brightness in 0-100.
	 * \param[in]  timeout    Timeout value in minutes before dimming
     * \param[in]  dimmed     Dimmed brighness level in 0-100
     */
	 void light( int brightness, unsigned long timeout, int dimmed );	 

	 

    /**
     * Sets the current color by color ID.
     * \param[in]  id    Numeric color ID.
     */
    void color( int id );

    /**
     * Return current color.
     * \param[out]  red    Red value in 0-255.
     * \param[out]  green  Green value in 0-255.
     * \param[out]  blue   Blue value in 0-255.
     */
    void color( uint8_t *red, uint8_t *green, uint8_t *blue );
    
    /**
     * Sets a color index to a specific RGB value
     * \param[in]  id    Numeric color ID.
     * \param[in]  red   Red value in 0-255.
     * \param[in]  green Green value in 0-255.
     * \param[in]  blue  Blue value in 0-255.
     */
    void colorId( int id, uint8_t red, uint8_t green, uint8_t blue );

    /**
     * Get RGB value of a color index
     * \param[in]  id    Numeric color ID.
     * \param[out] red   Red value in 0-255.
     * \param[out] green Green value in 0-255.
     * \param[out] blue  Blue value in 0-255.
     */
    void colorId( int id, uint8_t *red, uint8_t *green, uint8_t *blue );

    /**
     * Set current font to an internal factory font.
     * \param[in]  id    Factory font index. Currently 0 to 5. Default is 0,
     *                   default medium font.
     */ 
    void font( int id=0 );

    /**
     * Set current font to a programmable font (ezf file) from flash drive
     * \param[in]  fontname    Font filename on the ezLCD filesystem.
     */
    void font( const char* fontname );

    /**
     * Set font index (used by themes) to a programmable font (ezf file) 
     * from flash drive.
     * \param[in]  id        Font ID.
     * \param[in]  fontname  Font filename on the ezLCD filesystem.
     */
    void fontw( int id, const char* fontname );

    /**
     * Set current font orientation to horizontal or vertical
     * \param[in]  orientation    \c HORIZONTAL for horizontal orientation.
     *                            \c VERTICAL for vertical orientation.
     */
    void fonto( int orientation );

    /**
     * Returns current font orientation.
     * \retval  HORIZONTAL    Horizontal orientation.
     * \retval  VERTICAL      Vertical orientation.
     */
    int fonto();

    /**
     * Invoke a touch screen calibrate.
     */
    void calibrate();

    /** Set current line width
     * \param[in]  width    Line width in pixels. Can be 1 or 3 pixels.
     */
    void lineWidth( int width );

    /**
     * Returns current line width
     * \return    Line width in pixels. 1 or 3 pixels.
     */
    int lineWidth();

    /**
     * Set line type to solid, dot or dash.
     * \param[in]  type    \c 0 is solid, increasing number increases spacing
     *                     between dots.
     */
    void lineType( int type );

    /**
     * Return current line type.
     * \return    \c 0 when solid, higher number for dash and even higher for
     *            dot.
     */
    int lineTtype();

    /**
     * Return the maximum allowed x-coordinate.
     * \return    Maximum allowed x-coordinate in pixels.
     */
    uint16_t xmax();

    /**
     * Return the screen width.
     * \return    Screen width in pixels.
     */
    inline uint16_t width();

    /**
     * Return the maximum allowed y-coordinate.
     * \return    Maximum allowed y-coordinate in pixels.
     */
    uint16_t ymax();

    /**
     * Return the screen height.
     * \return    Screen height in pixels.
     */
    inline uint16_t height();

    /**
     * Set the drawing cursor to location x,y on screen.
     * \param[in]  x    x-coordinate in pixels.
     * \param[in]  y    y-coordinate in pixels.
     */
    void xy( uint16_t x, uint16_t y );

    /**
     * Set the drawing cursor to a preset aligned location.
     * \param[in]  align    Alignment. Allowed values are \c LEFTTOP, 
     *                      \c TOPLEFT, \c TOP, \c RIGHTTOP, \c TOPRIGHT,
     *                      \c LEFT, \c CENTER, \c RIGHT, \c LEFTBOTTOM,
     *                      \c BOTTOMLEFT, \c BOTTOM, \c RIGHTBOTTOM, 
     *                      \c BOTTOMRIGHT
     */
    void xyAligned( uint32_t align );

    /**
     * Return current x,y drawing location.
     * \param[out]  x    x-coordinate in pixels.
     * \param[out]  y    y-coordinate in pixels.
     */
    void xyGet( uint16_t *x, uint16_t *y );

    /**
     * Store current x and y into x,y array on ezLCD.
     * \param[in]  id    Index where x and y is stored.
     */
    void xy_store( int id );

    /**
     * Restore current x and y from the x,y array on ezLCD.
     * \param[in]  id    Index from which x and y are restored.
     */
    void xy_restore( int id );

    /**
     * Store string at an index in the string array on ezLCD.
     * \param[in]  id    String ID at which to store.
     * \param[in]  str   Null-terminated string to store.
     */
    void string( int id, const char *str );

    /**
     * Get pixel value at x y
     */
    uint16_t getPixel( uint16_t x, uint16_t y );
    /**
     * Draw a pixel at the current x and y with the current color.
     */
    void plot();

    /**
     * Draw a pixel at a specified x,y with current color.
     * \param[in]  x    x-coordinate in pixels.
     * \param[in]  y    y-coordinate in pixels.
     */
    void plot( uint16_t x, uint16_t y );

    /**
     * Draw a line from the current x,y to the specified x,y
     * \param[in]   x    x-coordinate in pixels to draw the line to.
     * \param[in]   y    y-coordinate in pixels to draw the line to.
     */
    void line( uint16_t x, uint16_t y );

    /**
     * Draw a box from the current x,y with specified width, height, and fill.
     * \param[in]   width    Box width in pixels.
     * \param[in]   height   Box height in pixels.
     * \param[in]   fill     Set to \c true to fill the solid with box. Default
     *                       is \c false which only draws the box outline.
     */
    void box( uint16_t width, uint16_t height, bool fill=false );

    /**
     * Draw a circle at current x,y with the specified radius and fill.
     * \param[in]  radius    Circle radius in pixels.
     * \param[in]  fill      Set to \c true to fill the circle solid with color.
     *                       Default is \c false which only draws the circle
     *                       outline
     */
    void circle( uint16_t radius, bool fill=false );

    /**
     * Draw an arc with the specified radius, start angle and end angle.
     * \param[in]  radius    Arc radius in pixels.
     * \param[in]  start     Start angle in degrees.
     * \param[in]  end       End angle in degrees.
     */
    void arc( uint16_t radius, int16_t start, int16_t end );

    /**
     * Draw a pie with the specified radius, start angle and end angle.
     * \param[in]  radius    Arc radius in pixels.
     * \param[in]  start     Start angle in degrees.
     * \param[in]  end       End angle in degrees.
     */
    void pie( uint16_t radius, int16_t start, int16_t end );

    /**
     * Draw a picture by id on ezLCD. Same as \c image() function.
     * \param[in]  id       ID of the picture to display.
     * \param[in]  x        x-coordinate in pixels of where to draw.
     * \param[in]  y        y-coordinate in pixels of where to draw.
     * \param[in]  option    option.
     */
    void picture( int id, uint16_t x, uint16_t y, uint16_t option=0 );

    /**
     * Draw a picture from file on the ezLCD. Same as \c image() function.
     * \param[in]  filename  Filename of the picture to display. Must include
     *                       extension.
     * \param[in]  x         x-coordinate in pixels of where to draw.
     * \param[in]  y         y-coordinate in pixels of where to draw.
     * \param[in]  option    option.
     */
    void picture( const char *filename, uint16_t x, uint16_t y,
                  uint16_t option=0 );

    /**
     * Draw an image by id on ezLCD. Same as \c picture() function.
     * \param[in]  id       ID of the image to display.
     * \param[in]  x        x-coordinate in pixels of where to draw.
     * \param[in]  y        y-coordinate in pixels of where to draw.
     * \param[in]  option   option 
     */
    void image( int id, uint16_t x, uint16_t y, uint16_t option=0 );

    /**
     * Draw an image from file on the ezLCD. Same as \c picture() function.
     * \param[in]  filename  Filename of the image to display. Must include
     *                       extension.
     * \param[in]  x         x-coordinate in pixels of where to draw.
     * \param[in]  y         y-coordinate in pixels of where to draw.
     * \param[in]  option    option .
     */
    void image( const char *filename, uint16_t x, uint16_t y,
                uint16_t option=0 );

// Adds defines for graphics primitives similar to Processing and GLCD
#ifdef EZLCD_PROCESSING_PRIMITIVES
    /**
     * Graphics primitives similar to Processing and GLCD.\n
     * \param[in] isFilled set true filled boxes and circles
     */
	void fill(bool isFilled);
    /**
     * Graphics primitives similar to Processing and GLCD.\n
     * Draw a pixel at the current x and y with the current color.
     * \param[in] x     x-coordinate in pixels
     * \param[in] y     y-coordinate in pixels
     */
	void point(int x, int y);
    /**
     * Graphics primitives similar to Processing and GLCD.\n
     * Draw a line from x1 y1 to x2 y2 with the current color.
     * \param[in] x1     x1-coordinate in pixels
     * \param[in] y1     y1-coordinate in pixels
     * \param[in] x2     x2-coordinate in pixels
     * \param[in] y2     y2-coordinate in pixels     
     */
	void line(int x1, int y1, int x2, int y2);

	// rectangle with mode as corner 
    /**
     * Graphics primitives similar to Processing and GLCD.\n
     * Draw a rectangle at x y of width and height with the current color.\n
     * And filled if fill is set true
     * \param[in] x     x-coordinate in pixels
     * \param[in] y     y-coordinate in pixels
     * \param[in] width  width of rect in pixels
     * \param[in] height height of rect in pixels
     */
	void rect(int x,int y,int width,int height);

	// draws circle with mode as center
    /**
     * Graphics primitives similar to Processing and GLCD.\n
     */
	void ellipse(int x,int y,int diameter);

	// draws circle with mode as center
	// height argument is ignored
    /**
     * Graphics primitives similar to Processing and GLCD.\n
     */
	void ellipse(int x,int y,int width,int height);
    /**
     * Graphics primitives similar to Processing and GLCD.\n
     * Draw an arc with the specified radius, start angle and end angle at x y.
     * \param[in]  x         X
     * \param[in]  y         Y
     * \param[in]  diameter  diameter of arc
     * \param[in]  start     Start angle in degrees.
     * \param[in]  stop      End angle in degrees.
     */
	// start and stop are in degrees (Processing uses radians)
	void arc(int x,int y,int diameter, int start,int stop); 
#endif

    /**
     * Enable or disable the clip area.
     * \param[in]  enable    \c true to enable, \c false to disable.
     */
    void clipenable( bool enable );

    /**
     * Set the clip area to protect the surrounding area from change. The
     * parameters are limiting coordinates of the clip area.
     * \param[in]  left    Left edge of the clip area in pixels.
     * \param[in]  top     Top edge of the clip area in pixels.
     * \param[in]  right   Right edge of the clip area in pixels.
     * \param[in]  bottom  Bottom edge of the clip area in pixels.
     */
    void cliparea( uint16_t left, uint16_t top, 
                   uint16_t right, uint16_t bottom );

    /**
     * Print a string from the string array on ezLCD by id.
     * \param[in]  id      String ID.
     * \param[in]  alignment   Text alignment. Allowed values are \c LEFTTOP, 
     *                     \c TOPLEFT, \c TOP, \c RIGHTTOP, \c TOPRIGHT,
     *                     \c LEFT, \c CENTER, \c RIGHT, \c LEFTBOTTOM,
     *                     \c BOTTOMLEFT, \c BOTTOM, \c RIGHTBOTTOM, 
     *                     \c BOTTOMRIGHT, or \c NONE.
     */
    void printStringId( int id, uint32_t alignment=0 ); // was print

    /**
     * Print string to the display at the current x,y with given alignment.
     * \param[in]  str     Null-terminated string to print.
     * \param[in]  alignment   Text alignment. Allowed values are \c LEFTTOP, 
     *                     \c TOPLEFT, \c TOP, \c RIGHTTOP, \c TOPRIGHT,
     *                     \c LEFT, \c CENTER, \c RIGHT, \c LEFTBOTTOM,
     *                     \c BOTTOMLEFT, \c BOTTOM, \c RIGHTBOTTOM, 
     *                     \c BOTTOMRIGHT, or \c NONE.
     */
    void printAligned( const char *str, uint32_t alignment=0 ); // was print

	/**
     * Print string direct to LCD skipping Arduino filtering faster
  	 * set xy before printing 
     * \param[in]  str     Null-terminated string to print.
	 */
    void printString( char *str );

    /**
     * Wait for touch.
     * \param[in]  timeout    Timeout value in milliseconds before we give up
     *                        on waiting. Default is very long.
     */
    void waitTouch( unsigned long timeout=(unsigned long)-1 );

    /**
     * Wait for release of touch.
     * \param[in]  timeout    Timeout value in milliseconds before we give up
     *                        on waiting. Default is very long.
     */
    void waitNoTouch( unsigned long timeout=(unsigned long)-1 );


    /**
     * Show a YES, NO and CANCEL prompt, wait for input and return the result.
     * \param[in]  str     String to display in the prompt
     * \param[in]  theme   Theme to use for drawing the prompt.
     * \param[in]  timeout Timeout value in milliseconds before giving up on
     *                     waiting. Default is very long.
     * \retval     YES     User pressed Yes.
     * \retval     NO      User pressed No.
     * \retval     CANCEL  User pressed Cancel.
     */
    int choice( const char *str, int theme,
                unsigned long timeout = (unsigned long)-1 );

    /**
     * Set up a widget theme
     * \param[in] index             Theme Index
     * \param[in] embossDkColor     Dark Emboss color used for 3-D effect of
     *                              objects.
     * \param[in] embossLtColor     Light Emboss color used for 3-D effect of
     *                              objects .
     * \param[in] textColor0        For text, Useage may vary from one widget
     *                              to another, whether the widget is in focus or
     *                              not.
     * \param[in] textColor1        For text when pressed. Useage may vary from
     *                              one widget to another.
     * \param[in] textColorDisabled Color of objects that are disabled.
     * \param[in] color0            For objects. Usage may vary from one object
     *                              to another, whether the widget is in focus or
     *                              not.
     * \param[in] color1            For objects when pressed. Useage may vary 
     *                              from one object to another.
     * \param[in] colorDisabled     Used to render objects that are disabled.
     * \param[in] commonBkColor     Used to hide objects from screen but still
     *                              active.
     * \param[in] fontw             Font id associated with this theme.
     */
    void theme( int index, int embossDkColor, int embossLtColor,
                int textColor0, int textColor1, int textColorDisabled,
                int color0, int color1, int colorDisabled,
                int commonBkColor, int fontw );

    /**
     * Draw/alter a button widget.
     * \param[in]  id        Widget ID to assign.
     * \param[in]  theme     Theme ID to use.
     * \param[in]  strid     String ID to use for text.
     * \param[in]  x         Starting x-coordinate in pixels.
     * \param[in]  y         Starting y-coordinate in pixels.
     * \param[in]  width     Width in pixels.
     * \param[in]  height    Height in pixels.
     * \param[in]  radius    Controls roundness of the button edges.
     *                       Radius of \c 0 (default) means button corners are
     *                       perfect right angles. Radius of half the size of
     *                       the button results in an edge that is round.
     * \param[in]  align     Text alignment. Allowed values are \c CENTER
     *                       (default), \c LEFT, \c RIGHT or \c BOTTOM
     * \param[in]  option    Option. 
     */
    void button(  int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                uint16_t option, uint16_t align, uint16_t radius, int theme, int strid);
    /**
     * Draw/alter a touchZone widget.
     * \param[in]  id        Widget ID to assign.
     * \param[in]  x         Starting x-coordinate in pixels.
     * \param[in]  y         Starting y-coordinate in pixels.
     * \param[in]  width     Width in pixels.
     * \param[in]  height    Height in pixels.
     * \param[in]  option    Option. 
     */
    void touchZone(  int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                uint16_t option );
    /**
     * Draw/alter a checkbox.
     * \param[in]  id        Widget ID to assign.
     * \param[in]  theme     Theme ID to use.
     * \param[in]  strid     String ID to use for text.
     * \param[in]  x         Starting x-coordinate in pixels.
     * \param[in]  y         Starting y-coordinate in pixels.
     * \param[in]  width     Width in pixels.
     * \param[in]  height    Height in pixels.
     * \param[in]  option    Option.
     */
     
     
    void checkbox( int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                 uint16_t option, int theme, int strid );
                   
    /** Draw/alter a radio button widget.
     * \param[in]  id        Widget ID to assign.
     * \param[in]  theme     Theme ID to use.
     * \param[in]  strid     String ID to use for text.
     * \param[in]  x         Starting x-coordinate in pixels.
     * \param[in]  y         Starting y-coordinate in pixels.
     * \param[in]  width     Width in pixels.
     * \param[in]  height    Height in pixels.
     * \param[in]  option    Option:
     */
    void radioButton( int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                 uint16_t option, int theme, int strid );

    /**
     * Create/alter a groupox.
     * \param[in]  id        Widget ID to assign.
     * \param[in]  x         Starting x-coordinate in pixels.
     * \param[in]  y         Starting y-coordinate in pixels.
     * \param[in]  width     Width in pixels.
     * \param[in]  height    Height in pixels.
     * \param[in]  option    Option
	 * \param[in]  theme     Theme ID to use.
	 * \param[in]  strid     String ID to use for text.

     */
    void groupBox(  int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                      uint16_t option, int theme, int strid );
                   
    /**
     * Creates/alter a progress widget.
     * \param[in]  id          Widget ID to assign.
     * \param[in]  theme       Theme ID to use.
     * \param[in]  x           Starting x-coordinate in pixels.
     * \param[in]  y           Starting y-coordinate in pixels.
     * \param[in]  width       Width in pixels.
     * \param[in]  height      Height in pixels.
     * \param[in]  initial     Initial numeric reading of the progress bar.
     *                         Default is \c 0.
     * \param[in]  range       Maximum allowed value of the progress bar.
     *                         Default is \c 100.
     * \param[in]  option      Option
     * \param[in]  suffix      Char to display at end of number text % default
     */
    void progressBar( int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                   uint16_t option, int initial, int range,  int theme, int suffix);

    /**
     * Create/alter a static text widget.
     * \param[in]  id        Widget ID to assign.
     * \param[in]  theme     Theme ID to use.
     * \param[in]  strid     String ID to use for text.
     * \param[in]  x         Starting x-coordinate in pixels.
     * \param[in]  y         Starting y-coordinate in pixels.
     * \param[in]  width     Width in pixels.
     * \param[in]  height    Height in pixels.
     * \param[in]  option    Option

     */
    void staticText(  int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                            uint32_t option, int theme, int strid );
    /* TODO: static_value() widget? */

    /**
     * Creates/alter a digital meter widget.
     * \param[in]  id        Widget ID to assign.
     * \param[in]  theme     Theme ID to use.
     * \param[in]  x         Starting x-coordinate in pixels.
     * \param[in]  y         Starting y-coordinate in pixels.
     * \param[in]  width     Width in pixels.
     * \param[in]  height    Height in pixels.
     * \param[in]  initial   Initial numeric value of the meter. 
     * \param[in]  digits    Total number of digits. 
     * \param[in]  dotpos    Dot position counting digits from the left. 
     * \param[in]  option    Option

     */
    void digitalMeter( int id, uint16_t x, uint16_t y, uint16_t width,
                      uint16_t height, uint16_t option, int initial,
					  int digits, int dotpos,int theme );
					  
	
    /**
     * Creates/alter an analog meter widget.
     * \param[in]  id        Widget ID to assign.
     * \param[in]  theme     Theme ID to use.
     * \param[in]  strid     String ID to use for text.
     * \param[in]  x         Starting x-coordinate in pixels.
     * \param[in]  y         Starting y-coordinate in pixels.
     * \param[in]  width     Width in pixels.
     * \param[in]  height    Height in pixels.
     * \param[in]  initial   Initial numeric value of the meter. 
     * \param[in]  min       Minimum reading of the meter. 
     * \param[in]  max       Maximum reading of the meter. 
     * \param[in]  option    Option. 
     * \param[in]  type      type.
     */
    void analogMeter( int id, uint16_t x, uint16_t y, uint16_t width,
                    	uint16_t height, uint16_t option, int initial,
						int min, int max, int theme, int strid, int type );

    /**
     * Create/alter a dial widget.
     * \param[in]  id         Widget ID to assign.
     * \param[in]  theme      Theme ID to use.
     * \param[in]  x          Starting x-coordinate in pixels.
     * \param[in]  y          Starting y-coordinate in pixels.
     * \param[in]  radius     Radius of the dial in pixels.
     * \param[in]  resolution Resolution of the dial in degrees.
     * \param[in]  initial    Initial numeric position of the dial. 
     * \param[in]  max        Maximum numeric position of the dial. 
     * \param[in]  option     1=draw, 2=disabled, 3=ring, 4=accuracy
     */
    void dial( int id, uint16_t x, uint16_t y, uint16_t radius, uint16_t option,
             	int resolution, int initial, int max, int theme );

    /**
     * Ceate/alter a slider widget.
     * \param[in]  id          Widget ID to assign.
     * \param[in]  theme       Theme ID to use.
     * \param[in]  x           Starting x-coordinate in pixels.
     * \param[in]  y           Starting y-coordinate in pixels.
     * \param[in]  width       Width in pixels.
     * \param[in]  height      Height in pixels.
     * \param[in]  resolution  per step
     * \param[in]  initial     Initial numeric value of the slider. 
     * \param[in]  max         Maximum numeric value of the slider. 
     * \param[in]  option      Option
     */
    void slider( int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                 uint16_t option, int max, int resolution, int initial, int theme);

    /**
     * Set value for an analog meter widget.
     * \param[in]  id    Widget ID.
     * \param[in]  color1 color1
     * \param[in]  color2 color2
     * \param[in]  color3 color3
     * \param[in]  color4 color4
     * \param[in]  color5 color5
     * \param[in]  color6 color6
     */

    void analogMeterColor(  int id, int color1, int color2, int color3, int color4, int color5, int color6 );
    
    /**
     * Set value for a widget.
     * \param[in]  id    Widget ID.
     * \param[in]  value Numeric value to set.
     */
    void wvalue( int id, int value );
    void wvalue( int id, const char *str );  // added 2013/12/01 cbm

    
	 /**
      * return true if the button with the given id is pressed 
      * \param[in]  id     Widget ID of the button you want to check.
      * \retval     true   ezLCD signaled that the button is pressed.
      * \retval     false  not pressed
      *                    return is only meaningful if id is for a valid checkbox
	  */
	bool isPressed( int id );
   

	 /**
      * return true if the checkbox or radio button with the given id is checked 
      * \param[in]  id     Widget ID of the checkbox you want to check.
      * \retval     true   ezLCD signaled that the checkbox was checked.
      * \retval     false  not checked 
      *                    return is only meaningful if id is for a valid checkbox
      */
    bool isChecked( int id );
    
    /**
     * Get the state of the widget with the given id
     * Used to get the value of widget 
     * \param[in]  id    Widget ID you want to check .
     * \retval     value of widget
     */    
    unsigned int wstate( int id );
    /**
     * Get the value of the widget with the given id
     * Used to get the value of a slider or dial
     * \param[in]  id    Widget ID you want to check (slider or dial).
     * \return           returns the numeric value for the widget.
     * \retval     -1    There is no information of the slider with the
     *                   specified ID having it's position changed.
     */
    int getWidgetValue( int id );
	
	/**
     * Return the touch status
  	 * 0 = not currently pressed, 3= pressed, 4 = released
	 */	 
	int touchS();
	
	
	/**
     * Return the x (horizontal) coordinate
  	 * of the last location where the screen was touched.
	 */
	int touchX();
	
	/**
     * Return the y (vertical) coordinate
  	 * of the last location where the screen was touched.
	 */
	int touchY();

#ifdef NOT_SUPPORTED

    /**
     * Record a macro to the internal flash drive.
     * \param[in]  name    Filename for the macro. Specifying the extension is 
     *                     NOT necessary.
     */
    void record( const char *name );

    /**
     * Play a macro.
     * \param[in]  name    Filename for the macro. Specifying the extension is 
     *                     NOT necessary.
     */
    void play( const char *name );

    /**
     * Stop playing a macro
     */
    void stop();

    /**
     * Pause macro exection for the specified amount of time.
     * \param[in]  msec    Delay in milliseconds.
     */
    void pause( int msec );

    /**
     * Enable or disable looping of a macro.
     * \param[in]  val    \c true to enable looping. \c false to disable
     *                    looping.
     */
    void loop( bool val );

    /**
     * Set delay between processing each line of a macro.
     * \param[in]  msec    Delay in milliseconds.
     */
    void speed( int msec );

    /**
     * Configure a general purpose IO pin (1-9) on the ezLCD for input or
     * output.
     * \param[in]  port    Port number on the ezLCD.
     * \param[in]  type    \c INPUT for input and \c OUTPUT for outut.
     */
    void cfgio( uint8_t port, uint8_t type );

    /**
     * Write data to a general purpose IO pin (1-9). The pin must be configured
     * for output.
     * \param[in]  port    Port number on the ezLCD.
     * \param[in]  data    \c 0 to set the pin to low. \c 1 to set the pin high.
     */
    void io( uint8_t port, int data );

    /**
     * Read data from a general purpose IO pin (1-9). The pin must be configured
     * for input
     * \param[in]  port    Port number on the ezLCD.
     * \retval     0       Pin is low.
     * \retval     1       Pin is high.
     * \retval     -1      Failure.
     */
    int io( uint8_t port );
#endif	
	
	
    /**
     * Numerical values for the EarthSEMPL commands. Provided here for
     * users who wish to compose EarthSEMPL commands manually. (This is a low-
     * level asset that is not required for the common uses of the device)
     */    
     enum Commands {
         Command=             0,     /**< Direct command. */
         Status=              1,
         Clr_Screen=          2,     /**< Clear to provided color. */
         Ping=                3,     /**< Return Pong */
         zBeep=               4,     /**< Beep provided duration 
                                      *(frequency fixed) */
         Light=               5,     /**< \c 0 (off) to \c 100 (on) */
         Color=               6,
         eColor_ID=           7,

         Font=                10,    /**< Font number. */
         Fontw=               11,    /**< Font number widget. */
         Font_Orient=         12,    /**< Horizontal or vertical. */
         Line_Width=          13,    /**< 1 or 3. */
         Line_Type=           14,    /**< 1=dot dot 2=dash dash. */
         eXY=                 15,    /**< X and Y. */
         StringID=            16,    /**< SID ASCII String or File Name that
                                      * ends with 0. */
         Plot=                17,    /**< Place Pixel at X and Y. */
         eLine=               18,    /**< Draw a line to X and Y. */
         Box=                 19,    /**< Draws a Box to X and Y optional
                                      * fill. */
         zCircle=             20,    /**< Draws a Circle with Radius optional
                                      * fill */
         Arc=                 21,    /**< Draws an Arc with Radius and Begin 
                                      * Angle to End Angle. */
         Pie=                 22,    /**< Draws a Pie figure with Radius and
                                      * Begin Angle to End Angle and fills
                                      * it. */
         Picture=             24,    /**< Places a Picture on display. */
         Print=               25,    /**< Places the string on display which
                                      * ends with 0. */
         Beep_Freq=           26,    /**< Set the beeper frequency. */
         Get_Pixel=           27,    /**<get pixel         > */
         Calibrate=           28,    /**< Calibrate touch screen. */
         zReset=              29,    /**< Reset. */

         Rec_Macro=           30,    /**< Record Macro to flash drive. */
         Play_Macro=          31,    /**< Play Macro. */
         Stop_Macro=          32,    /**< Stop Macro. */
         Pause_Macro=         33,    /**< Pause n msec. */
         Loop_Macro=          34,    /**< Loop on Macro. */
         Speed_Macro=         35,    /**< Set the macro speed. */
         Peri=                36,
         ConfigIO=            37,
         IO=                  38,
         IOG=                 39,

         Security=            40,    /**< Set drive security string. */
         Location=            41,    /**< LID Location Vlaue. */
         Upgrade=             43,
         Parameters=          45,
         ClipEnable=          46,    /**< Set clip Enable. */
         ClipArea=            47,    /**< Set clip area. */

         /* Filesystem operations */
         Comment=             50,
         Fsgetcwd=            51,
         Fschdir=             52,
         Fsmkdir=             53,
         Fsrmdir=             54,
         Fsdir=               55,
         Fscopy=              56,
         Fsrename=            57,
         Fsremove=            58,
         Fsmore=              59,
         Format=              60,    /**< Format Flash Drive if string1 = 
                                      * "ezLCD" */
         If=                  61,
         Cmd=                 62,

         /* Widget commands */
         Set_Button=          70,    /**< Widget Button. */
         Set_CheckBox=        71,    /**< Widget Checkbox. */
         Set_Gbox=            72,    /**< Widget Group Box. */
         Set_RadioButton=     73,    /**< Widget Radio Button. */
         Set_DMeter=          74,    /**< Widget Digital Meter. */
         DMeter_Value=        75,    /**< Set DMeter value. */
         Set_AMeter=          76,    /**< Widget Analog Meter. */
         AMeter_Value=        77,    /**< Set AMeter value. */
         AMeter_Color=        78,    /**< Set AMeter color */
         Set_TouchZone=       79,    /**< touch zone       */
         Set_Dial=            80,    /**< Widget RoundDial. */
         Set_Slider=          82,    /**< Widget Slider. */
         Set_Progress=        85,    /**< Widget Progress bar. */
         Progress_Value=      86,    /**< Progress value. */
         Set_StaticText=      87,    /**< Widget Static text. */
         StaticText_Value=    88,    /**< Static text Value. */
         Choice=              89,    /**< Widget get choice. */

         Widget_Theme=        90,    /**< Widget Scheme. */
         Widget_Values=       91,    /**<Widget Values (Slider and Dial in this version).*/
	     Widget_State=        92,    /**<Widget State (Button, checkbox, radiobutton in this version).*/
		                             // no id returns the id of the last touched  
	
		 
         Mode=                98,
         Comport=             99,

         Xmax=                100,   /**< Return Xmax width. */
         Ymax=                101,   /**< Return Ymax height. */
         Wait=                102,   /**< Wait for touch. */
         Waitn=               103,   /**< Wait for no touch. */
         Waitt=               104,   /**< Wait for touch. */
         Threshold=           105,   /**< Touch threshold. */
         Verbose=             106,   /**< Controls the verbose mode. */
         Lecho=               107,   /**< Controls the echo mode. */
		 Xtouch=              110,   /**< return touchX. */
		 Ytouch=              111,   /**< return touchY. */
		 Stouch=              112,   /**< return touchS. */
         Wquiet=              113,
         Wstack=              114,
    };

/* Declares enums in the local class scope when \c EZLCD_GLOBALENUMS is 1 */
#if !EZLCD_GLOBALENUMS
    EZLCD_ENUM_ORIENTATION
    EZLCD_ENUM_ALIGNMENT
    EZLCD_ENUM_COMBALIGN
    EZLCD_ENUM_WIDGETS
    EZLCD_ENUM_CHOICE
#endif


protected:

    /**
     * Pointer to an instance of a serial communication class which is either
     * \c HardwareSerial (provided by Arduino) or \c SoftwareSerial (provided
     * in Arduino 1.0 and written by David A. Mellis).
     */
    Stream *m_pStream;
    
    /**
     * Establish communication with the ezLCD by sending "ping" commands and
     * waiting for a response. 
     */
    void findEzLCD();
    
    /**
     * Return whether the hardware or software serial class is being used.
     * \retval  true     \c HardwareSerial class is being used.
     * \retval  false    \c SoftwareSerial class is being used.
     */
    bool isHWSerial(); // was inline
	
   /**
     * Alternative to Stream parseInt that can parse hexadecimal numbers
     * \retval  the integer value of sequence of hex charactres
     */
	unsigned int parseHex(unsigned long timeout);


private:

    /**
	 * circle and rectangles are filled if true
	 */
    bool m_isFilled;
	
    /**
     * Maintain information on whether ezLCD currenly echoes its input. \c true
     * when it does and \c false otherwise.
     */
    bool m_echo;
   
    /**
     Return a justification string for the EarthSempl "print" command
     * \param[in]  align
     * \return     Justification string for the "print" command. Can be either
     *             \c "LT", \c "CT", \c "RT", \c "LC", \c "CC", \c "RC",
     *             \c "LB", \c "RB.
     */
    const char* jstStr( uint32_t align );

	
	/* Execute a command using the given arguments
     * outputs if \c m_echo  is set 
     * \return    Return value of the integer sent by the GPU in response to the cmd
     */
	 
    void initGPUCmd(int cmd);  // initiate a gpu command		 
    int terminateGPUCmd();     // terminate a command	
    void sendTwoArgs(int arg1, int arg2);
	void sendFourArgs(int arg1, int arg2, int arg3, int arg4);
	
	int sendGPUCmd(int cmd); 	
    int sendGPUCmd(int cmd, int arg1);
	int sendGPUCmd(int cmd, int arg1, int arg2);
    int sendGPUCmd(int cmd, int arg1, int arg2, int arg3);
	int sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4); 
    int sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6 );
    int sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6,
   	              int arg7 );
	int sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6,
   	              int arg7, int arg8);
	int sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6,
   	              int arg7, int arg8, int arg9);				  
	int sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6,
   	              int arg7, int arg8, int arg9, int arg10); 
	int sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6,
   	              int arg7, int arg8, int arg9, int arg10, int arg11);
	int sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6,
   	              int arg7, int arg8, int arg9, int arg10, int arg11, int arg12); 				  
	int sendGPUCmd(int cmd, const char* arg1); 
	int sendGPUCmd(int cmd, const char* arg1, int arg2); 
    int sendGPUCmd(int cmd, const char* arg1, const char* arg2); 
	int sendGPUCmd(int cmd, int arg1, const char* arg2); 
	int sendGPUCmd(int cmd, int arg1, int arg2, int arg3, const char* arg4); 
	int sendGPUCmd(int cmd, float arg1);
	
};

/**
 * Class derived from \c EzLCD3 that uses the Arduino's hardware serial
 * class \c HardwareSerial. 
 */
class EzLCD3_HW : public EzLCD3
{
public:

    /**
     * Class constructor. Requires no parameters because hardware serial
     * implies \c pin \c 0 for receive and \c pin \c 1 for transmit.
     */
    EzLCD3_HW();
    
    void begin( long baud );
};

/** 
 * Class that derived from \c EzLCD3 that uses Arduino's software serial
 * class \c SoftwareSerial.
 */

class EzLCD3_SW : public EzLCD3
{
public:

    /**
     * Class constructor
     * \param[in]  rx    Receive pin to be used.
     * \param[in]  tx    Transmit pin to be used.
     */
    EzLCD3_SW( int rx, int tx );
    void begin( long baud );

#if EZLCD_SERIALDEBUG
    /**
     * Debugging function to be called from the main loop of an Arduino code
     * in order to forward bytes from Arduino's hardware serial port to the
     * software serial port talking to ezLCD and read the response back.
     */
    void forwardSerial();
#endif

private:

    /**
     * Instance of a \c SoftwareSerial class that is controlling the ezLCD.
     */
    SoftwareSerial m_sws;
#if EZLCD_SERIALDEBUG
    /**
     * \c true when initialization for debugging over the hardware serial has
     * been completed.
     */
    static bool s_sdebugInit;
#endif
};

#endif // END #ifndef _EZLCD3XX_H
