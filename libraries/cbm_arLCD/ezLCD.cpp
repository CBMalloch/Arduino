/** ezlcd3xx.c
 * Source file for the ezlcd3xx Arduino library for ezLCD-3xx devices by
 * EarthLCD.com. The library communicates with the ezLCD via a hardware and/or
 * serial port 
 */

 /*
 * Arduino Library version by Michael Margolis
 * This version of the library requires the GPU to be in quiet mode
 * derived from code by Sergey Butylkov for EarthLCD.com
 *
 * Project home: http://arduino-ezlcd3xx.googlecode.com
 *
 * Copyright (C) 2012 Michael Margolis
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
  * Arduino Library version created 12 Oct 2012
  * Release beta Novermber 11 2012
  * Updated to v1.0 3/12/2013
  * Updated to v1.1 4/4/2013 'fixed isPressed function
  * Ken Segler EarthLCD
  */
 
#include <SoftwareSerial.h>
#include <Arduino.h>
#include "ezLCD.h"

#if EZLCD_SERIALDEBUG
    #define sdebug( str ) if( !isHWSerial() ) Serial.print(str)
    #define sdebugln( str ) if( !isHWSerial() ) Serial.println(str)
    bool EzLCD3_SW::s_sdebugInit = false;
#else
    #define sdebug( str )
    #define sdebugln( str )
#endif

#define DEFAULT_TIMEOUT 100  // default timeout for stream reads
#define PARSE_TIMEOUT   100  // timeout for parsing returned integer values

#define SEMPL_SEP   ' '        // separator between fields sent to GPU

EzLCD3::EzLCD3()
: m_echo( false )
{
}

EzLCD3::~EzLCD3()
{
}

// debug only, not for release
void EzLCD3::debugWrite(char b)
{
   m_pStream->write(b);
}


// function needed for arduino print methods
inline size_t EzLCD3::write(uint8_t value) {
static  char buf[4] = {0x22,0,0x22,0};

  buf[1] = value;
  if(value == 0xa)
    sendGPUCmd( Print, "\x5c\x6e");
  else if (value == 0xd)	
    sendGPUCmd( Print, "\x5c\x72");
  else
    sendGPUCmd( Print, buf );
	
  return 1; // assume success
}


bool EzLCD3::isHWSerial()
{
    // Compare the serial device used with the global instance
    // of the Arduino HardwareSerial class
    return m_pStream == &Serial;
}

void EzLCD3::findEzLCD()
{
    // First eat some possible buffered data from ezLCD output
    sdebugln("debug: flushing stream data...");
    while( m_pStream->available() )
        m_pStream->read();
    while( 1 )
    {
        // Send ping...
        sendGPUCmd(Ping);
        // wait up to default timeout (1 second) for response
		if( m_pStream->find("0"))  // todo check this
		{
		   sdebugln("debug: found ezLCD");
		   break;
		}
		else {
    	   sdebugln("."); // print a dot for each attempt    
		}  	   		
    }
}

void EzLCD3::initGPUCmd(int cmd)
{
    sdebug( "tx CMD: " );
    sdebugln( cmd );
	while( m_pStream->available() > 0) // flush data in serial buffer
	{ 
	   m_pStream->read();
    }	   
	m_pStream->print( cmd );       // send the command  
}

int EzLCD3::terminateGPUCmd()
{
    char data;
	m_pStream->write( '\r' ); // Append carriage-return
	while( m_pStream->available() == 0 );
//    data = m_pStream->read();
//    if ( data == '\r' ) 
    return 1;
//    else
//    return 0;
}

void EzLCD3::sendTwoArgs(int arg1, int arg2)
{
	m_pStream->print( arg1 );
    m_pStream->print( SEMPL_SEP  );
	m_pStream->print( arg2 );
    m_pStream->print( SEMPL_SEP  );	
}

void EzLCD3::sendFourArgs(int arg1, int arg2, int arg3, int arg4)
{
	sendTwoArgs(arg1, arg2);
	sendTwoArgs(arg3, arg4);
}

int EzLCD3::sendGPUCmd(int cmd)
{
    initGPUCmd(cmd);
    return terminateGPUCmd();
}

int EzLCD3::sendGPUCmd(int cmd, int arg1)
{
    initGPUCmd(cmd);
    m_pStream->print( SEMPL_SEP  );
	m_pStream->print( arg1 );
	return terminateGPUCmd( );
}

