//################## Config IO ###################
// Takes:   Nothing
// Returns: Nothing
// Effect:  Configures all Pins, Digital, PWM and Analog
//          Sets everything initially to OFF

void Config_IO()
{
  unsigned char Pin = 0;                              // PIN Number indexer
  for(int Index = 0 ; Index < 10; Index++)            // Scan through all available Registers, 0-9
  {
    for(int Bit = 0; Bit < 16; Bit++)                 // Scan through all 16 bits of each Register
    {
      Pin = (Index * 16)+Bit;                         // Calculate corrisponding Pin
      if(Pin >= Digital_IO_Pins)                      // If we have reached the number of IO pins
      {
        Index = 11;                                   // Set Index above max Index to force exiting of outer Loop
        break;                                        // Break For Loop
      }
      pinMode(Pin,~bitRead(Register[Index+IO_Config_Register],Bit));      // Set Pin Mode to Input or Output
      digitalWrite(Pin,bitRead(Register[Index+IO_Config_Register],Bit));  // Turns on Internal Pull-Up Resistor if Input
    }   
  } 
  for(int Bit = 0; Bit < 16; Bit++)                  // Cycle throuh all Analog pins
  {
    if(bitRead(Register[ANIOMap_Register],Bit)==0)   // If I/O Pin
    {
      if(Bit == 0)                                   // If Output
      {
        pinMode(A0,~bitRead(Register[IO_Config_Register+4],Bit));      // Set Pin Mode to Input or Output
        digitalWrite(A0,bitRead(Register[IO_Config_Register+4],Bit));  // Turns on Internal Pull-Up Resistor if Input
      }
      if(Bit == 1)
      {
        pinMode(A1,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A1,bitRead(Register[IO_Config_Register+4],Bit));
      }
      if(Bit == 2)
      {
        pinMode(A2,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A2,bitRead(Register[IO_Config_Register+4],Bit));
      }
      if(Bit == 3)
      {
        pinMode(A3,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A3,bitRead(Register[IO_Config_Register+4],Bit));
      }
      if(Bit == 4)
      {
        pinMode(A4,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A4,bitRead(Register[IO_Config_Register+4],Bit));
      }
      if(Bit == 5)
      {
        pinMode(A5,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A5,bitRead(Register[IO_Config_Register+4],Bit));
      }
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)          // If larger board
      if(Bit == 6)
      {
        pinMode(A6,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A6,bitRead(Register[IO_Config_Register+4],Bit));
      }
      if(Bit == 7)
      {
        pinMode(A7,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A7,bitRead(Register[IO_Config_Register+4],Bit));
      }
      if(Bit == 8)
      {
        pinMode(A8,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A8,bitRead(Register[IO_Config_Register+4],Bit));
      }
      if(Bit == 9)
      {
        pinMode(A9,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A9,bitRead(Register[IO_Config_Register+4],Bit));
      }
      if(Bit == 10)
      {
        pinMode(A10,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A10,bitRead(Register[IO_Config_Register+4],Bit));
      }
      if(Bit == 11)
      {
        pinMode(A11,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A11,bitRead(Register[IO_Config_Register+4],Bit));
      }
      if(Bit == 12)
      {
        pinMode(A12,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A12,bitRead(Register[IO_Config_Register+4],Bit));
      }
      if(Bit == 13)
      {
        pinMode(A13,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A13,bitRead(Register[IO_Config_Register+4],Bit));
      }
      if(Bit == 14)
      {
        pinMode(A14,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A14,bitRead(Register[IO_Config_Register+4],Bit));
      }
      if(Bit == 15)
      {
        pinMode(A15,~bitRead(Register[IO_Config_Register+4],Bit));
        digitalWrite(A15,bitRead(Register[IO_Config_Register+4],Bit));
      }
#endif

    }
  }
}

//################## Kill IO ###################
// Takes:   Nothing
// Returns: Nothing
// Effect:  Drive all desired PIN Low on execution

