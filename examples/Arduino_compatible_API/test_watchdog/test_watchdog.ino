/*
 *  test_watchdog: <avr/wdt.h> on this board's watchdog. No wiring.
 *
 *  Resets the board on purpose, twice, and checks after each reset that
 *  MCUSR says it was the watchdog's:
 *
 *    A  wdt_enable(WDTO_250MS), fed every 100ms for a second, then not:
 *       a reset, with WDRF set
 *    B  wdt_enable() then wdt_disable(): no reset in 1.5 seconds
 *    C  wdt_enable(WDTO_500MS), then panic(): its message, then a reset
 *       with WDRF set
 *
 *  The steps go on across the resets through a record in RAM that a reset
 *  doesn't clear (.noinit). A reset that isn't the watchdog's in the
 *  middle (RESET pressed, a new upload) starts them over, saying so.
 *  The same checks as release_check/0A_watchdog. Ends with ALL OK or the
 *  number of failures, about 5 seconds from the start.
 *
 *  Open the Serial Monitor at 115200 baud.
 */

#include <Arduino.h>
#include <avr/wdt.h>

enum Step : uint32_t { STEP_START, STEP_A_WAIT, STEP_C_WAIT, STEP_DONE };

struct Record {
  uint32_t magic;
  uint32_t step;
  uint32_t failures;
  uint32_t check;
};
__attribute__((section(".noinit"))) Record rec;
const uint32_t MAGIC = 0x57445431;  // "WDT1"

bool record_ok() {
  return rec.magic == MAGIC && rec.check == (rec.step ^ rec.failures ^ 0xA5A5A5A5);
}

// Every change to the record goes through here, so that check stays right
void record_save(uint32_t step) {
  rec.magic = MAGIC;
  rec.step = step;
  rec.check = rec.step ^ rec.failures ^ 0xA5A5A5A5;
}

void check(const char *what, bool ok) {
  Serial.print(ok ? "OK: " : "FAIL: ");
  Serial.println(what);
  if (!ok) {
    rec.failures++;
    record_save(rec.step);
  }
}

void step_a() {
  Serial.println("A: wdt_enable(WDTO_250MS), fed for 1 s, then not");
  wdt_enable(WDTO_250MS);
  for (int i = 0; i < 10; i++) {  // 100ms apart: well within 250ms
    delay(100);
    wdt_reset();
  }
  check("A: still running after 1 s of feeding", true);
  record_save(STEP_A_WAIT);
  Serial.flush();
  delay(2000);  // the watchdog resets the board in here
  record_save(STEP_START);
  check("A: the watchdog reset the board within 2 s", false);
  finish();
}

void step_b() {
  Serial.println("B: wdt_enable(WDTO_250MS), then wdt_disable()");
  wdt_enable(WDTO_250MS);
  wdt_disable();
  delay(1500);
  check("B: no reset 1.5 s after wdt_disable()", true);
}

void step_c() {
  Serial.println("C: wdt_enable(WDTO_500MS), then panic()");
  wdt_enable(WDTO_500MS);
  record_save(STEP_C_WAIT);
  Serial.flush();
  panic("test_watchdog: this panic should end in a watchdog reset");
}

void finish() {
  record_save(STEP_DONE);
  Serial.println();
  if (rec.failures == 0) {
    Serial.println("ALL OK");
  } else {
    Serial.print(rec.failures);
    Serial.println(" FAILED");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  // Read before anything changes the record
  bool ok = record_ok();
  uint32_t step = rec.step;
  uint8_t flags = MCUSR;
  bool by_wdt = flags & _BV(WDRF);

  Serial.print("\ntest_watchdog. MCUSR: 0x");
  Serial.print(flags, HEX);
  Serial.print((flags & _BV(PORF)) ? " PORF" : "");
  Serial.print((flags & _BV(EXTRF)) ? " EXTRF" : "");
  Serial.print((flags & _BV(BORF)) ? " BORF" : "");
  Serial.println((flags & _BV(WDRF)) ? " WDRF" : "");

  if (!ok || step == STEP_DONE || !by_wdt) {
    if (ok && step != STEP_DONE)
      Serial.println("A reset without WDRF in the middle of the steps: starting over "
                     "(if RESET wasn't pressed and nothing was uploaded, the watchdog's reset didn't show in MCUSR)");
    rec.failures = 0;
    record_save(STEP_START);
    MCUSR = 0;
    check("MCUSR = 0 clears the copy", MCUSR == 0);
    step_a();
    return;
  }

  if (step == STEP_A_WAIT) {
    check("A: the reset was the watchdog's (WDRF)", by_wdt);
    step_b();
    step_c();
    return;
  }

  if (step == STEP_C_WAIT) {
    check("C: panic() ended in a watchdog reset (WDRF)", by_wdt);
    finish();
  }
}

void loop() {
}