int EzLCD3::sendGPUCmd(int cmd, int arg1, int arg2)
{
    initGPUCmd(cmd);
	m_pStream->print( SEMPL_SEP  );
	m_pStream->print( arg1 );	
	m_pStream->print( SEMPL_SEP  );
	m_pStream->print( arg2 );
	return terminateGPUCmd( );;
}

int EzLCD3::sendGPUCmd(int cmd, int arg1, int arg2, int arg3)
{
    initGPUCmd(cmd);
	m_pStream->print( SEMPL_SEP  );
	sendTwoArgs( arg1, arg2);
	m_pStream->print( arg3 );
	return terminateGPUCmd( );
}

int EzLCD3::sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4)
{
    initGPUCmd(cmd);	
	m_pStream->print( SEMPL_SEP  );	
	sendTwoArgs(arg1, arg2);	
    m_pStream->print( arg3 );
    m_pStream->print( SEMPL_SEP  );
	m_pStream->print( arg4 );
	return terminateGPUCmd( );	
} 

int EzLCD3::sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6 )
{
    initGPUCmd(cmd);
	m_pStream->print( SEMPL_SEP  );	
    sendFourArgs(arg1, arg2, arg3, arg4);
	sendTwoArgs(arg5, arg6);
	return terminateGPUCmd( );
}	

int EzLCD3::sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6,
   	              int arg7 )
{
    initGPUCmd(cmd);
	m_pStream->print( SEMPL_SEP  );	
    sendFourArgs(arg1, arg2, arg3, arg4);
	sendTwoArgs(arg5, arg6);
	m_pStream->print( arg7 );
	return terminateGPUCmd( );
}	

int EzLCD3::sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6,
   	              int arg7, int arg8 )
{
    initGPUCmd(cmd);
	m_pStream->print( SEMPL_SEP  );	
    sendFourArgs(arg1, arg2, arg3, arg4);
	sendTwoArgs(arg5, arg6);
	m_pStream->print( arg7 );
    m_pStream->print( SEMPL_SEP  );
    m_pStream->print( arg8 );
	return terminateGPUCmd( );
}	

int EzLCD3::sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6,
   	              int arg7, int arg8, int arg9)
{
    initGPUCmd(cmd);
    m_pStream->print( SEMPL_SEP  );	
    sendFourArgs(arg1, arg2, arg3, arg4);
	sendFourArgs(arg5, arg6, arg7, arg8);
	m_pStream->print( arg9 );
	return terminateGPUCmd( );
}				  
				  
int EzLCD3::sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6,
   	              int arg7, int arg8, int arg9, int arg10)
{

    initGPUCmd(cmd);
    m_pStream->print( SEMPL_SEP  );		
	sendFourArgs( arg1, arg2, arg3, arg4);
	sendFourArgs(arg5, arg6, arg7, arg8);
    m_pStream->print( arg9 );
	m_pStream->print( SEMPL_SEP  );	
    m_pStream->print( arg10 );
	return terminateGPUCmd( );
}  

			  
int EzLCD3::sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6,
   	              int arg7, int arg8, int arg9, int arg10, int arg11)
{
    initGPUCmd(cmd);	
	m_pStream->print( SEMPL_SEP  );	
	sendFourArgs(arg1, arg2, arg3, arg4);
	sendFourArgs( arg5, arg6, arg7, arg8);
	sendTwoArgs( arg9, arg10);
	m_pStream->print( arg11 );
	return terminateGPUCmd( );	
}				  


int EzLCD3::sendGPUCmd(int cmd, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6,
   	              int arg7, int arg8, int arg9, int arg10, int arg11, int arg12)
{
    initGPUCmd(cmd);
	m_pStream->print( SEMPL_SEP );	
	sendFourArgs(arg1, arg2, arg3, arg4);
	sendFourArgs(arg5, arg6, arg7, arg8);
	sendTwoArgs( arg9, arg10);
	m_pStream->print( arg11 );
    m_pStream->print( SEMPL_SEP  );
	m_pStream->print( arg12 );
	return terminateGPUCmd( );	
}


int EzLCD3::sendGPUCmd(int cmd, const char* arg1)
{
    initGPUCmd(cmd);	
	m_pStream->print( SEMPL_SEP );
    m_pStream->print( arg1 );
    return terminateGPUCmd();
}
int EzLCD3::sendGPUCmd(int cmd, const char* arg1, int arg2) 
{
    initGPUCmd(cmd);	
	m_pStream->print( SEMPL_SEP );
    m_pStream->print( arg1 );
	m_pStream->print( SEMPL_SEP ); 
    m_pStream->print( arg2 );	
    return terminateGPUCmd();
}

