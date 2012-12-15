#include <Thermosensor.h>

Thermistor xyzzy;

void setup () {
  Serial.begin(115200);
  xyzzy = Thermistor (
                      12,
                      10000.0,
                      10000.0,
                      25.0,
                      3380.0
                     );
}

void loop () {
  float theTemp;
  Serial.print(xyzzy.F2K(xyzzy.K2F(xyzzy.C2K(xyzzy.K2C(17.97)))));
  Serial.println(" s.b. 17.97");
  Serial.print(xyzzy.C2K(xyzzy.K2C(17.97)));
  Serial.println(" s.b. 17.97");
  Serial.print(xyzzy.F2K(xyzzy.K2F(17.97)));
  Serial.println(" s.b. 17.97");
  
  theTemp = xyzzy.getDegK(512);  // should be absolute zero
  Serial.print(theTemp);
  Serial.println(" degK");
  theTemp = xyzzy.K2C(theTemp);
  Serial.print(theTemp);
  Serial.println(" degC");
  theTemp = xyzzy.K2F(theTemp);
  Serial.print(theTemp);
  Serial.println(" degF");
  delay(100);
}