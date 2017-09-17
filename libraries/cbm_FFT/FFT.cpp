// FFT.c

#include <FFT.h>

// log2 FFT_SIZE   
#define LOG2_N          6
// full length of Sinewave[]
#define N_WAVE          ( FFT_SIZE / 2 )
// Adjustment timing to ~4 kHz
#define TIMER2          133

// Combine SINE and COSINE wave table,  2 bytes in one read-access to memory
const prog_int16_t Sinewave[N_WAVE] PROGMEM = {
  0X4000,0X4006,0X3F0C,0X3D13,0X3B18,0X381E,0X3524,0X3129,0X2D2D,0X2931,0X2435,0X1E38,0X183B,0X133D,0X0C3F,0X0640,
  0X0040,0XFA40,0XF43F,0XED3D,0XE83B,0XE238,0XDC35,0XD731,0XD32D,0XCF29,0XCB24,0XC81E,0XC518,0XC313,0XC10C,0XC006
};

void rev_bin ( CMPLX * fr ) {
  int8_t m, mr, nn, l, tr; 

  mr = 0;
  nn = FFT_SIZE - 1;        

  for ( m = 1; m <= nn; ++m ) { 
    l = FFT_SIZE;     
    do {
      l >>= 1;  
    } while (mr+l > nn); 

    mr = (mr & (l-1)) + l;

    if (mr <= m) continue;
    tr = fr[m].hb[1];
    fr[m].hb[1] = fr[mr].hb[1];
    fr[mr].hb[1] = tr;
  }
}

void FFT ( CMPLX * fr, int16_t fft_n ) {
  CMPLX wri;

  int16_t   shag, sredn, j, r, s;
  int8_t    extr, tr, ti;

  for ( extr = LOG2_N - 1; extr >= 0; --extr )
  {
    shag = ( fft_n >> extr );
    sredn = ( shag >> 1 );    
    j = 0;
    do {   
      wri.intl = pgm_read_word(&Sinewave[j<<extr]);  
      r = j;
      if ( !wri.hb[1] ) {                        // SIN = 0,  Twid Factor Optimization
        do {
          s = r + sredn; 

          tr =  fr[s].hb[0];  
          ti =  fr[s].hb[1];

          fr[s].hb[1] = (fr[r].hb[1] + tr)>>1;
          fr[s].hb[0] = (fr[r].hb[0] - ti)>>1;
          fr[r].hb[1] = (fr[r].hb[1] - tr)>>1;
          fr[r].hb[0] = (fr[r].hb[0] + ti)>>1;

          r += shag;
        } while ( r < fft_n );
      }
      else {
        if ( !wri.hb[0] ){                      // COS = 0, Twid Factor Optimization
          do {
            s = r + sredn; 

            tr = fr[s].hb[1];                                                     
            ti = fr[s].hb[0];

            fr[s].hb[1] = (fr[r].hb[1] - tr)>>1;
            fr[s].hb[0] = (fr[r].hb[0] - ti)>>1;
            fr[r].hb[1] = (fr[r].hb[1] + tr)>>1;
            fr[r].hb[0] = (fr[r].hb[0] + ti)>>1;

            r += shag;
          } while ( r < fft_n );
        } else {                                   // Full Butterfly Cell, Shift / Round Optimization
          do {
            s = r + sredn; 

            tr =  (wri.hb[1] * fr[s].hb[1] -  wri.hb[0] * fr[s].hb[0])>>6;                  
            ti =  (wri.hb[1] * fr[s].hb[0] +  wri.hb[0] * fr[s].hb[1])>>6;

            fr[s].hb[1] = (fr[r].hb[1] - tr)>>1;
            fr[s].hb[0] = (fr[r].hb[0] - ti)>>1;
            fr[r].hb[1] = (fr[r].hb[1] + tr)>>1;
            fr[r].hb[0] = (fr[r].hb[0] + ti)>>1;

            r += shag;
          } while ( r < fft_n );
        }
      }
      j++;
    }
    while ( j < sredn );
  }
}

void fix_fftr( CMPLX * fr, int16_t fft_n ) {
  rev_bin ( fr );
  
  FFT ( fr, fft_n );
}