int EzLCD3::sendGPUCmd(int cmd, const char* arg1, const char* arg2)
{
	initGPUCmd(cmd);
	m_pStream->print( SEMPL_SEP );
    m_pStream->print( arg1 );
    m_pStream->print( SEMPL_SEP );
    m_pStream->print( arg2 );
    return terminateGPUCmd();
}

int EzLCD3::sendGPUCmd(int cmd, int arg1, const char* arg2)
{
    initGPUCmd(cmd);	
	m_pStream->print( SEMPL_SEP );
    m_pStream->print( arg1 );
    m_pStream->print( SEMPL_SEP );
    m_pStream->print( arg2 );	
    return terminateGPUCmd();
} 

int EzLCD3::sendGPUCmd(int cmd, int arg1, int arg2, int arg3, const char* arg4)
{
    initGPUCmd(cmd);	
    m_pStream->print( SEMPL_SEP );
	sendTwoArgs(arg1, arg2);
	m_pStream->print( arg3);
    m_pStream->print( SEMPL_SEP );
    m_pStream->print( arg4 );
	return terminateGPUCmd();
}		  

int EzLCD3::sendGPUCmd(int cmd, float arg1)
{
    initGPUCmd(cmd);
	m_pStream->print( SEMPL_SEP );
    m_pStream->print( arg1 );
	return terminateGPUCmd();
}	

void EzLCD3::ezLCDUpgrade( void )
{
    m_pStream->print( "Upgrade ezLCD" );
	terminateGPUCmd();
}
void EzLCD3::printString( char *str )
{
    unsigned char c;
    m_pStream->print( Print );
    m_pStream->print( SEMPL_SEP );
    m_pStream->print( "\x22" );
    while( (c = *str++) )
        m_pStream->write( c );
    m_pStream->print( "\x22" );
	terminateGPUCmd();    
}
const char* EzLCD3::jstStr( uint32_t align )
{
    switch( align )
    {
        case LEFTTOP:     return "LT"; break;
        case TOP:         return "CT"; break;
        case RIGHTTOP:    return "RT"; break;
        case LEFT:        return "LC"; break;
        case CENTER:      return "CC"; break;
        case RIGHT:       return "RC"; break;
        case LEFTBOTTOM:  return "LB"; break;
        case BOTTOM:      return "CB"; break;
        case RIGHTBOTTOM: return "RB"; break;
    }
    return "";
}

void EzLCD3::reset()
{
    sdebug( "\ntx:zReset ");
    sendGPUCmd( zReset );
    findEzLCD(); // Attempt to establish communication after reset
}

void EzLCD3::wquiet() 
{
    sdebug( "\ntx:Clr_Screen ");
    sendGPUCmd(Wquiet);
}

uint16_t EzLCD3::getPixel(  uint16_t x, uint16_t y  ) 
{
    sendGPUCmd( Get_Pixel,x ,y );
    unsigned int ret =  m_pStream->parseInt();
    return ret;
}

unsigned int EzLCD3::wstack( int cmd ) 
{
    sendGPUCmd(Wstack,cmd);
    unsigned int ret =  m_pStream->parseInt() << 8;
    ret +=  m_pStream->parseInt();
    return ret;
}

void EzLCD3::cls( int id )
{
    sdebug( "\ntx:Clr_Screen ");
    sendGPUCmd(Clr_Screen, id);
}

void EzLCD3::cls( const char *color )
{
    sdebug( "\ntx:Clr_Screen,clr ");
	sendGPUCmd(Clr_Screen, color);
}


void EzLCD3::echo( bool val )
{
    sdebug( "\ntx:Lecho ");
    sendGPUCmd(Lecho, val ? "on" : "off" );
    m_echo = val;
}

void EzLCD3::light( int brightness, unsigned long timeout, int dimmed )
{
    sdebug( "\ntx:Light,br,t,d ");
	sendGPUCmd(Light, brightness, timeout, dimmed);
}	
	
void EzLCD3::light( int brightness, unsigned long timeout )
{
    sdebug( "\ntx:Light,br,t ");
	sendGPUCmd(Light, brightness, timeout);
}

	 
void EzLCD3::light( int brightness )
{
    sdebug( "\ntx:Light,br ");
	sendGPUCmd(Light, brightness);
}

