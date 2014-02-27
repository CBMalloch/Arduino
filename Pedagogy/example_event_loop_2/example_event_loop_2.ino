// Event Loop Demo
// Charles B. Malloch, Phd  2012-09-25

#define BAUDRATE 115200
#define steadyBlinkPin 13
#define variableBlinkPin 9
#define buttonPin 8
#define steadyBlinkONPeriod 500
#define steadyBlinkOFFPeriod 500

long tLastSteadyEvent, tLastVariableEvent;
int steadyBlinkState, variableBlinkState, variableBlinkONPeriod, variableBlinkOFFPeriod;

void setup() {
  serial.Begin(BAUDRATE);
  
  pinMode(steadyBlinkPin, OUTPUT);
  pinMode(variableBlinkPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  
  tLastSteadyEvent = 0;
  tLastVariableEvent = 0;
  
  variableBlinkONPeriod = 200;
  variableBlinkOFFPeriod = 300;
}

void loop() {
  long now;
  int buttonState;

  now = millis();
  if (steadyBlinkState == 1 && now > tLastSteadyEvent + steadyBlinkONPeriod) {
    # it's been on long enough. turn it off
    digitalWrite(steadyBlinkPin, 0);
    steadyBlinkState = 0;
    tLastSteadyEvent = now;
  }
  if (steadyBlinkState == 0 && now > tLastSteadyEvent + steadyBlinkOFFPeriod) {
    # it's been off long enough. turn it on
    digitalWrite(steadyBlinkPin, 1);
    steadyBlinkState = 1;
    tLastSteadyEvent = now;
  }

  if (variableBlinkState == 1 && now > tLastVariableEvent + variableBlinkONPeriod) {
    # it's been on long enough. turn it off
    digitalWrite(variableBlinkPin, 0);
    variableBlinkState = 0;
    tLastVariableEvent = now;
  }
  if (variableBlinkState == 0 && now > tLastVariableEvent + variableBlinkOFFPeriod) {
    # it's been off long enough. turn it on
    digitalWrite(variableBlinkPin, 1);
    variableBlinkState = 1;
    tLastVariableEvent = now;
  }
  
  buttonState = digitalRead(buttonPin);
  if (buttonPin == 0) {
    // button is pressed, grounding the sense line
    variableBlinkONPeriod = 400;
    variableBlinkOFFPeriod = 100;
  } else {
    // button is released, allowing the sense line to be 5V
    variableBlinkONPeriod = 200;
    variableBlinkOFFPeriod = 800;
  }
}
