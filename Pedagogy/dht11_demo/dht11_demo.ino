/*
  Program DHT_demo
  Author: Charles B. Malloch, PhD
  Date:   2015-09-22
  
*/

#include "DHT.h"

#define pdDHT11 5

DHT dht ( pdDHT11, DHT11);

void setup() {
  Serial.begin(115200);
  
  dht.begin();
  
  Serial.println ( "DHT demo v1.0  2015-09-22  cbm" );
}

void loop() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float temp_degC = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float temp_degF = dht.readTemperature(true);
  
  Serial.print ( "E" );
  Serial.print ( humidity ); Serial.print ( "," );
  Serial.print ( temp_degF ); Serial.println ();

  delay(2000);
}