int EzLCD3::light()
{
    sdebug( "\ntx:Light ");
    int ret = sendGPUCmd(Light);	
	sdebug( "debug: light = ");
    sdebugln( ret );
    if( ret > 0 )
    {
		m_pStream->setTimeout(PARSE_TIMEOUT);
        ret = m_pStream->parseInt(); // TODO - is this a long?
        sdebug( "debug: light = ");
        sdebugln( ret );
        return ret;
    }
	else
      return -1;
}

void EzLCD3::color( int id )
{
    sdebug( "\ntx:Color,id ");
    sendGPUCmd(Color, id );
}

void EzLCD3::color( uint8_t *red, uint8_t *green, uint8_t *blue )
{
    sdebug( "\ntx:Color ");
    int ret = sendGPUCmd( Color );	

    if( ret > 0 )
    {
	    m_pStream->setTimeout(PARSE_TIMEOUT);
        *red   =  m_pStream->parseInt();
        *green =  m_pStream->parseInt();
        *blue  =  m_pStream->parseInt();

        sdebug( "debug: R=" );
        sdebug( (int)*red );
        sdebug( " G=" );
        sdebug( (int)*green );
        sdebug( " B=" );
        sdebugln( (int)*blue );		
    }
    else
        sdebugln( "Error reading RGB values" );
}

void EzLCD3::colorId( int id, uint8_t red, uint8_t green, uint8_t blue )
{
    sdebug( "\ntx:eColor_ID,r,g,b ");
    sendGPUCmd( eColor_ID, id, red, green, blue );
}

void EzLCD3::colorId( int id, uint8_t *red, uint8_t *green, uint8_t *blue )
{
    sdebug( "\ntx:eColor_ID ");
    int ret = sendGPUCmd(eColor_ID, id);	

    if( ret > 0 )
    {  
	    m_pStream->setTimeout(PARSE_TIMEOUT);
        *red   =  m_pStream->parseInt();
        *green =  m_pStream->parseInt();
        *blue  =  m_pStream->parseInt();

        sdebug( "debug: R=" );
        sdebug( (int)*red );
        sdebug( " G=" );
        sdebug( (int)*green );
        sdebug( " B=" );
        sdebugln( (int)*blue );   sdebugln( *blue );	
    }
    else
        sdebugln( "Error reading RGB values" );
}

void EzLCD3::font( int id )
{

    sdebug( "\ntx:Font,id ");
    sendGPUCmd(Font, id );
}

void EzLCD3::font( const char* fontname )
{
    sdebug( "\ntx:Font,name ");
    sendGPUCmd( Font, fontname );
}

void EzLCD3::fontw( int theme, const char* fontname )
{
    sdebug( "\ntx:Fontw ");
    sendGPUCmd( Fontw, theme, fontname );			  
} 

void EzLCD3::fonto( int orientation )
{
    sdebug( "\ntx:Font_Orient ");
    sendGPUCmd( Font_Orient, orientation );
}

void EzLCD3::calibrate()
{
   sdebug( "\ntx:Calibrate ");
   sendGPUCmd( Calibrate );
}

void EzLCD3::lineWidth( int width )
{
    sdebug( "\ntx:Line_Width,w ");
    sendGPUCmd( Line_Width, width );
}

int EzLCD3::lineWidth()
{
    sdebug( "\ntx:Line_Width ");
    sendGPUCmd(Line_Width );

	m_pStream->setTimeout(PARSE_TIMEOUT);
    int ret =  m_pStream->parseInt();
    sdebug( "debug: linewidth = " );
    sdebug( ret );
    return ret;
}

void EzLCD3::lineType( int type )
{
    sdebug( "\ntx:Line_Type ");
    sendGPUCmd( Line_Type, type );
}

uint16_t EzLCD3::xmax()
{
    sdebug( "\ntx:Xmax ");
    sendGPUCmd( Xmax );
 
	m_pStream->setTimeout(PARSE_TIMEOUT);
    int ret =  m_pStream->parseInt();
    sdebug( "debug: xmax = " );
    sdebugln( ret );
    return ret;
}

uint16_t EzLCD3::width()
{
    return xmax()+1;
}

