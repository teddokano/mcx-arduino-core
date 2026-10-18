/** micros() leaves interrupts as it found them.
 *
 *  micros() turns interrupts off for a moment to read the millisecond
 *  count and the timer together. Up to 0.9.1 it turned them back on
 *  afterwards whatever they were before, so micros() inside
 *  noInterrupts() ... interrupts() quietly let interrupts in: a timing
 *  loop built on micros() with interrupts off was cut into by SysTick
 *  and every other interrupt.
 *
 *  The check calls micros() with interrupts off for 3ms. Interrupts that
 *  stay off keep SysTick from counting, so millis() has to read the same
 *  before and after. Then micros() has to keep counting normally with
 *  interrupts on.
 *
 *  Wiring: none.
 *
 *  Automatic: reads "ALL OK" or "N FAILED" at the end.
 */

#include <Arduino.h>

int failures = 0;

void check(const char *name, bool ok) {
  Serial.print(ok ? "  OK    " : "  FAIL  ");
  Serial.println(name);
  if (!ok)
    failures++;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(100);

  Serial.println("\r\nmicros() and noInterrupts()");

  //  1: one call leaves PRIMASK set
  noInterrupts();
  (void)micros();
  bool still_off = (__get_PRIMASK() & 1) != 0;
  interrupts();
  check("micros() with interrupts off leaves them off", still_off);

  //  2: 3ms of micros() with interrupts off: no SysTick, so millis() stands
  noInterrupts();
  unsigned long ms_before = millis();
  unsigned long calls = 0;
  for (int i = 0; i < 300; i++) {
    (void)micros();
    calls++;
    delayMicroseconds(10);
  }
  unsigned long ms_after = millis();
  bool off_after_loop = (__get_PRIMASK() & 1) != 0;
  interrupts();
  Serial.print("        millis() before ");
  Serial.print(ms_before);
  Serial.print(", after ");
  Serial.print(ms_after);
  Serial.print(" (");
  Serial.print(calls);
  Serial.println(" calls)");
  check("millis() stands still through 3ms of micros() with interrupts off",
        ms_after == ms_before && off_after_loop);

  //  3: with interrupts on, micros() still counts and keeps up with millis()
  delay(2);
  unsigned long us0 = micros();
  unsigned long ms0 = millis();
  delay(100);
  unsigned long us1 = micros();
  unsigned long ms1 = millis();
  Serial.print("        delay(100): micros() ");
  Serial.print(us1 - us0);
  Serial.print(", millis() ");
  Serial.println(ms1 - ms0);
  check("micros() counts 100ms as 99000 - 101000us",
        us1 - us0 >= 99000 && us1 - us0 <= 101000);

  //  4: never runs backwards across many ticks
  bool monotonic = true;
  unsigned long prev = micros();
  unsigned long end = millis() + 200;
  while ((long)(millis() - end) < 0) {
    unsigned long now = micros();
    if (now < prev)
      monotonic = false;
    prev = now;
  }
  check("micros() never runs backwards over 200ms", monotonic);

  //  5: interrupts on again after all of it
  check("interrupts are on at the end", (__get_PRIMASK() & 1) == 0);

  Serial.println();
  if (failures == 0) {
    Serial.println("ALL OK");
  } else {
    Serial.print(failures);
    Serial.println(" FAILED");
  }
}

void loop() {}