int freeRam () {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

//**************************************************************************************************


volatile int8_t FFT_array_time [FFT_SIZE] = { 0};       // ADC Sampling array
CMPLX FFT_array_freq[FFT_SIZE] = {{ 0}};     						// FFT Real:Image Processing array
int8_t spectrogram_array[NBR_FRQ][TIME_SZ] = {{ 0}};    // Spectrogram Array 16 x 64
int8_t  TEMP[NBR_FRQ][3]      = {{ 0}};                 // Spectrogram Filtering Buffer 16 x 3

uint8_t FFT_record_to_EEPROM  = 0;      // Switch "RECORD" to EEPROM
uint8_t FFT_print_ADC   = 0;            // Switch "Print ADC"
uint8_t FFT_print_FFT  = 0;             // Switch "Print FFT"
uint8_t FFT_print_prefiltered = 0;      // Switch "Print Pre-Filter"
uint8_t FFT_print_filtered = 0;         // Switch "Print Post-Filter"
uint8_t FFT_play_from_EEPROM = 0;       // Switch "PLAY" from EEPROM

const     int8_t  EE[3][3] = {                        // EDGE ENHANCEMENT HPF MATRIX 3 x 3
                  {-1, -1, -1  },
                  {-1,  8, -1  },
                  {-1, -1, -1  }
                  };

volatile uint8_t capture = 0;         // Flag "capture in progress"
volatile uint8_t process = 0;         // Flag "new samples ready"
volatile uint8_t n_sampl = 0;         // Sampling counter
volatile int16_t address = 0;         // EEPROM address

const    uint8_t trigger =  75;       // Trigger Level VOX (VOice Activated Capture)
const    int16_t sdvigDC = 512;       // DC offset
uint8_t spectro = 0;          				// Spectrogram lines counter
uint8_t fltcorr = 0;          				// Flag "Spectrogram ready for Filtering & Cross-Correlation"

long    cross_c = 0;          				// Cross-Correlation Factor Current
long    cross_s = 0;          				// Cross-Correlation Factor Stored  
 
// ************************
               
void FFTsetup() {
  TCCR2A &= ~((1<<WGM21) | (1<<WGM20));
  TCCR2B &= ~(1<<WGM22);
  ASSR   &= ~(1<<AS2);
  TIMSK2  = 0;
  TIMSK0  = 0;
  TIMSK1  = 0;
  
  TCCR2B |= ((1<<CS21) | (1<<CS20));
  TCCR2B &= ~(1<<CS22) ;                         // prescaler = 32, tick = 2 microsec.
  TCNT2   = TIMER2; 
  TIMSK2 |= (1<<TOIE2);

  ADCSRA  = 0x87;
  ADMUX   = 0x40;
  ADCSRA |= (1<<ADSC);
}

void FFT_print_spg1() {
  for ( int strok = 0; strok < TIME_SZ; strok++ ) {
    for ( int stolb = 0; stolb < NBR_FRQ; stolb++ ) {
     Serial.print("\t");
     Serial.print(spectrogram_array[stolb][strok], DEC);
    } 
    Serial.print("\n");
  }
  Serial.print("\n");
}

void FFT_print_spg2() {
  for ( int strok = 0; strok < TIME_SZ; strok++ ) {
    for ( int stolb = 0; stolb < NBR_FRQ; stolb++ ) {
     Serial.print("\t");
     Serial.print(spectrogram_array[stolb][strok], DEC);
    } 
    Serial.print("\n");
  }
  Serial.print("\n\tCROSS-CORRELATION FACTOR: ");
  Serial.print( cross_c, DEC );
  Serial.print("\tSTORED: ");
  Serial.print( cross_s, DEC );
  Serial.print("\tMATCH: ");
  Serial.print((((cross_c +1) * 100) / (cross_s +1)), DEC );
  Serial.println(" %.");
}

void FFTloop() {
  if ( process ) {
    // *outs[pdPROCESSING] |= bits[pdPROCESSING]; // digitalWrite(12, HIGH);
    for ( int8_t i = 0; i < FFT_SIZE; i++ ) {
      FFT_array_freq[i].hb[0] = 0;                   // Image -zero.
      FFT_array_freq[i].hb[1] = FFT_array_time[i];                // Real - data.
    }    
    
    fix_fftr ( FFT_array_freq, FFT_SIZE );

    for ( int s = 0; s < NBR_FRQ; s++) {
      spectrogram_array[s][spectro] = 0;                              // Clear, Allow to use it as accumulator
    }    

    for ( int i = 0; i < (FFT_SIZE >> 1); i++) {          //OPTIMIZED: CALCULUS MAGNITUDE FOR HALF BINS ONLY
       FFT_array_freq[i].hb[1] = sqrt(FFT_array_freq[i].hb[1] * FFT_array_freq[i].hb[1] + FFT_array_freq[i].hb[0] * FFT_array_freq[i].hb[0]);
  //         if ( FFT_array_freq[i].hb[1] > 2 )                        // NOISE CANCELLER
  //           spectrogram_array[i>>1][spectro] += (FFT_array_freq[i].hb[1]);
    }

  // Non-Linear Compression 31 Bins to 16 Bands (Memory Limits: 16 x 64 = 1 K. ).
  // 0 - 1  1 - 1  2 - 1  3 - 1  4 - 1  5 - 1  
  //        6 - 2  7 - 2  8 - 2  9 - 2 10 - 2 
  //       11 - 3 12 - 3 13 - 3 14 - 3 15 - 3            // Total: 31  

    for ( int ps = 1, pt = 0; ps < 32; ps++ ) {           // No DC, start from 1
      if ( ps > 14 )  {
        pt = 10 + ((ps - 14)/3);          
      } else if ( ps > 5 )  {
          pt = 5 + ((ps - 5)/2);
      } else {
        pt = ps -1;
      }
  //     if ( FFT_array_freq[ps].hb[1] > 2 )                           // NOISE CANCELLER
      spectrogram_array[pt][spectro] += (FFT_array_freq[ps].hb[1]);
    }

    // *outs[pdPROCESSING] &= ~bits[pdPROCESSING]; // digitalWrite(12, LOW);

    spectro++; 
    if ( spectro >= TIME_SZ ) {
      capture = 0;
      spectro = 0; 
      // *outs[pdCAPTURING] &= ~bits[pdCAPTURING]; // digitalWrite( 13, LOW );
      fltcorr = 1;
    }
    process = 0;
  }

  if ( fltcorr ) { 
    // *outs[pdFILTERING] |= bits[pdFILTERING]; // digitalWrite(11, HIGH);

    if ( FFT_print_prefiltered ) {                                 // PRINT OUT PRE-FILTERED SPECTROGRAM (DEBUG)
      FFT_print_spg1();
      FFT_print_prefiltered = 0;
    }
  //  memset( SPG2, 0, sizeof(SPG2));

    for ( int strok = 0; strok < TIME_SZ; strok++ ) {  //FILTERING EDGE_ENHANCEMENT (HPF KERNEL 3 x 3) IN-PLACE
      for ( int tmp = 0; tmp < NBR_FRQ; tmp++ ){
        TEMP[tmp][2] = TEMP[tmp][1];
        TEMP[tmp][1] = TEMP[tmp][0];
        TEMP[tmp][0] = spectrogram_array[tmp][strok];
        spectrogram_array[tmp][strok]     = 0;
      }
      for ( int b = 0; b < 3; b++ ) {
        for ( int stolb = 0; stolb < NBR_FRQ; stolb++ ){
          for ( int a = 0; a < 3; a++ ) {
            int c = stolb + a - 1;
            if ((c >= 0) && (c < NBR_FRQ)) {
//                        spectrogram_array[stolb][strok] += ((EE[a][b] * TEMP[c][b]) >> 2 );// GAIN = 2, DIVISION by 4, NOT 8
              int rounddown = EE[a][b] * TEMP[c][b];                 //SYMETRICAL  ROUNDDOWN
              if (rounddown < 0)
                rounddown = ( rounddown + 4 ) >> 3;
              else
                rounddown >>= 3;
              spectrogram_array[stolb][strok] += rounddown;
            }
          }
        }
      }
    }

    // *outs[pdFILTERING] &= ~bits[pdFILTERING]; // digitalWrite(11, LOW);    
    // *outs[pdSPECTROGRAM_STORE_COMPARE] |= bits[pdSPECTROGRAM_STORE_COMPARE]; // digitalWrite(10, HIGH);    

    if ( FFT_record_to_EEPROM ) {   //STORE SPECTROGRAM
      cross_s = 0;
      address = 0;
      
      for ( int8_t strok = 0; strok < TIME_SZ; strok++ ) {
        for ( int8_t stolb = 0; stolb < NBR_FRQ; stolb++ ) {
   
          while (EECR & (1<<EEPE));
          EEAR = address;
          EEDR = spectrogram_array[stolb][strok];
          EECR |= (1<<EEMPE);
          EECR |= (1<<EEPE);

          cross_s = cross_s + (spectrogram_array[stolb][strok] * spectrogram_array[stolb][strok]);
            
          address++;
          if ( address >= 1024 )
            address = 0;  
        }
      }
      FFT_record_to_EEPROM = 0;
    } else {                //COMPARE VIA CROSS-CORRELATION 
      address = 0;
      cross_c = 0;

      for ( int8_t strok = 0; strok < TIME_SZ; strok++ ) {  
        for ( int8_t stolb = 0; stolb < NBR_FRQ; stolb++ ) {

          while (EECR & (1<<EEPE));
          EEAR = address;
          EECR |= (1<<EERE);

          cross_c = cross_c + (((int8_t) EEDR) * spectrogram_array[stolb][strok]);

          address++;
          if ( address >= 1024 ) address = 0;  
        }
      }
    }
    // *outs[pdSPECTROGRAM_STORE_COMPARE] &= ~bits[pdSPECTROGRAM_STORE_COMPARE]; // digitalWrite(10, LOW);
    if ( FFT_print_filtered ) {                             //POST-FILTERED SPECTROGRAM
      FFT_print_spg2();
      FFT_print_filtered = 0;
    }
    fltcorr = 0;
  }

  if ( FFT_print_ADC ) {
    // print ADC input data
    for ( uint8_t i = 0; i < FFT_SIZE; i++) {
      Serial.print("\t");
      Serial.print( FFT_array_time[i], DEC);
      if ((i+1)%16 == 0) Serial.print("\n");
    } 
    Serial.print("\n");
    FFT_print_ADC = 0;
  }

  if ( FFT_print_FFT ) {
    // print fft
    for ( uint8_t i = 0; i < ( FFT_SIZE >> 1); i++) {
      Serial.print ("\t");
      Serial.print (FFT_array_freq[i].hb[1], DEC);
      if ((i+1) % 16 == 0) Serial.print("\n");
    } 
    Serial.print ("\n");
    FFT_print_FFT = 0;
  }

  if ( FFT_play_from_EEPROM ) {
    address = 0;

    for ( int strok = 0; strok < TIME_SZ; strok++ ) { 
      for ( int stolb = 0; stolb < NBR_FRQ; stolb++ ) {

        while(EECR & (1<<EEPE));
        EEAR = address;
        EECR |= (1<<EERE);

        Serial.print("\t");
        Serial.print(((int8_t) EEDR), DEC);

        address++;
        if ( address >= 1024 ) address = 0;  
      }
      Serial.print("\n");
    }
//     Serial.print("\n\tTOTAL: ");
//     Serial.println( address, DEC );
    FFT_play_from_EEPROM = 0;
  }

}

// this is an interrupt service routine. Probably reacts to the completion of an analog read.
ISR(TIMER2_OVF_vect) {
  TCNT2 = TIMER2;               

  if (ADCSRA & 0x10) {
	
    int16_t temp;
		temp = ADCL;
		temp += (ADCH << 8);
		temp -= sdvigDC;
		temp >>= 1;                  // OPA is not rail-to-rail. ~50% Dynamics only
    ADCSRA |= (1<<ADSC);

		// restart capturing
    if ((abs(temp) > trigger) && (!capture)) {
      capture = 1;
      n_sampl = 0;
      // *outs[pdCAPTURING] |= bits[pdCAPTURING]; // digitalWrite( 13, HIGH );
    }
    if ( capture ) {
      if ( temp >  127 ) temp =  127;     // CLIPPING
      if ( temp < -128 ) temp = -128;
      FFT_array_time[n_sampl]  = temp; 
      n_sampl++;    
    }
  }
  
  if ( n_sampl >= FFT_SIZE ) {
    n_sampl = 0;
    process = 1;
  }
}
