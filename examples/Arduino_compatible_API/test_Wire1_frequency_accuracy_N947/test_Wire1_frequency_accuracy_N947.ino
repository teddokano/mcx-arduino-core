/** Wire1 (I3C-in-I2C-mode) clock accuracy check -- companion to the Wire2
 *  investigation (test_Wire2_frequency_accuracy_N947).
 *
 *  Wire1 is backed by a completely different peripheral (I3C1 in I2C_MODE)
 *  from Wire2 (LPI2C3) -- if the same erratic per-transaction timing shows
 *  up here too, that points at something in the shared Arduino-layer I2C
 *  code (arduino_i2c.cpp/TwoWire) rather than something LPI2C-specific.
 *  If Wire1 is clean, that keeps the suspicion on LPI2C/Wire2.
 *
 *  Same no-device-required probe methodology as the Wire2 sketch (expect a
 *  NAK from a fixed unused address; timing is what's being measured, not
 *  the ACK/NAK result), but this version also tracks the MAX single-
 *  transfer time, not just the total. The Wire2 investigation found a
 *  single transfer stalling for 122ms on a logic analyzer capture, which
 *  a sum-only bound missed entirely -- 500 fast transfers can absorb one
 *  huge outlier and still land under a generous total-time bound.
 *
 *  Order is 400kHz -> 100kHz -> 10kHz (reversed from the "obvious" order),
 *  matching what the Wire2 investigation found actually matters: the
 *  first clock speed used right after begin() behaved differently from a
 *  clock speed switched to after a prior transaction had already run.
 */

#include <Arduino.h>

#define NUM_TRANSFERS 500
#define PROBE_ADDR    0x08

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

unsigned long lastMaxUs = 0;

unsigned long timeProbes(uint32_t hz) {
  Wire1.setClock(hz);
  lastMaxUs = 0;
  unsigned long t0 = micros();
  for (int i = 0; i < NUM_TRANSFERS; i++) {
    unsigned long tStart = micros();
    Wire1.beginTransmission(PROBE_ADDR);
    Wire1.write(0x00);
    Wire1.endTransmission();
    unsigned long elapsed = micros() - tStart;
    if (elapsed > lastMaxUs)
      lastMaxUs = elapsed;
  }
  return micros() - t0;
}

void printResult(const char *label, unsigned long totalUs, unsigned long maxUs) {
  Serial.print(label);
  Serial.print(" : total=");
  Serial.print(totalUs);
  Serial.print("us  max_single_transfer=");
  Serial.print(maxUs);
  Serial.println("us");
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("=== Wire1 (I3C-in-I2C-mode) clock accuracy check ===");

  Wire1.begin();

  unsigned long t400k = timeProbes(400000UL);
  unsigned long max400k = lastMaxUs;
  delay(100);
  unsigned long t100k = timeProbes(100000UL);
  unsigned long max100k = lastMaxUs;
  delay(100);
  unsigned long t10k = timeProbes(10000UL);
  unsigned long max10k = lastMaxUs;

  printResult("Wire1 400kHz", t400k, max400k);
  printResult("Wire1 100kHz", t100k, max100k);
  printResult("Wire1  10kHz", t10k, max10k);

  // 50ms is generous for a single transfer at any of these speeds (a
  // normal transfer takes low tens of microseconds even at 10kHz) -- the
  // point is to catch the kind of pathological multi-tens/hundreds-ms
  // stall the Wire2 investigation found, not ordinary jitter.
  check("Wire1 400kHz: no single transfer exceeds 50ms", max400k < 50000UL);
  check("Wire1 100kHz: no single transfer exceeds 50ms", max100k < 50000UL);
  check("Wire1  10kHz: no single transfer exceeds 50ms", max10k < 50000UL);

  Serial.println();
  if (failCount == 0)
    Serial.println("ALL OK");
  else {
    Serial.print(failCount);
    Serial.println(" FAILED");
  }
}

void loop() {
}
