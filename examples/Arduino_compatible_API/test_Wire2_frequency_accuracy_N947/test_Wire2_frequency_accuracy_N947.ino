/** Wire2 (MikroBus I2C) clock accuracy check -- FRDM-MCXN947 only
 *
 *  Companion investigation to GitHub Issue #4 (SPI::frequency() silently
 *  failing to program the LPSPI baud rate, fixed in v0.4.1): mcu.cpp
 *  attaches Wire2's peripheral (LPI2C3/FlexComm3) to a 12MHz source
 *  (kFRO12M_to_FLEXCOMM3) that -- unlike the default SPI/SPI1, both of
 *  which turned out to need the faster kFRO_HF_DIV (48MHz) source and
 *  were fixed -- has never actually been measured against what
 *  setClock() requests. CLAUDE.md flags this as an open question.
 *
 *  Unlike SPI (which needs roughly 2x its source clock's rate, so it
 *  fails outright when the source is too slow), I2C's usual target
 *  speeds (100kHz/400kHz) are comfortably below a 12MHz source, so a
 *  repeat of the SPI bug's severity is unlikely here -- but "unlikely"
 *  isn't "confirmed", which is what this sketch is for.
 *
 *  No device required: each probe is a beginTransmission()/write()/
 *  endTransmission() to a fixed address that's expected to NAK -- timing
 *  is what matters here, not the ACK/NAK result. This sketch alone can
 *  only prove/disprove that setClock() has *some* real effect on timing
 *  (via the strict monotonic-scaling check below); confirming the
 *  absolute rate is numerically correct needs a logic analyzer on
 *  MB_SCL, the same way Issue #4 was ultimately confirmed for SPI.
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

void printResult(const char *label, unsigned long totalUs, unsigned long maxUs) {
  Serial.print(label);
  Serial.print(" : total=");
  Serial.print(totalUs);
  Serial.print("us  max_single_transfer=");
  Serial.print(maxUs);
  Serial.println("us");
}

unsigned long timeProbes(uint32_t hz) {
  Wire2.setClock(hz);
  lastMaxUs = 0;
  unsigned long t0 = micros();
  for (int i = 0; i < NUM_TRANSFERS; i++) {
    unsigned long tStart = micros();
    Wire2.beginTransmission(PROBE_ADDR);
    Wire2.write(0x00);
    Wire2.endTransmission();
    unsigned long elapsed = micros() - tStart;
    if (elapsed > lastMaxUs)
      lastMaxUs = elapsed;
  }
  return micros() - t0;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("=== Wire2 (MikroBus I2C) clock accuracy check ===");

  Wire2.begin();

  unsigned long t10k = timeProbes(10000UL);
  unsigned long max10k = lastMaxUs;
  delay(100);  // gap between segments, so they're easy to tell apart on a logic analyzer capture
  unsigned long t100k = timeProbes(100000UL);
  unsigned long max100k = lastMaxUs;
  delay(100);
  unsigned long t400k = timeProbes(400000UL);
  unsigned long max400k = lastMaxUs;

  printResult("Wire2  10kHz", t10k, max10k);
  printResult("Wire2 100kHz", t100k, max100k);
  printResult("Wire2 400kHz", t400k, max400k);

  // Generous upper bounds -- these only need to catch a true hang/stall,
  // not pin down the exact rate (that needs the logic analyzer). On a
  // genuinely stuck bus, expect these to blow way past the bounds below
  // instead of landing close to them.
  check("Wire2 10kHz completes within 2s", t10k < 2000000UL);
  check("Wire2 100kHz completes within 500ms", t100k < 500000UL);
  check("Wire2 400kHz completes within 200ms", t400k < 200000UL);
  check("Wire2 timing scales with requested clock (10kHz > 100kHz > 400kHz)",
        t10k > t100k && t100k > t400k);

  // Per-transfer bound, not just the total: the address-NAK-without-STOP bug
  // this sketch was written to chase showed up as a *single* transfer
  // stalling for 122ms, which a total-time bound alone absorbed silently
  // across 500 otherwise-fast transfers.
  check("Wire2 10kHz: no single transfer exceeds 50ms", max10k < 50000UL);
  check("Wire2 100kHz: no single transfer exceeds 50ms", max100k < 50000UL);
  check("Wire2 400kHz: no single transfer exceeds 50ms", max400k < 50000UL);

  Serial.println();
  Serial.println("Point a logic analyzer at MB_SCL to read the actual SCL");
  Serial.println("frequency for each of the three requests above and compare");
  Serial.println("against what was requested (10kHz/100kHz/400kHz).");
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
