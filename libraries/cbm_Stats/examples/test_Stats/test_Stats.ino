#include <Stats.h>

Stats banana = Stats();
int i;
float x;
int ledPin = 13;   // select the pin for the LED

void setup()
{
  pinMode(ledPin, OUTPUT);  // declare the ledPin as an OUTPUT
  Serial.begin(115200);
  while ( !Serial ) {}
  delay ( 100 );
  // Serial.println(banana.resultString());
  Serial.println(banana.version());                    // 1.000.000
  Serial.print(long(banana.results()[0] * 100));       // -65529
  Serial.print("  ");
  Serial.println(long(banana.results()[1] * 100));     //          -65529
  banana.reset();
  banana.record(99.4);
  Serial.print(long(banana.results()[0] * 100));       // 9940
  Serial.print("  ");
  Serial.println(long(banana.results()[1] * 100));     //          -65529
  banana.record(100.6);
  Serial.print(int(banana.results()[0] * 100));        // 10000
  Serial.print("  ");
  Serial.println(int(banana.results()[1] * 100));     //             84
  banana.reset();
  banana.record(12.3);
  banana.record(14.3);
  banana.record(16.3);
  Serial.print(int(banana.results()[0] * 100));       // 1430
  Serial.print("  ");
  Serial.println(int(banana.results()[1] * 100));     //           199
  banana.reset();
  for (i = 0; i < 1000; i++) {
    banana.record(float(i));
  }
  Serial.print(int(banana.results()[0] * 100));       // 49950
  Serial.print("  ");
  Serial.println(int(banana.results()[1] * 100));     //          28881
}

void loop()
{
  digitalWrite(ledPin, HIGH);  // turn the ledPin on
  delay(200);                  // stop the program for some time
  digitalWrite(ledPin, LOW);   // turn the ledPin off
  delay(200);                  // stop the program for some time
  digitalWrite(ledPin, HIGH);  // turn the ledPin on
  delay(200);                  // stop the program for some time
  digitalWrite(ledPin, LOW);   // turn the ledPin off
  delay(400);                  // stop the program for some time
}