uint16_t EzLCD3::ymax()
{
    sdebug( "\ntx:Ymax ");
    sendGPUCmd( Ymax );
	
	m_pStream->setTimeout(PARSE_TIMEOUT);
    int ret =  m_pStream->parseInt();
    sdebug( "debug: ymax = " );
    sdebugln( ret );
    return ret;
}

uint16_t EzLCD3::height()
{
    return ymax()+1;
}

void EzLCD3::xy( uint16_t x, uint16_t y )
{
    sdebug( "\ntx:eXY,x,y ");
    sendGPUCmd( eXY, x, y  );
}

void EzLCD3::xyAligned( uint32_t align )
{
    const char *jst = jstStr(align);
	sdebug( "\ntx:eXY,align ");
    sendGPUCmd( eXY, jst );
}

void EzLCD3::xyGet( uint16_t *x, uint16_t *y )
{
    sdebug( "\ntx:eXY ");
    int ret = sendGPUCmd( eXY );

    if( ret > 0 )
    {
		m_pStream->setTimeout(PARSE_TIMEOUT);        
        *x = m_pStream->parseInt();
        *y = m_pStream->parseInt();
        sdebug( "debug: x = " );
        sdebug( *x );
        sdebug( " y = " );
        sdebugln( *y );
    }
}

void EzLCD3::xy_store( int id )
{
    sdebug( "\ntx:Location ");
    sendGPUCmd( Location, id );
}

void EzLCD3::xy_restore( int id )
{
    sdebug( "\ntx:Location,id ");
    sendGPUCmd( Location, id );
}

void EzLCD3::string( int id, const char *str )
{
    sdebug( "\ntx:StringId,id,str  ");  //ks changed 11/9
	initGPUCmd(StringID);   
	m_pStream->write( SEMPL_SEP );    
	m_pStream->print( id );    
	m_pStream->write( SEMPL_SEP);        
	m_pStream->write( '\"' );    
//    sendGPUCmd( StringID, id, str);
	m_pStream->print( str );
	m_pStream->write( '\"' );  
    terminateGPUCmd();	
}

#ifdef NOT_SUPPORTED
const char* EzLCD3::string( int id )
{
    sdebug( "\ntx:StringId,id  ");
    int ret = sendGPUCmd( StringID, id );

    if( ret > 0 ) //TODO 
    {
        const char *ptr = m_buf;
        sdebug( "debug: string id " );
        sdebug( id );
        sdebug( " = \"" );
        sdebug( ptr );
        sdebugln( '\"' );
        return ptr;
    }
    return NULL;
}
#endif

void EzLCD3::plot()
{
    sdebug( "\ntx:Plot ");
    sendGPUCmd( Plot );
}

void EzLCD3::plot( uint16_t x, uint16_t y )
{
    sdebug( "\ntx:Plot,x,y ");
    sendGPUCmd( Plot, x, y );
}

void EzLCD3::line( uint16_t x, uint16_t y )
{
    sdebug( "\ntx:eLine,x,y ");
    sendGPUCmd( eLine, x, y );
}

void EzLCD3::box( uint16_t width, uint16_t height, bool fill )
{
    sdebug( "\ntx:Box ");  
    sendGPUCmd( Box, width, height, fill ? 1 : 0 );
}

void EzLCD3::circle( uint16_t radius, bool fill )
{
    sdebug( "\ntx:zCircle ");
    sendGPUCmd( zCircle, radius, fill );  
}

void EzLCD3::arc( uint16_t radius, int16_t start, int16_t end )
{
    sdebug( "\ntx:Arc ");
    sendGPUCmd( Arc, radius, start, end );
}

void EzLCD3::pie( uint16_t radius, int16_t start, int16_t end )
{
    sdebug( "\ntx:Pie ");
    sendGPUCmd( Pie, radius, start, end );
}

void EzLCD3::picture( int id, uint16_t x, uint16_t y, 
                        uint16_t option )
{
    sdebug( "\ntx:Picture ");
    sendGPUCmd( Picture, x, y, option, id );
}

void EzLCD3::image( int id, uint16_t x, uint16_t y,
                      uint16_t options )
{
    picture( id, x, y, options );
}

void EzLCD3::picture( const char *filename, uint16_t x, uint16_t y,
                        uint16_t option )
{
    sdebug( "\ntx:Picture ");
    sendGPUCmd( Picture, x, y, option, filename );
}

void EzLCD3::image( const char *filename, uint16_t x, uint16_t y,
                      uint16_t option )
{
    picture( filename, x, y, option );
}

