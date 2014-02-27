// Example event loop - do two different things at once
// Charles B. Malloch, PhD  2012-09-30

#define BAUDRATE 115200
#define buttonPin 8
#define faderPin 9
#define ledPin 13

long tFader, tLED;
int faderTick = 30; 
int faderStep = -10;
int faderValue = 255;
int LEDTick = 500;
int LEDValue = 0;

void setup () {
  Serial.begin(BAUDRATE);
  pinMode (buttonPin, INPUT);
  pinMode (faderPin, OUTPUT);
  pinMode (ledPin, OUTPUT);
  tFader = 0;
  tLED = 3000;  // start LED 3 seconds after other stuff
}

void loop () {
  // unrelated detail: we use type unsigned long rather than type int 
  // for the variable now, because type int is 8 bits and can only hold 
  // numbers up to 65535. That means we'll roll over or start again 
  // every 65 seconds...
  // unsigned long can hold numbers up to 4,294,967,295
  unsigned long now = millis();
  
  // is it time to make a fader step?
  if (now > tFader + faderTick) {
    // it's time to fade!
    faderValue += faderStep;
    if (faderValue < 0 || faderValue > 255) {
      // turn around and go the other way
      Serial.println ("Fader reverse");
      // faderStep will always be 1 or -1
      faderStep = -faderStep;
      faderValue += faderStep;
    }
    tFader = now;
    // Serial.print ("Fader "); Serial.println (faderValue);
    analogWrite(faderPin, faderValue);
  }
  
  // is it time to change the state of the LED?
  if (now > tLED + LEDTick) {
    // turn the LED off or on
    // fancy trick: if it (LEDValue) was 1 (on), then the new value (1 – LEDValue) will be 0
    //              if it was 0, the new value will be 1
    LEDValue = 1 - LEDValue;
    Serial.print ("LED "); Serial.println (LEDValue);
    digitalWrite(ledPin, LEDValue);
    tLED = now;
  }

  // check the state of the button. It will be either 1 (representing 5 volts)
  // or 0 (representing 0 volts)
  // 1 is equivalent to TRUE, and 0 is equivalent to FALSE  
  if (digitalRead(buttonPin)) {
    LEDTick = 500;
  } else {
    LEDTick = 100;
  }
}
