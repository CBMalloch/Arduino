#define BAUDRATE 115200

// allow user to select motor, line, etc. A/B, E/e, 0/1/2

#define N_MOTORS 2
int motorPins[N_MOTORS][3] = {{10, 2, 3}, {11, 4, 5}};  // {(PWM) enable, motor A, motor B}

#define paPot 3

/* CONNECTIONS TO L293D, MOTOR, and POT
      Pot L -> GND
      Pot C -> Analog 0
      Pot R -> AREF
      
      Note that L293D has diodes (L293 does not); L293D max 600mA, L293 max 1.0A
      L293D 
        1 - motor 1 enable PWM
        2, 7 - motor 1 direction pins (logic level, only one high at a time else short)
        3, 6 - motor 1 leads
        8 - motor power source +
        9 - motor 2 enable PWM
        10, 15 - motor 2 direction pins (logic level, only one high at a time else short)
        11, 14 - motor 2 leads
        4, 5, 12, 13 - Arduino and motor power source GND
        16 - Arduino 5V
*/

void setup() {
  int motor;
  
  Serial.begin(BAUDRATE);
  for (int motor = 0; motor < N_MOTORS; motor++) {
    for (int pin = 0; pin < 3; pin++) {
      pinMode (motorPins[motor][pin], OUTPUT); 
      digitalWrite (motorPins[motor][pin], LOW);
    }
  }

  if (1) {
    // diagnostic
    // A and B refer to motors
    // E and e turns on / off enable pin
    // + / - / . change direction lines for the current motor
    // 0..9 change PWM value (9 is 100%)
    // z breaks out to continue
    
    Serial.println ("Diagnostic: hit A/B (motors) E/e (en/disable) + / - / . (dir) 0-9 (PWM) z (exit)");
    
    short done = 0;
    
    while (!done) {
      char c = getch();
      
      switch (c) {
        case 'A':
        case 'a':
          motor = 0;
          break;
          
        case 'B':
        case 'b':
          motor = 1;
          break;
        
        case 'E':
          digitalWrite (motorPins[motor][0], HIGH);
          break;
          
        case 'e':
          digitalWrite (motorPins[motor][0], LOW);
          break;
          
        case '+':
          digitalWrite (motorPins[motor][2], LOW);
          digitalWrite (motorPins[motor][1], HIGH);
          break;
          
        case '-':
          digitalWrite (motorPins[motor][1], LOW);
          digitalWrite (motorPins[motor][2], HIGH);
          break;
          
        case '.':
          digitalWrite (motorPins[motor][1], LOW);
          digitalWrite (motorPins[motor][2], LOW);
          break;
          
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          analogWrite (motorPins[motor][0], 28 * (c - '0'));
          break;
          
        case 'z':
          done = 1;
          Serial.println ("Exiting diagnostic");
          break;
          
      }  // case
    }  // looping diagnostic
    
    for (int motor = 0; motor < N_MOTORS; motor++) {
      for (int pin = 0; pin < 3; pin++) {
        digitalWrite (motorPins[motor][pin], LOW);
      }
    }
  }  // diagnostic section enabled / disabled
  
}

uint8_t getch() {
  while(!Serial.available());
  return Serial.read();
}

void loop() {
  int value;
  value = analogRead(paPot);  // in range 0-1023
  Serial.print (value); Serial.print ("  ");
  value = map(value, 0, 1023, -255, 255);
  Serial.print (value); Serial.println ();
  if (value > 0) {
    for (int motor = 0; motor < N_MOTORS; motor++) {
      digitalWrite (motorPins[motor][0], LOW);    // safety while switching
      digitalWrite (motorPins[motor][2], LOW);
      digitalWrite (motorPins[motor][1], HIGH);
      analogWrite  (motorPins[motor][0], value);
    }
  } else {
    for (int motor = 0; motor < N_MOTORS; motor++) {
      digitalWrite (motorPins[motor][0], LOW);    // safety while switching
      digitalWrite (motorPins[motor][1], LOW);
      digitalWrite (motorPins[motor][2], HIGH);
      analogWrite  (motorPins[motor][0], -value);
    }
  } 
  delay (20);
}