// Adds defines for graphics primitives similar to Processing and GLCD
// todo: add stroke and fill functions to set drawing states for these functions
#ifdef EZLCD_PROCESSING_PRIMITIVES

void EzLCD3::fill(bool isFilled)
{
  m_isFilled = isFilled;
}

void EzLCD3::point(int x, int y)
{
  plot(x,y);
} 


void EzLCD3::line(int x1, int y1, int x2, int y2)
{
  xy(x1,y1);
  line(x2,y2);
}

// rectangle with mode as corner 
void EzLCD3::rect(int x,int y,int width,int height)
{
  xy(x,y);
  box(width, height, m_isFilled); 
}

// draws circle with mode as center
void EzLCD3::ellipse(int x,int y,int diameter)
{
  xy(x,y);
  circle(diameter/2, m_isFilled);
}

// draws circle with mode as center
// height argument is ignored
// this version compatible with Processing
void EzLCD3::ellipse(int x,int y,int width,int height)
{
  xy(x,y);
  circle(width/2, m_isFilled);
}

// start and stop are in degrees (Processing uses radians)
void EzLCD3::arc(int x,int y,int diameter,int start,int stop) 
{
  xy(x,y); 
  arc(diameter/2,start, stop );
}
#endif  // Processing primitives

void EzLCD3::printStringId( int id, uint32_t align )
{
    sdebug( "\ntx:Print ");
    sendGPUCmd( Print, id, jstStr(align) );  
}

void EzLCD3::clipenable( bool enable )
{
    sdebug( "\ntx:ClipEnable ");
    sendGPUCmd( ClipEnable, enable ? "ON" : "OFF" );
}

void EzLCD3::cliparea( uint16_t left, uint16_t top, uint16_t right, uint16_t bottom )
{
    sdebug( "\ntx:ClipArea ");
    sendGPUCmd( ClipArea, left, top, right, bottom );
}

void EzLCD3::waitTouch( unsigned long timeout )
{
    sendGPUCmd( Waitt ); // was sendCommand();
	m_pStream->setTimeout(timeout);
    m_pStream->read();	//wait for the cr
	m_pStream->setTimeout(DEFAULT_TIMEOUT);
    sdebugln( "debug: waitt done." );
}

void EzLCD3::waitNoTouch( unsigned long timeout )
{
	sendGPUCmd( Waitn ); // was sendCommand();	
	m_pStream->setTimeout(timeout);
    m_pStream->read();	//wait for the cr
	m_pStream->setTimeout(DEFAULT_TIMEOUT);
    sdebugln( "debug: waitnt done." );
}

void EzLCD3::printAligned( const char *str, uint32_t align )
{
    sendGPUCmd( Print, str, jstStr(align) );
}

int EzLCD3::choice( const char *str, int theme, unsigned long timeout )
{
    sdebug( "\ntx:Choice ");
    sendGPUCmd( Choice, str, theme );
	m_pStream->setTimeout(PARSE_TIMEOUT);
    int ret =  m_pStream->parseInt();
    sdebug( "debug: choice = " );
    sdebugln( ret );
    return ret;
}

void EzLCD3::theme( int index, int EmbossDkColor, int EmbossLtColor,
                      int TextColor0, int TextColor1, int TextColorDisabled,
                      int Color0, int Color1, int colorDisabled,
                      int CommonBkColor, int fontw )
{
    sdebug( "\ntx:Widget_Theme ");
    sendGPUCmd(Widget_Theme,
        index, EmbossDkColor, EmbossLtColor,
        TextColor0, TextColor1, TextColorDisabled,
        Color0, Color1, colorDisabled, CommonBkColor, fontw );		
}

void EzLCD3::button( int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                uint16_t option, uint16_t align, uint16_t radius, int theme, int strid)
{
    sdebug( "\ntx:Widget_Theme ");
    sendGPUCmd( Set_Button, id, x, y, width, height, option,
                align, radius, theme, strid );
}

void EzLCD3::touchZone( int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                uint16_t option)
{
    sendGPUCmd( Set_TouchZone, id, x, y, width, height, option );
}
void EzLCD3::checkbox(  int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                          uint16_t option, int theme, int strid )
{
    sdebug( "\ntx:Widget_Theme ");
    sendGPUCmd( Set_CheckBox, id, x, y, width, height, option, theme, strid );
}

