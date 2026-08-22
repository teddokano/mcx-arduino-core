/** Release check 6/N: automatic OK/FAIL checks for shiftOut/shiftIn/
 *  pulseIn/pulseInLong/random -- its own unique 4-jumper wiring, kept
 *  separate from the other release_check/ sketches.
 *
 *  Mirrors examples/Arduino_compatible_API/test_shiftOut_pulseIn_random
 *  exactly (that sketch is already a single, self-contained, fully-
 *  checked test -- copied here so every release_check/ group lives
 *  together in one place).
 *
 *  Wiring needed:
 *    1) Jumper D5 (SHIFTOUT_DATA) <-> D8 (MONITOR_DATA)
 *       Jumper D6 (SHIFTOUT_CLOCK) <-> D9 (MONITOR_CLOCK)
 *       shiftOut() and shiftIn() both try to drive the clock themselves, so
 *       they can't loop back to each other directly on one MCU. Instead,
 *       attachInterrupt() on MONITOR_CLOCK's rising edge captures
 *       MONITOR_DATA in real time -- the same "digitalRead synced to a
 *       clock edge" mechanism shiftIn() itself uses internally -- so the
 *       byte shiftOut() actually sent can be verified bit-for-bit.
 *    2) Jumper D10 (SHIFTIN_DATA) <-> D11 (SHIFTIN_CLOCK)
 *       Standalone shiftIn() sanity check: shiftIn() drives CLOCK high
 *       right before sampling DATA, so with DATA tied directly to CLOCK,
 *       every sampled bit reads 1 -- shiftIn() should return 0xFF.
 *    3) Jumper D13 (TONE_PIN) <-> D7 (PULSE_MONITOR_PIN)
 *       tone(D13, 1000) is a 1kHz square wave (500us high / 500us low).
 *       pulseIn()/pulseInLong() on D7 should read ~500 (microseconds).
 */

#include <Arduino.h>

#define SHIFTOUT_DATA      D5
#define SHIFTOUT_CLOCK     D6
#define MONITOR_DATA       D8
#define MONITOR_CLOCK      D9
#define SHIFTIN_DATA       D10
#define SHIFTIN_CLOCK      D11
#define TONE_PIN           D13
#define PULSE_MONITOR_PIN  D7

volatile uint8_t captured     = 0;
volatile uint8_t bitsCaptured = 0;

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

void onMonitorClockRise() {
  captured = (uint8_t)((captured << 1) | (digitalRead(MONITOR_DATA) ? 1 : 0));
  bitsCaptured = bitsCaptured + 1;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(SHIFTOUT_DATA, OUTPUT);
  pinMode(SHIFTOUT_CLOCK, OUTPUT);
  pinMode(MONITOR_DATA, INPUT);
  pinMode(MONITOR_CLOCK, INPUT);
  pinMode(SHIFTIN_DATA, INPUT);
  pinMode(SHIFTIN_CLOCK, OUTPUT);
  pinMode(PULSE_MONITOR_PIN, INPUT);

  Serial.println("=== Release check 6/N: shiftOut/shiftIn/pulseIn/random (4-jumper) ===");

  // ---- shiftOut + interrupt-captured shiftIn-equivalent (D5<->D8, D6<->D9) ----
  attachInterrupt(digitalPinToInterrupt(MONITOR_CLOCK), onMonitorClockRise, RISING);

  uint8_t sent = 0xB6;
  captured     = 0;
  bitsCaptured = 0;

  shiftOut(SHIFTOUT_DATA, SHIFTOUT_CLOCK, MSBFIRST, sent);
  delay(5);  // let the last interrupt land

  Serial.print("sent     = 0x"); Serial.println(sent, HEX);
  Serial.print("captured = 0x"); Serial.println(captured, HEX);
  Serial.print("bits     = "); Serial.println(bitsCaptured);
  check("shiftOut() bit-for-bit via interrupt capture", captured == sent && bitsCaptured == 8);

  // ---- shiftIn standalone sanity (D10<->D11 jumper, expect 0xFF) ----
  uint8_t sIn = shiftIn(SHIFTIN_DATA, SHIFTIN_CLOCK, MSBFIRST);
  Serial.print("shiftIn() = 0x");
  Serial.println(sIn, HEX);
  check("shiftIn() standalone sanity (DATA tied to CLOCK)", sIn == 0xFF);

  // ---- pulseIn / pulseInLong (D13<->D7 jumper, tone 1kHz) ----
  tone(TONE_PIN, 1000);
  unsigned long w1 = pulseIn(PULSE_MONITOR_PIN, HIGH, 100000UL);
  Serial.print("pulseIn(HIGH) us = ");
  Serial.println(w1);
  check("pulseIn(HIGH) ~500us (1kHz square wave)", w1 > 450 && w1 < 550);

  unsigned long w2 = pulseInLong(PULSE_MONITOR_PIN, HIGH, 100000UL);
  Serial.print("pulseInLong(HIGH) us = ");
  Serial.println(w2);
  check("pulseInLong(HIGH) ~500us (1kHz square wave)", w2 > 450 && w2 < 550);
  noTone(TONE_PIN);

  // ---- random / randomSeed ----
  // Not checked against exact values (that would couple this test to the
  // specific libc rand() sequence behind random()) -- instead checked
  // against random()'s actual contract: results stay within the
  // requested range, and aren't all identical (a degenerate/stuck PRNG).
  randomSeed(42);

  bool inRange100 = true;
  bool varies100  = false;
  int  first100   = -1;
  for (int i = 0; i < 5; i++) {
    int v = random(100);
    Serial.print("random(100) = ");
    Serial.println(v);
    if (v < 0 || v >= 100)
      inRange100 = false;
    if (i == 0)
      first100 = v;
    else if (v != first100)
      varies100 = true;
  }
  check("random(100) stays within [0,100)", inRange100);
  check("random(100) doesn't stick on one value", varies100);

  bool inRange1020 = true;
  bool varies1020  = false;
  int  first1020   = -1;
  for (int i = 0; i < 5; i++) {
    int v = random(10, 20);
    Serial.print("random(10,20) = ");
    Serial.println(v);
    if (v < 10 || v >= 20)
      inRange1020 = false;
    if (i == 0)
      first1020 = v;
    else if (v != first1020)
      varies1020 = true;
  }
  check("random(10,20) stays within [10,20)", inRange1020);
  check("random(10,20) doesn't stick on one value", varies1020);

  Serial.println();
  Serial.println(failCount == 0 ? "ALL OK" : "SOME FAILED");
}

void loop() {
}
