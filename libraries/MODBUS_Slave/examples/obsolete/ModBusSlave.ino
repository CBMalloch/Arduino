/*
     Modbus Slave designed for Mach 3 CNC Software
 Written by: Tim W. Shilling, March 4, 2012.
 Released into the Public Domain 
 */
//############################################
//  Supported Functions
//  1:  Read Coil
//  2:  Read Descrete Input   
//  3:  Read Holding Reg
//  4:  Read Input Reg
//  5:  Write Single Coil
//  6:  Write Single Reg
//  7   Read Exception
//  15: Write Multiple Coils
//  16: Write Multiple Holding Reg

// Notes on Timing:
// Original Design implemented Timer 1 and 0, but these conflict with the PWM generators
// Timers dropped for millis and micros functions.  Not as accurate but fine for this purpose

//################# INCLUDES #################
#include <Modbus_Slave.h>    // Ensure you have included the libraries in the Arduino Libraries folder

//################# DEFINES ##################
#define Baudrate 19200       //Desired Baud Rate, Recommend, 9600, 19200, 56800, 115200
#define Freq 16000000        //Don't Touch unless using a differnt board, Freq of Processor
#define Slave_Address 0x01   //Address of MODBus Slave
#define Kill_Time 2000       //2 Sec (2000msec) keep alive, 0 => OFF

//############  REGISTER DEFINES ############# // Each Register is 16bit
#define Digital_IO_Register 0
#define PWM_Register        10
#define AN_Register         30
#define Timer_Register      50
#define IO_Config_Register  60
#define Kill_IO_Register    70
#define PWMIOMap_Register   80
#define ANIOMap_Register    81
#define General_Config      90
#define Error_Register      91
#define Digital_IO_Pins     14   //Total number of Digital IO pins, Limits update scanner pin count
#define Number_Of_Registers 100
//############ GLOBAL VARIABLES ##############
unsigned char Data[256];                                         // All received data ends up in here, also used as output buffer 
unsigned short Index = 0;                                        // Current Location in Data
unsigned short Register[Number_Of_Registers];                    // Where all user data, Coils/Registers are kept
ModBusSlave ModBus(Slave_Address,Register,Number_Of_Registers);  // Initialize a new ModBus Slave, Again, you must have my Arduino ModBusSlave Library 
unsigned long Last_Time=0;
unsigned long Time = 0;
unsigned long LongBreakTime;                                            //Time for 3.5 characters to be RX
//################## Setup ###################
// Takes:   Nothing
// Returns: Nothing
// Effect:  Opens Serial port 1 at defined Baudrate
//          Configures all Pins
//          Initiallized Timer 1
void setup()
{
  //################ Initialize IO #################     // 0 => Output, 1 => Input, opposite of normal Arduino, but my habit from other platforms, 0 looks like an O and 1 looks like an I
  Register[IO_Config_Register]    = 0b0000000000000000;  // UNO and MEGA  PIN 00-15
  Register[IO_Config_Register+1]  = 0b0000000000000000;  // MEGA          PIN 16-31
  Register[IO_Config_Register+2]  = 0b0000000000000000;  // MEGA          PIN 32-47
  Register[IO_Config_Register+3]  = 0b0000000000000000;  // MEGA          PIN 48-64
  Register[IO_Config_Register+4]  = 0b1111111111111111;  // AN Digital    PIN A0-A16

  //################ Kill IO Register #################  // 0 => Leave, 1 => Kill
  Register[Kill_IO_Register]    = 0b1111111111111111;    // UNO and MEGA  PIN 00-15
  Register[Kill_IO_Register+1]  = 0b1111111111111111;    // MEGA          PIN 16-31
  Register[Kill_IO_Register+2]  = 0b1111111111111111;    // MEGA          PIN 32-47
  Register[Kill_IO_Register+3]  = 0b1111111111111111;    // MEGA          PIN 48-64
  Register[Kill_IO_Register+3]  = 0b1111111111111111;    // AN Digital    PIN A0-A16

  //################ PWM IO Register #################   // 0 => Normal I/O, 1 => PWM I/O
  Register[PWMIOMap_Register]   = 0b0000111001101000;    // UNO and MEGA  PWM 01-16

  //################ AN IO Register #################    // 0 => Digital, 1=> Analog
  Register[ANIOMap_Register]    = 0b1111111111111111;    // UNO and MEGA

  Config_IO();

  LongBreakTime = (long)((long)28000000.0/(long)Baudrate);
  if(Baudrate > 19200)
    LongBreakTime = 1750;                                // 1.75 msec
  Serial.begin(Baudrate);                                // Open Serial port at Defined Baudrate
  Time = micros();                                       // Preload Time variable with current System microsec
}

