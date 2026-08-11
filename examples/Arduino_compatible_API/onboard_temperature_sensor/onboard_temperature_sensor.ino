/** On-board P3T1755 temperature sensor sample
 *
 *  FRDM-MCXA153 has an on-board P3T1755 temperature sensor connected via
 *  I3C (operated here in I2C mode through Wire1). No external wiring needed.
 *
 *  @author  Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  About P3T1755:
 *    https://www.nxp.com/products/sensors/ic-digital-temperature-sensors/i3c-ic-bus-0-5-c-accurate-digital-temperature-sensor:P3T1755DP
 */

#include <P3T1755.h>
#include <Wire.h>

P3T1755 sensor(Wire1, 0x48);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Wire1.begin();

  Serial.println("\n***** on-board P3T1755 temperature sensor *****");
}

void loop() {
  float t = sensor.temp();

  Serial.println(t, 4);
  delay(1000);
}