void EzLCD3::radioButton( int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                      uint16_t option, int theme, int strid )
{
    sdebug( "\ntx:Widget_Theme ");
    sendGPUCmd(Set_RadioButton, id, x, y, width, height, option, theme, strid );
}

void EzLCD3::groupBox( int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                      uint16_t option, int theme, int strid )
{  
    sdebug( "\ntx:Widget_Theme ");
    sendGPUCmd(Set_Gbox, id, x, y, width, height, option, theme, strid );
}

void EzLCD3::progressBar( int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                   uint16_t option, int initial, int range, int theme, int suffix)
{
    sdebug( "\ntx:Widget_Theme ");
    sendGPUCmd( Set_Progress, id, x, y, width, height, option, initial, range, theme, suffix );
}

void EzLCD3::staticText( int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                            uint32_t option, int theme, int strid )
{
    sdebug( "\ntx:Widget_Theme ");
    sendGPUCmd( Set_StaticText, id, x, y, width, height, option, theme, strid );
}

void EzLCD3::digitalMeter( int id, uint16_t x, uint16_t y, uint16_t width,
                      uint16_t height, uint16_t option, int initial,
					  int digits, int dotpos,int theme )
{
    sdebug( "\ntx:Widget_Theme ");
    sendGPUCmd(Set_DMeter, id, x, y, width, height, option, initial, digits, dotpos, theme );
}


void EzLCD3::analogMeter( int id, uint16_t x, uint16_t y, uint16_t width,
                    	uint16_t height, uint16_t option, int initial,
						int min, int max, int theme, int strid, int type)
{
    sdebug( "\ntx:Set_AMeter ");
    sendGPUCmd(Set_AMeter, id, x, y, width, height, option, initial, min, max, theme, strid, type );
}

void EzLCD3::dial( int id, uint16_t x, uint16_t y, uint16_t radius, uint16_t option,
             	int resolution, int initial, int max, int theme )
{
    sdebug( "\ntx:Set_Dial ");
    sendGPUCmd(Set_Dial, id, x, y, radius, option, resolution, initial, max, theme );
}

void EzLCD3::slider( int id, uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                       uint16_t option, int max, int resolution, int initial, int theme)
{   
    sdebug( "\ntx:Set_Slider ");
    sendGPUCmd( Set_Slider, id, x, y, width, height, option, max, resolution, initial, theme );
}

void EzLCD3::analogMeterColor(  int id, int color1, int color2, int color3, int color4, int color5, int color6 )
{
    sdebug( "\ntx:AMeter_Value ");
    sendGPUCmd( AMeter_Color, id, color1, color2, color3, color4, color5, color6 );
}

void EzLCD3::wvalue( int id, int value )
{
    sdebug( "\ntx:WValue ");
    sendGPUCmd( Widget_Values, id, value );
}

void EzLCD3::wvalue( int id, const char *str )    // added 2013/12/01 cbm
{
    sdebug( "\ntx:StringId,id,str  ");  //ks changed 11/9
	initGPUCmd(Widget_Values);   
	m_pStream->write( SEMPL_SEP );    
	m_pStream->print( id );    
	m_pStream->write( SEMPL_SEP);        
	m_pStream->write( '\"' );    
//    sendGPUCmd( StringID, id, str);
	m_pStream->print( str );
	m_pStream->write( '\"' );  
    terminateGPUCmd();	
}

unsigned int EzLCD3::wstate( int id )
{
    sdebug( "\ntx:WValue ");
    sendGPUCmd( Widget_State, id );
    unsigned int ret =  m_pStream->parseInt() ;
    ret +=  ( m_pStream->parseInt() << 8);
    return ret;
}

bool EzLCD3::isPressed( int id )
{
   return isChecked(id);
}

bool EzLCD3::isChecked( int id )
{
    sdebug( "\ntx:Widget_State ");
//    sendGPUCmd( Widget_State, id ); 
    initGPUCmd(Widget_State);
	m_pStream->print( SEMPL_SEP  );
	m_pStream->print( id );
	m_pStream->write( '\r' ); // Append carriage-return
//	m_pStream->setTimeout( PARSE_TIMEOUT );
    int data = m_pStream->parseInt();
	return ( data == 4 ) ? 1:0 ; // bit position 0x4 holds the checked state
}

int EzLCD3::getWidgetValue(int id)
{
    sdebug( "\ntx:get widget value ");
    sendGPUCmd( Widget_Values, id );
	m_pStream->setTimeout( PARSE_TIMEOUT );
    int data = m_pStream->parseInt();  
	return (data);    
}

