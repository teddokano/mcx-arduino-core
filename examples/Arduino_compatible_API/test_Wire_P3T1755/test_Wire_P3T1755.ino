/** P3T1755 temperature sensor operation sample
 *  
 *  This sample code is showing P3T1755 temperature sensor operation.
 *
 *  @author  Tedd OKANO
 *
 *  Released under the MIT license License
 *
 *  About P3T1755:
 *    https://www.nxp.com/products/sensors/ic-digital-temperature-sensors/i3c-ic-bus-0-5-c-accurate-digital-temperature-sensor:P3T1755DP
 */

#include <P3T1755.h>
#include <Wire.h>

//P3T1755 sensor;
P3T1755 sensor(Wire1, 0x48);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Wire.begin();

  Serial.println("\n***** Hello, P3T1755! *****");
}

void loop() {
  float t = sensor.temp();

  Serial.println(t, 4);
  delay(1000);
}