void Kill_IO()
{
  unsigned char Pin = 0;                              // PIN Number indexer
  for(int Index = 0 ; Index < 10; Index++)            // Scan through all available Registers, 0-9
  {
    for(int Bit = 0; Bit < 16; Bit++)                 // Scan through all 16 bits of each Register
    {
      Pin = (Index * 16)+Bit;                         // Calculate corrisponding Pin
      if(Pin >= Digital_IO_Pins)                      // If we have reached the number of IO pins
      {
        Index = 11;                                   // Set Index above max Index to force exiting of outer Loop
        break;                                        // Break For Loop
      }
      if(bitRead(Register[Index+IO_Config_Register],Bit) == 0) // If Output, then...
      {
        if(bitRead(Register[Index+Kill_IO_Register],Bit) == 1) // If Kill Desired on Output, then...
        {
          digitalWrite(Pin,0);                                 // Drive Output Low
        }
      }
    }
  }
  for(int Bit = 0; Bit < 16;Bit++)                         // Cycle through all Analog Pins
  {
    if(bitRead(Register[ANIOMap_Register],Bit) == 0)       // If Analog mode is Enabled, then...
    {
      if(bitRead(Register[IO_Config_Register+4],Bit) == 0) // If Output, then...
      {
        if(bitRead(Register[Kill_IO_Register],Bit) == 1)   // If Kill Desired on Output, then...
        {
          if(Bit == 0)
            digitalWrite(A0,LOW);                          // Drive PIN Low
          if(Bit == 1)
            digitalWrite(A1,LOW);
          if(Bit == 2)
            digitalWrite(A2,LOW);
          if(Bit == 3)
            digitalWrite(A3,LOW);
          if(Bit == 4)
            digitalWrite(A4,LOW);
          if(Bit == 5)
            digitalWrite(A5,LOW);

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)  // If Larger Board
          if(Bit == 6)
            digitalWrite(A6,LOW);
          if(Bit == 7)
            digitalWrite(A7,LOW);
          if(Bit == 8)
            digitalWrite(A8,LOW);
          if(Bit == 9)
            digitalWrite(A9,LOW);
          if(Bit == 10)
            digitalWrite(A10,LOW);
          if(Bit == 11)
            digitalWrite(A11,LOW);
          if(Bit == 12)
            digitalWrite(A12,LOW);
          if(Bit == 13)
            digitalWrite(A13,LOW);
          if(Bit == 14)
            digitalWrite(A14,LOW);
          if(Bit == 15)
            digitalWrite(A15,LOW);
#endif
        }
      }
    }
  }
}

//################## Update Analog States ###################
// Takes:   Nothing
// Returns: Nothing
// Effect:  Updates Analog Pin, either as Digital I/O pins or as Analog Read Pins
void Update_AN_States()
{
  for(int Bit = 0; Bit < 16; Bit++)                     // Cycle through available pins
  {
    if(bitRead(Register[ANIOMap_Register],Bit)==1)      // If Analog mode, then...
    {
      Register[Bit+AN_Register] = analogRead(Bit);      // Load Register with read value
      if(Register[Bit+AN_Register] >= 512)              // If Value is >= 2.5 Volts, then...
      {
        bitWrite(Register[Digital_IO_Register+4],Bit,1);// Set Digital I/O Register to 1
      }
      else                                              // If < 2.5 Volts, then...
      {
        bitWrite(Register[Digital_IO_Register+4],Bit,0);// Set Digital I/O Register to 0
      }
    }
    else                                                // If Digital Mode, then...
    {
      if(bitRead(Register[IO_Config_Register+4],Bit) == 1)  // If Input Pin
      {
        if(Bit == 0)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A0));
        if(Bit == 1)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A1));
        if(Bit == 2)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A2));
        if(Bit == 3)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A3));
        if(Bit == 4)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A4));
        if(Bit == 5)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A5));
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
        if(Bit == 6)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A6));
        if(Bit == 7)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A7));
        if(Bit == 8)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A8));
        if(Bit == 9)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A9));
        if(Bit == 10)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A10));
        if(Bit == 11)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A11));
        if(Bit == 12)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A12));
        if(Bit == 13)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A13));
        if(Bit == 14)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A14));
        if(Bit == 15)
          bitWrite(Register[Digital_IO_Register+4],Bit,digitalRead(A15));