//################ Main Loop #################
// Takes:   Nothing
// Returns: Nothing
// Effect:  Main Program Loop

unsigned long LongKillTime = (long)((long)Kill_Time * (long)1000);      //Time ellapsed between RX that causes system Kill

unsigned long Keep_Alive = 0;                                           // Keep Alive Counter Variable

void loop()
{
  Update_Time();
  if(Keep_Alive >= LongKillTime){               // Communications not Received in Kill_Time, Execute Lost Comm Emerency Procedure
    if(Kill_Time != 0){                         // If Kill_Time is 0, then disable Lost Comm Check
      Keep_Alive = LongKillTime;                // Avoid Keep_Alive rollover
      ModBus.Error = 0xff;                      // Set error code to Lost Comm
      Register[Error_Register] = ModBus.Error;  // Set Error Register
      Kill_IO();                                // Kill what should be killed
      digitalWrite(13,1);                       // Indicate Error with Solid LED
    }
  }
  else
  {
    Update_Pin_States();                        // Update Digital Pins
    Update_AN_States();                         // Update Analog Pins
    if(ModBus.Error != 0){                      // Flash Error LED is ModBus Error != 0
      digitalWrite(13,bitRead(Time,11));        // Toggle LED
    }
    else                                        // If no Error
    {
      digitalWrite(13,0);                       // Turn off LED
    }
  }
  //######################## RX Data ###############################
  if (Serial.available() > 0)                   // If Data Avilable
  {
    Data[Index] = (unsigned char)Serial.read(); // Read Next Char 
    Keep_Alive = 0;                             // Reset Keep Alive counter     
    Last_Time = micros();                       // Update Last_Time to current Time
    Index++;                                    // Move Index Counter Forward
  }
  //##################### Process Data #############################
  if(Index > 0)                                 // If there are bytes to be read
  {
    if(Keep_Alive >= LongBreakTime)             // Transmission Complete, 3.5 char spacing observered
    {
      Register[Timer_Register] = (unsigned int)(millis() / 125.0);  // Converts to 1/8sec and load into Register 
      ModBus.Process_Data(Data,Index);          // Process Data
      if(ModBus.Error == 0)                     // If no Errors, then... 
      {
        Keep_Alive = 0;                         // Reset Keep Alive 
        Last_Time = micros();                   // Set Last_Time to current time
      }
      else                                      // If there was an error
      {
        Register[Error_Register] = ModBus.Error;// Set Error Code
      }
      Index = 0;                                // Reset Index to 0, no bytes to read
    }
  }
}
//################ Update Time #################
// Takes:   Nothing
// Returns: Nothing
// Effect:  Updates Timer Register
//          Updates KeepAlive Count

void Update_Time()
{
  Last_Time = Time;                             // Set Last_Time to Current Time
  Time = micros();
  if(Time < Last_Time)                          // Counter has rolled over, should take 70 Min
  {
    Last_Time = 0;                              // Introduces a small clitch in timing by not accounting for how bad the roll over was.  Happens every 70 Min
                                                // Result is Delta will be smaller than it should be, Kill and Character Spacing will be usec longer than they should  
  }
  Keep_Alive += Time - Last_Time;               // Increment Keep Alive counter
  Register[Timer_Register] = (unsigned int)(millis() / 125.0);  // Converts to 1/8sec and load into Register 
}
//EOF
