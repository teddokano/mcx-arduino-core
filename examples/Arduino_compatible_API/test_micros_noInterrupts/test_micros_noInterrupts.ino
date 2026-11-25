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
 *  Then the debugger's cycle counter (DWT CYCCNT), which micros(), delay()
 *  and delayMicroseconds() counted with on the Cortex-M33 boards until
 *  0.14.0: a debugger clears DEMCR.TRCENA, which stops it, as it lets go of
 *  a running sketch (probe-rs does, and right after an upload to
 *  FRDM-MCXA153 or FRDM-MCXA156 that lands about 100ms after the sketch
 *  starts), and one that measures cycles writes it back. Until 0.13.0
 *  micros() stood still with interrupts off then, and a wait on it never
 *  ended; until 0.14.0 a counter written back made millis() jump by tens
 *  of seconds. They now count SysTick on every board. The sketch does to
 *  the counter what the debugger would.
 *
 *  And a SysTick tick held back by up to 0.9ms with interrupts off: until
 *  0.14.0 micros() then read a millisecond short once the tick was over
 *  half a millisecond late (on FRDM-MCXC444; from 0.14.0 every board
 *  reads micros() the same way).
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

  //  5: with interrupts off, micros() keeps counting after DEMCR.TRCENA is
  //     cleared, as a debugger does when it lets go. (delay() and
  //     delayMicroseconds() count SysTick as micros() does.)
#if defined(__ARM_ARCH_6M__)
  Serial.println("        (FRDM-MCXC444 counts with SysTick, not the cycle counter)");
#else
  noInterrupts();
  CoreDebug->DEMCR = 0;
  unsigned long t0 = micros();
  unsigned long spins = 0;
  while (micros() - t0 < 500 && spins < 10000000UL)
    spins++;
  unsigned long waited = micros() - t0;
  interrupts();
  Serial.print("        micros() counted ");
  Serial.print(waited);
  Serial.print("us in ");
  Serial.print(spins);
  Serial.println(" reads");
  check("micros() keeps counting after DEMCR is cleared, interrupts off",
        waited >= 500 && spins < 10000000UL);
#endif

  //  6: a tick held back by interrupts for up to 0.9ms, at 40 different
  //     places in the millisecond: micros() reads the 900us right
  unsigned long late_min = 0xFFFFFFFFUL, late_max = 0;
  for (int i = 0; i < 40; i++) {
    delayMicroseconds((i * 37) % 1000);
    noInterrupts();
    unsigned long t0 = micros();
    delayMicroseconds(900);
    unsigned long t1 = micros();
    interrupts();
    unsigned long d = t1 - t0;
    if (d < late_min)
      late_min = d;
    if (d > late_max)
      late_max = d;
  }
  Serial.print("        900us with the tick held back: ");
  Serial.print(late_min);
  Serial.print(" - ");
  Serial.print(late_max);
  Serial.println("us");
  check("micros() reads 900us as 895 - 960us with the tick held back",
        late_min >= 895 && late_max <= 960);

  //  7: the cycle counter written back, as a debugger that measures cycles
  //     does: millis() takes no notice
#if defined(__ARM_ARCH_6M__)
  Serial.println("        (FRDM-MCXC444 has no cycle counter)");
#else
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  unsigned long m0 = millis();
  DWT->CYCCNT = 0;
  delay(20);
  unsigned long m1 = millis();
  DWT->CYCCNT = 0x80000000UL;
  delay(20);
  unsigned long m2 = millis();
  Serial.print("        millis() over delay(20) with the counter written back: ");
  Serial.print(m1 - m0);
  Serial.print(", ");
  Serial.println(m2 - m1);
  check("millis() counts 19 - 22ms over delay(20) with the cycle counter written back",
        m1 - m0 >= 19 && m1 - m0 <= 22 && m2 - m1 >= 19 && m2 - m1 <= 22);
#endif

  //  8: interrupts on again after all of it
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