#endif
      }
      else                                              // If Digital Pin, then...
      {
        if(Bit == 0)
          digitalWrite(A0,bitRead(Register[Digital_IO_Register+4],Bit));
        if(Bit == 1)
          digitalWrite(A1,bitRead(Register[Digital_IO_Register+4],Bit));
        if(Bit == 2)
          digitalWrite(A2,bitRead(Register[Digital_IO_Register+4],Bit));
        if(Bit == 3)
          digitalWrite(A3,bitRead(Register[Digital_IO_Register+4],Bit));
        if(Bit == 4)
          digitalWrite(A4,bitRead(Register[Digital_IO_Register+4],Bit));
        if(Bit == 5)
          digitalWrite(A5,bitRead(Register[Digital_IO_Register+4],Bit));

#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
        if(Bit == 6)
          digitalWrite(A6,bitRead(Register[Digital_IO_Register+4],Bit));
        if(Bit == 7)
          digitalWrite(A7,bitRead(Register[Digital_IO_Register+4],Bit));
        if(Bit == 8)
          digitalWrite(A8,bitRead(Register[Digital_IO_Register+4],Bit));
        if(Bit == 9)
          digitalWrite(A9,bitRead(Register[Digital_IO_Register+4],Bit));
        if(Bit == 10)
          digitalWrite(A10,bitRead(Register[Digital_IO_Register+4],Bit));
        if(Bit == 11)
          digitalWrite(A11,bitRead(Register[Digital_IO_Register+4],Bit));
        if(Bit == 12)
          digitalWrite(A12,bitRead(Register[Digital_IO_Register+4],Bit));
        if(Bit == 13)
          digitalWrite(A13,bitRead(Register[Digital_IO_Register+4],Bit));
        if(Bit == 14)
          digitalWrite(A14,bitRead(Register[Digital_IO_Register+4],Bit));
        if(Bit == 15)
          digitalWrite(A15,bitRead(Register[Digital_IO_Register+4],Bit));
#endif
      }
    }
  }
}

//################## Update Pin States ###################
// Takes:   Nothing
// Returns: Nothing
// Effect:  Updates igital I/O pins and PWM
void Update_Pin_States()
{
  Register[Error_Register] = ModBus.Error;
  unsigned char Pin = 0;                              // PIN Number indexer
  for(int Index = 0 ; Index < 10; Index++)            // Scan through all available Registers, 0-9
  {
    for(int Bit = 0; Bit < 16; Bit++)                 // Scan through all 16 bits of each Register
    {
      Pin = (Index * 16)+Bit;                         // Calculate corrisponding Pin
      if(Pin >= Digital_IO_Pins)                      // If we have reached the number of IO pins
      {
        Index = 11;                                   // Set Index above max Index to force exiting of outer Loop
        break;                                        // Break For Loop
      }
      if(bitRead(Register[Index+IO_Config_Register],Bit) == 1)        // If Input, then...
      {
        bitWrite(Register[Index+Digital_IO_Register],Bit,digitalRead(Pin));// Read input state into current Bit
      }
      else
      {
        if(Pin < 16)  // If PWM is even possible, all available PWM are on pins 0-15 on current Arduino devices
        {
          if(bitRead(Register[PWMIOMap_Register],Bit) == 0)  //Not a PWM pin, then...
          {
            digitalWrite(Pin,bitRead(Register[Digital_IO_Register],Bit));  // Set Output to reflecct bit
          }
          else
          { 
            analogWrite(Pin,lowByte(Register[PWM_Register+Pin]));  // Set PWM Duty Cycle
          }
        }
        else
        {
          digitalWrite(Pin,bitRead(Register[Index+Digital_IO_Register],Bit));  // Set Output to reflecct bit
        }
      }
    }
  }
}
//EOF
