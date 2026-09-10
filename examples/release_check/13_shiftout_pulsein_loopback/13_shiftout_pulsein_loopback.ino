/** Release check 13: automatic OK/FAIL checks for shiftOut/shiftIn/
 *  pulseIn/pulseInLong/random, plus the pulse timing of the bundled
 *  mcxRCServo library -- its own unique 5-jumper wiring, kept separate
 *  from the other release_check/ sketches.
 *
 *  Mirrors examples/Arduino_compatible_API/test_shiftOut_pulseIn_random
 *  in behavior (that sketch is already a single, self-contained, fully-
 *  checked test), but with the pins remapped to four adjacent pairs
 *  within D0-D7 -- each jumper spans just one pin gap on the header,
 *  instead of the original's scattered D5/D6/D7/D8/D9/D10/D11/D13.
 *
 *  Wiring needed:
 *    1) Jumper D0 (SHIFTOUT_DATA) <-> D1 (MONITOR_DATA)
 *       Jumper D2 (SHIFTOUT_CLOCK) <-> D3 (MONITOR_CLOCK)
 *       shiftOut() and shiftIn() both try to drive the clock themselves, so
 *       they can't loop back to each other directly on one MCU. Instead,
 *       attachInterrupt() on MONITOR_CLOCK's rising edge captures
 *       MONITOR_DATA in real time -- the same "digitalRead synced to a
 *       clock edge" mechanism shiftIn() itself uses internally -- so the
 *       byte shiftOut() actually sent can be verified bit-for-bit.
 *    2) Jumper D4 (SHIFTIN_DATA) <-> D5 (SHIFTIN_CLOCK)
 *       Standalone shiftIn() sanity check: shiftIn() drives CLOCK high
 *       right before sampling DATA, so with DATA tied directly to CLOCK,
 *       every sampled bit reads 1 -- shiftIn() should return 0xFF.
 *    3) Jumper D6 (TONE_PIN) <-> D7 (PULSE_MONITOR_PIN)
 *       tone(D6, 1000) is a 1kHz square wave (500us high / 500us low).
 *       pulseIn()/pulseInLong() on D7 should read ~500 (microseconds).
 *    4) Jumper PWM0 (SERVO_PWM_PIN) <-> D8 (SERVO_MONITOR_PIN)
 *       Pulse-width check for the bundled mcxRCServo library. No servo
 *       motor is needed -- the board measures its own output with
 *       pulseIn(), the same way item 3 does.
 *       This is the one part of mcxRCServo that #02 does not already
 *       cover. #02 drives analogWrite() at the default 8-bit resolution
 *       and analogWriteFrequency() at a fixed 50% duty; mcxRCServo
 *       instead calls analogWriteResolution(16) on a real FlexPWM pin
 *       and then asks for duty fractions of 2.5%-12% (0.5ms-2.4ms out
 *       of a 20ms period). Nothing else under release_check/ ever sets
 *       16-bit resolution on a PWM pin -- #01's analogWriteResolution()
 *       checks run on the non-PWM digitalWrite() fallback path instead.
 *       A scaling bug in there is silent: it compiles, CI stays green,
 *       nothing panics, and the shaft just sits at the wrong angle.
 */

#include <Arduino.h>
#include <SG90.h>          // bundled with this core: libraries/mcxRCServo

#define SHIFTOUT_DATA      D0
#define MONITOR_DATA       D1
#define SHIFTOUT_CLOCK     D2
#define MONITOR_CLOCK      D3
#define SHIFTIN_DATA       D4
#define SHIFTIN_CLOCK      D5
#define TONE_PIN           D6
#define PULSE_MONITOR_PIN  D7
#define SERVO_PWM_PIN      PWM0
#define SERVO_MONITOR_PIN  D8

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
  pinMode(SERVO_MONITOR_PIN, INPUT);

  Serial.println("=== Release check 13: shiftOut/shiftIn/pulseIn/random/servo (5-jumper) ===");

  // ---- shiftOut + interrupt-captured shiftIn-equivalent (D0<->D1, D2<->D3) ----
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

  // ---- shiftIn standalone sanity (D4<->D5 jumper, expect 0xFF) ----
  uint8_t sIn = shiftIn(SHIFTIN_DATA, SHIFTIN_CLOCK, MSBFIRST);
  Serial.print("shiftIn() = 0x");
  Serial.println(sIn, HEX);
  check("shiftIn() standalone sanity (DATA tied to CLOCK)", sIn == 0xFF);

  // ---- pulseIn / pulseInLong (D6<->D7 jumper, tone 1kHz) ----
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

  // ---- mcxRCServo pulse timing (PWM0<->D8 jumper, no servo needed) ----
  // SG90's datasheet pulse range is 0.5ms..2.4ms high out of a 20ms
  // period (50Hz). mcxRCServo maps its default 0.0..1.0 user range onto
  // that, so the three points below are the two ends and the middle of
  // the travel. Expected widths follow straight from those numbers, so
  // they stay right even if the core's duty scaling changes underneath.
  {
    SG90 servo(SERVO_PWM_PIN);

    struct { double pos; unsigned long expect_us; const char *label; } points[] = {
      { 0.0,  500, "mcxRCServo position(0.0) -> ~500us pulse"  },
      { 0.5, 1450, "mcxRCServo position(0.5) -> ~1450us pulse" },
      { 1.0, 2400, "mcxRCServo position(1.0) -> ~2400us pulse" },
    };

    for (auto &p : points) {
      servo.position(p.pos);
      delay(50);  // let at least one 20ms period run out at the new duty

      unsigned long w = pulseIn(SERVO_MONITOR_PIN, HIGH, 100000UL);
      Serial.print("position(");
      Serial.print(p.pos);
      Serial.print(") pulse us = ");
      Serial.println(w);
      check(p.label, w > p.expect_us - 50 && w < p.expect_us + 50);
    }
  }

  // Put the analogWrite() settings back: the servo constructor changed
  // resolution and frequency globally, not just for its own pin, so
  // leaving them set would silently reinterpret any later analogWrite().
  analogWriteResolution(8);
  analogWrite(SERVO_PWM_PIN, 0);
  analogWriteFrequency(SERVO_PWM_PIN, 1000);

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
