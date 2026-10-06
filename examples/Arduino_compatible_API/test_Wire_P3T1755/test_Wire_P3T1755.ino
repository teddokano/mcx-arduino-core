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

// The on-board P3T1755's bus: Wire1 on FRDM-MCXA153 and FRDM-MCXN947,
// Wire (D18/D19) on FRDM-MCXA156, whose sensor is on the Arduino I2C pins
// FRDM-MCXN236 has none on board: connect one to its MikroBus I2C (Wire1)
#if defined(FRDM_MCXA153) || defined(FRDM_MCXN947) || defined(FRDM_MCXN236)
#define SENSOR_WIRE Wire1
#elif defined(FRDM_MCXA156)
#define SENSOR_WIRE Wire
#else
#error "This sketch has no settings for this board yet"
#endif

//P3T1755 sensor;
P3T1755 sensor(SENSOR_WIRE, 0x48);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  SENSOR_WIRE.begin();

  Serial.println("\n***** Hello, P3T1755! *****");
}

void loop() {
  float t = sensor.temp();

  Serial.println(t, 4);
  delay(1000);
}
