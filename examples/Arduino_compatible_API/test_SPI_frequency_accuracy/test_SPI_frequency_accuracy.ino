/** SPI clock accuracy check -- regression test for GitHub Issue #4
 *  (SPI::frequency() silently failing to program the LPSPI baud rate).
 *
 *  Before the fix, TCR[PRESCALE] never actually changed: elapsed time
 *  for a fixed number of transfers stayed roughly constant no matter
 *  what frequency was requested (SPISettings' bitrate was ignored, and
 *  the actual prescaler ended up as whatever garbage was on the stack
 *  at the time). After the fix, elapsed time should scale with the
 *  requested frequency -- slower requests take measurably longer.
 *
 *  This mirrors the exact methodology used to find and confirm the fix
 *  in the issue itself: time N SPI.transfer(uint8_t) calls at a few
 *  different SPISettings and compare. No logic analyzer needed --
 *  the effect is large enough (two orders of magnitude, pre-fix) to
 *  see in wall-clock timing alone.
 *
 *  Wiring: same as release_check/05_spi_loopback -- jumper D11(MOSI)
 *  to D12(MISO) for the default SPI, and the MikroBus header's
 *  MOSI to MISO for SPI1.
 */

#include <Arduino.h>

#define NUM_TRANSFERS 2000

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

unsigned long timeTransfers(SPIClass &spi, uint32_t hz) {
  spi.beginTransaction(SPISettings(hz, MSBFIRST, SPI_MODE0));
  unsigned long t0 = micros();
  for (int i = 0; i < NUM_TRANSFERS; i++)
    spi.transfer(0xFF);
  unsigned long elapsed = micros() - t0;
  spi.endTransaction();
  return elapsed;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("=== SPI clock accuracy check (Issue #4 regression test) ===");

  // ---- default SPI: 24MHz / 250kHz / 50kHz, same points as the issue ----
  pinMode(SS, OUTPUT);
  digitalWrite(SS, HIGH);
  SPI.begin();

  unsigned long t24m = timeTransfers(SPI, 24000000UL);
  unsigned long t250k = timeTransfers(SPI, 250000UL);
  unsigned long t50k = timeTransfers(SPI, 50000UL);

  Serial.print("SPI  24MHz : ");
  Serial.print(t24m);
  Serial.println(" us");
  Serial.print("SPI 250kHz : ");
  Serial.print(t250k);
  Serial.println(" us");
  Serial.print("SPI  50kHz : ");
  Serial.print(t50k);
  Serial.println(" us");

  // The bug's signature was near-constant timing regardless of requested
  // frequency; a fixed prescaler is also a fixed *loose* bound (whatever
  // divider was on the stack could itself have been the slow one), so the
  // real tell is that timing scales with the request -- generous absolute
  // bounds (well beyond the issue's own measured values) plus strict
  // monotonicity are what actually distinguish "fixed" from "still broken".
  check("SPI 24MHz completes well under 100ms", t24m < 100000UL);
  check("SPI 250kHz completes within 40-200ms", t250k > 40000UL && t250k < 200000UL);
  check("SPI 50kHz completes within 200-600ms", t50k > 200000UL && t50k < 600000UL);
  check("SPI timing scales with requested frequency (50kHz > 250kHz > 24MHz)",
        t50k > t250k && t250k > t24m);

  // ---- SPI1 (MikroBus): spot-check at one frequency, same code path ----
  Serial.println("--- SPI1 (MikroBus) spot check ---");
  pinMode(MB_CS, OUTPUT);
  digitalWrite(MB_CS, HIGH);
  SPI1.begin();

  unsigned long spi1_t1m = timeTransfers(SPI1, 1000000UL);
  unsigned long spi1_t100k = timeTransfers(SPI1, 100000UL);

  Serial.print("SPI1   1MHz : ");
  Serial.print(spi1_t1m);
  Serial.println(" us");
  Serial.print("SPI1 100kHz : ");
  Serial.print(spi1_t100k);
  Serial.println(" us");

  check("SPI1 timing scales with requested frequency (100kHz > 1MHz)", spi1_t100k > spi1_t1m);

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
