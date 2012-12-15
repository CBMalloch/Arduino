/*
	test_FFT v0.1
	Charles B. Malloch, PhD
	2012-12-02
*/

#define BAUDRATE 115200

#include <FFT.h>

void setup () {
  Serial.begin(BAUDRATE);  
  FFTsetup();
	
  Serial.println ("Arduino ready");
}

void loop() {

	uint8_t incomingByte;      

	FFTloop();
	
	//CONSOLE COMMAND LINE INTERFACE
  //THERE ARE 7 COMMANDS,  1 SINGLE LETTER: 

  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    if (incomingByte == 'x') {           // PRINT INPUT ADC DATA
      FFT_print_ADC = 1;
    }
    if (incomingByte == 'f') {           // FFT OUTPUT
      FFT_print_FFT = 1;
    }
    if (incomingByte == 's') {           // SPECROGRAM PRE  FILTERED
      FFT_print_prefiltered = 1;
    }
    if (incomingByte == 'g') {           // SPECROGRAM POST FILTERED
      FFT_print_filtered = 1;
    }
    if (incomingByte == 'r') {           // RECORD SPECROGRAM TO EEPROM
      FFT_record_to_EEPROM = 1;
    }
    if (incomingByte == 'p') {           // PLAY SPECROGRAM FROM EEPROM
      FFT_play_from_EEPROM = 1;
    }
    if (incomingByte == 'm') {           // FREE MEMORY BYTES 
      Serial.println(freeRam());
    }
  }

}
