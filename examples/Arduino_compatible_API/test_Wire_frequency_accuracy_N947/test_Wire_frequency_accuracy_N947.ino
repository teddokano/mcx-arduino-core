/** Wire (plain I2C, LPI2C2/D18-D19) clock accuracy check -- companion to the
 *  Wire2 investigation (test_Wire2_frequency_accuracy_N947).
 *
 *  Wire is backed by the *same family* of peripheral as Wire2 -- LPI2C2
 *  here vs. LPI2C3 for Wire2, both driven by the same fsl_lpi2c.c code --
 *  unlike Wire1 (I3C1 in I2C_MODE, a different peripheral entirely, which
 *  the companion Wire1 sketch already found does NOT reproduce Wire2's
 *  stall). If Wire (LPI2C2) reproduces the same stall, that points at
 *  something generic to the LPI2C driver/setClock() pattern, not
 *  something specific to Wire2/FlexComm3's clock source. If Wire is
 *  clean, suspicion narrows further to something specific to Wire2 (e.g.
 *  the LPI2C_MASTER_CLOCK_FREQUENCY macro hardcoded to FlexComm2's
 *  frequency regardless of which FlexComm is actually in use, found
 *  earlier in this investigation but not yet confirmed as the cause).
 *
 *  Same no-device-required probe methodology as the Wire2 sketch (expect a
 *  NAK from a fixed unused address; timing is what's being measured, not
 *  the ACK/NAK result), but this version also tracks the MAX single-
 *  transfer time, not just the total. The Wire2 investigation found a
 *  single transfer stalling for 122ms on a logic analyzer capture, which
 *  a sum-only bound missed entirely -- 500 fast transfers can absorb one
 *  huge outlier and still land under a generous total-time bound.
 *
 *  Order is 400kHz -> 100kHz -> 10kHz, with a small gap between
 *  back-to-back transactions -- deliberately the same conditions a
 *  logic-analyzer capture was taken under before the address-NAK-without-
 *  STOP fix, so the two captures can be compared directly.
 */

#include <Arduino.h>

#define NUM_TRANSFERS 500
#define PROBE_ADDR    0x08

// Gap between back-to-back transactions at the same clock speed. Sits
// outside the per-transaction timing window below, so max_single_transfer
// stays comparable to runs without a gap (the total does include it).
#define INTER_TRANSFER_GAP_US 5

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
  Wire.setClock(hz);
  lastMaxUs = 0;
  unsigned long t0 = micros();
  for (int i = 0; i < NUM_TRANSFERS; i++) {
    unsigned long tStart = micros();
    Wire.beginTransmission(PROBE_ADDR);
    Wire.write(0x00);
    Wire.endTransmission();
    unsigned long elapsed = micros() - tStart;
    if (elapsed > lastMaxUs)
      lastMaxUs = elapsed;
    delayMicroseconds(INTER_TRANSFER_GAP_US);
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

  Serial.println("=== Wire (plain I2C, LPI2C2/D18-D19) clock accuracy check ===");

  Wire.begin();

  unsigned long t400k = timeProbes(400000UL);
  unsigned long max400k = lastMaxUs;
  delay(100);
  unsigned long t100k = timeProbes(100000UL);
  unsigned long max100k = lastMaxUs;
  delay(100);
  unsigned long t10k = timeProbes(10000UL);
  unsigned long max10k = lastMaxUs;

  printResult("Wire 400kHz", t400k, max400k);
  printResult("Wire 100kHz", t100k, max100k);
  printResult("Wire  10kHz", t10k, max10k);

  // 50ms is generous for a single transfer at any of these speeds (a
  // normal transfer takes low tens of microseconds even at 10kHz) -- the
  // point is to catch the kind of pathological multi-tens/hundreds-ms
  // stall the Wire2 investigation found, not ordinary jitter.
  check("Wire 400kHz: no single transfer exceeds 50ms", max400k < 50000UL);
  check("Wire 100kHz: no single transfer exceeds 50ms", max100k < 50000UL);
  check("Wire  10kHz: no single transfer exceeds 50ms", max10k < 50000UL);

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
