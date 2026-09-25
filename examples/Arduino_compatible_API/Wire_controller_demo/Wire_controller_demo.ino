/** I2C controller (master) demo -- pair it with Wire_target_demo on
 *  another board.
 *
 *  Once a second this board:
 *    - writes a line of text to the target at address 0x08
 *    - reads 7 bytes back from it
 *  and prints what happened. If the target isn't there (not wired, or
 *  not running yet), it says so and keeps trying.
 *
 *  Wiring, between the two boards:
 *    D18 (SDA) -- D18 (SDA)
 *    D19 (SCL) -- D19 (SCL)
 *    GND       -- GND
 *
 *  Works on FRDM-MCXA153 and FRDM-MCXN947.
 */

#include <Arduino.h>
#include <Wire.h>

const uint8_t TARGET_ADDRESS = 0x08;

int count = 0;

// This demo's bus has no pull-up resistors, so it uses the pins' internal
// ones. A real I2C bus should have external pull-ups (e.g. 4.7k to 3.3V)
// on SDA and SCL instead; with those fitted, leave this out.
void enableInternalPullUps() {
#if defined(FRDM_MCXN947)
  PORT4->PCR[0] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;  // D18 = P4_0
  PORT4->PCR[1] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;  // D19 = P4_1
#else
  PORT1->PCR[8] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;  // D18 = P1_8
  PORT1->PCR[9] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;  // D19 = P1_9
#endif
}

void setup() {
  Serial.begin(115200);

  // A timeout keeps a transfer from hanging if the other board is reset
  // in the middle of it. It also lets the bus recover from the moment
  // before the pull-ups below are on, when the lines float.
  Wire.setWireTimeout(25000);
  Wire.begin();
  enableInternalPullUps();

  Serial.println("I2C controller ready");
}

void loop() {
  char text[24];
  snprintf(text, sizeof(text), "hello #%d", count);

  // Write
  Wire.beginTransmission(TARGET_ADDRESS);
  Wire.print(text);
  uint8_t result = Wire.endTransmission();

  Serial.print("sent \"");
  Serial.print(text);
  Serial.print("\" -> ");
  if (result == 0) {
    Serial.print("OK");
  } else {
    Serial.print("no answer (");
    Serial.print(result);
    Serial.print(")");
  }

  // Read
  if (result == 0) {
    uint8_t n = Wire.requestFrom(TARGET_ADDRESS, (size_t)7);
    Serial.print(", reply: \"");
    while (Wire.available())
      Serial.print((char)Wire.read());
    Serial.print("\" (");
    Serial.print(n);
    Serial.print(" bytes)");
  }
  Serial.println();

  count++;
  delay(1000);
}
