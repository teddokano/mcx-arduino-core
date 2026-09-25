/** Wire.begin() argument handling.
 *
 *  begin(address) makes Wire a target (slave); see test_Wire_target_self.
 *  On a bus that can't be one it stops the sketch with a message rather
 *  than going on as something else: Wire1 runs on the I3C peripheral, and
 *  FRDM-MCXN947's Wire2 (LPI2C3) doesn't respond as a target. This checks
 *  Wire1. Before v0.7.0 begin(0x10) quietly asked for a 16Hz controller.
 *
 *  Wiring: none -- and on FRDM-MCXN947, take release_check/11's Serial1
 *  loopback jumper (MB_TX-MB_RX) off first: those are Wire1's SDA/SCL
 *  pins there.
 *
 *  First checks that the pre-0.7.0 form, begin(frequency), still starts
 *  a controller (reads the on-board sensor on Wire1), then calls
 *  Wire1.begin(0x10).
 *
 *  Judged by reading the output: the last lines should be
 *    error: Wire1: target (slave) mode isn't supported on Wire1, ...
 *  repeated every couple of seconds, with the red LED blinking an SOS.
 *  "NOT STOPPED" means it failed.
 */

#include <Arduino.h>
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("=== Wire.begin() arguments (no wiring) ===");

  Wire1.begin(400000);  // deprecated form: a value over 127 is a frequency
  Wire1.beginTransmission(0x48);
  Wire1.write(0x00);
  bool ok = Wire1.endTransmission(false) == 0 && Wire1.requestFrom((uint8_t)0x48, (size_t)2) == 2;
  Serial.print("begin(400000) still starts a controller: ");
  Serial.println(ok ? "OK" : "FAIL");

  Serial.println("calling Wire1.begin(0x10) -- should stop here with an error:");
  Serial.flush();
  Wire1.begin(0x10);

  Serial.println("NOT STOPPED: Wire1.begin(address) went on as if it worked -- FAIL");
}

void loop() {
}