int EzLCD3::touchS()
{
    sdebug( "\ntx:Stouch ");
    sendGPUCmd( Stouch );
	m_pStream->setTimeout( PARSE_TIMEOUT );
    int data = m_pStream->parseInt();  
	return (data);    
}

int EzLCD3::touchX()
{
    sdebug( "\ntx:Xtouch ");
    sendGPUCmd( Xtouch );
	m_pStream->setTimeout( PARSE_TIMEOUT );
    int data = m_pStream->parseInt();  
	return (data);    
}

int EzLCD3::touchY()
{
    sdebug( "\ntx:Ytouch ");
    sendGPUCmd( Ytouch );
	m_pStream->setTimeout( PARSE_TIMEOUT );
    int data = m_pStream->parseInt();  
	return (data);
}

unsigned int EzLCD3::parseHex(unsigned long timeout)
{
  long value = 0;
  boolean isNegative = false;
  char buf[8]; 
  char *p;
  
  m_pStream->setTimeout(timeout);
  m_pStream->readBytesUntil('\r', buf, sizeof(buf)-1);
  for( p = buf; *p; p++)
  {
    if(*p == '-')
      isNegative = true;
    else if(*p >= '0' && *p <= '9')        // is c a decimal digit?
      value = value * 16 + *p - '0';  
    else if(*p >= 'A' && *p <= 'F') 	  
	   value = value * 16 + *p - 'A' + 10;	  
  }

  if(isNegative)
    value = -value;
  return value;	
}
	
#ifdef NOT_SUPPORTED
// the following functions are not supported in this release	
void EzLCD3::record( const char *name )
{
    sendGPUCmd( Rec_Macro, name );
}

void EzLCD3::play( const char *name )
{
    sendGPUCmd( Play_Macro, name );
}

void EzLCD3::stop()
{
    sendGPUCmd( Stop_Macro );
}

void EzLCD3::pause( int msec )
{
    sendGPUCmd( Pause_Macro, msec );
}

void EzLCD3::loop( bool val )
{
    sendGPUCmd( Loop_Macro, val ? 1 : 0 );
}

void EzLCD3::speed( int msec )
{
    sendGPUCmd( Speed_Macro, msec );
}

void EzLCD3::cfgio( uint8_t port, uint8_t type )
{
    sendGPUCmd( ConfigIO, port, type );
}

void EzLCD3::io( uint8_t port, int data )
{
    sendGPUCmd( IO, port, data );
}

int EzLCD3::io( uint8_t port )
{
    int ret = sendGPUCmd( IO, port );
    if( ret > 0 )
    {
	    m_pStream->setTimeout(PARSE_TIMEOUT);
        int ret =  m_pStream->parseInt();
        sdebug( "debug: io @ port " );
        sdebug( port );
        sdebug( " = " );
        sdebugln( ret );
        return ret;
    }
    sdebugln( "io read failed" );
    return (uint8_t)-1;
}	
#endif

EzLCD3_HW::EzLCD3_HW()
{
    m_pStream = &Serial;
}

void EzLCD3_HW::begin( long baud )
{
    Serial.begin( baud );
	   
    findEzLCD();
    echo( false );
    sdebugln( "ezLCD initialization done" );
}


EzLCD3_SW::EzLCD3_SW( int rx, int tx )
: m_sws( rx, tx )
{
    m_pStream = &m_sws;
}

void EzLCD3_SW::begin( long baud )
{
#if EZLCD_SERIALDEBUG
    if( !s_sdebugInit )
    {
        // If the serial port was not intialized already,
        // do so now to enable debugging over it
        Serial.begin(9600);
        sdebugln( "\n* * * * *\nSerial debug initialized" );
        s_sdebugInit = true;
    }
#endif
    m_sws.begin( baud );
    findEzLCD();
    echo( false );
    sdebugln( "ezLCD initialization done" );
}

#ifdef NEEDED
#if EZLCD_SERIALDEBUG
void EzLCD3_SW::forwardSerial()
{
    while( Serial.available() )              // There are bytes from PC to be read
    {
        char ch = Serial.read();             // Read a char
        if( ch == '\n' )                     // Convert <LF> to <CR>, when needed
            ch = '\r';
        m_pStream->write( ch );              // Write to the serial stream.
    }
}
#endif
#endif

