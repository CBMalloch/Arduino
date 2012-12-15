#include <Stats.h>

Stats banana = Stats();
int i;
float x;
int ledPin = 13;   // select the pin for the LED

void setup()
{
  pinMode(ledPin, OUTPUT);  // declare the ledPin as an OUTPUT
  Serial.begin(115200);
  Serial.println(banana.resultString());
  Serial.println(banana.version());
  Serial.print(long(banana.results()[0] * 100));
  Serial.print("  ");
  Serial.println(long(banana.results()[1] * 100));
  banana.reset();
  banana.record(99.4);
  Serial.print(long(banana.results()[0] * 100));
  Serial.print("  ");
  Serial.println(long(banana.results()[1] * 100));
  banana.record(100.6);
  Serial.print(int(banana.results()[0] * 100));
  Serial.print("  ");
  Serial.println(int(banana.results()[1] * 100));
  banana.reset();
  banana.record(12.3);
  banana.record(14.3);
  banana.record(16.3);
  Serial.print(int(banana.results()[0] * 100));
  Serial.print("  ");
  Serial.println(int(banana.results()[1] * 100));
  banana.reset();
  for (i = 0; i < 1000; i++) {
    banana.record(float(i));
  }
  Serial.print(int(banana.results()[0] * 100));
  Serial.print("  ");
  Serial.println(int(banana.results()[1] * 100));
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
