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
 *       On FRDM-MCXN236, PWM0 is D6 itself, so jumper 3 already carries
 *       the servo pulses to D7 and the sketch measures them there: no
 *       jumper 4 on that board.
 *    5) No new jumper: the bundled Servo library on pins without PWM,
 *       whose pulses come from a timer interrupt (CTIMER2, CTIMER3 on
 *       FRDM-MCXN947, TPM2 on FRDM-MCXC444) raising and lowering the pin.
 *       A servo on D2 is measured on D3 through jumper 1's clock pair, and
 *       one on D4 on D5 through jumper 2. Each edge's time is taken with
 *       micros() in a pin interrupt: the widths, the 20ms period, the
 *       D4 pulse starting as the D2 pulse ends (one after another, as on
 *       AVR), and detach() stopping the pulses.
 *
 *  FRDM-MCXC444: D0/D1 are Serial there (the USB serial port's own lines,
 *  driven by the MCU-LINK), so jumper 1's data pair moves to D8 <-> D9.
 *  It has no PWM0-PWM5: the servo pulses come out of D6 (TPM0_CH2) and
 *  jumper 3 carries them to D7, as on FRDM-MCXN236. So four jumpers:
 *  D8-D9, D2-D3, D4-D5, D6-D7. The blue LED (D6) flickers meanwhile.
 */

#include <Arduino.h>
#include <SG90.h>          // bundled with this core: libraries/mcxRCServo
#include <Servo.h>         // bundled with this core: libraries/Servo

#if defined(FRDM_MCXC444)
#define SHIFTOUT_DATA      D8   // D0/D1 are Serial there
#define MONITOR_DATA       D9
#else
#define SHIFTOUT_DATA      D0
#define MONITOR_DATA       D1
#endif
#define SHIFTOUT_CLOCK     D2
#define MONITOR_CLOCK      D3
#define SHIFTIN_DATA       D4
#define SHIFTIN_CLOCK      D5
#define TONE_PIN           D6
#define PULSE_MONITOR_PIN  D7
#if defined(FRDM_MCXC444)
#define SERVO_PWM_PIN      D6   // no PWM0-PWM5 there: D6 (TPM0_CH2)
#else
#define SERVO_PWM_PIN      PWM0
#endif
#if defined(FRDM_MCXN236) || defined(FRDM_MCXC444)
#define SERVO_MONITOR_PIN  D7   // the servo pin is D6 here: jumper 3 carries it
#elif defined(FRDM_MCXA153) || defined(FRDM_MCXA156) || defined(FRDM_MCXN947)
#define SERVO_MONITOR_PIN  D8
#else
#error "This sketch has no settings for this board yet"
#endif

// ---- Servo edge capture (item 5) ----
#define TIMED_SERVO_A      SHIFTOUT_CLOCK   // D2, no PWM on any board
#define TIMED_MONITOR_A    MONITOR_CLOCK    // D3
#define TIMED_SERVO_B      SHIFTIN_DATA     // D4, no PWM on any board
#define TIMED_MONITOR_B    SHIFTIN_CLOCK    // D5

struct Edge { unsigned long t; uint8_t which; uint8_t level; };
const int MAX_EDGES = 64;
volatile Edge edges[MAX_EDGES];
volatile int  edgeCount = 0;
volatile bool capturing = false;

struct PulseStats {
  int pulses;              // complete high pulses seen
  unsigned long minW, maxW;
  int periods;             // rise-to-rise intervals seen
  unsigned long minP, maxP;
};

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

void recordEdge(uint8_t which, int pin) {
  unsigned long t = micros();
  if (!capturing || edgeCount >= MAX_EDGES)
    return;
  edges[edgeCount].t     = t;
  edges[edgeCount].which = which;
  edges[edgeCount].level = digitalRead(pin) ? 1 : 0;
  edgeCount = edgeCount + 1;
}

void onTimedA() { recordEdge(0, TIMED_MONITOR_A); }
void onTimedB() { recordEdge(1, TIMED_MONITOR_B); }

// Edges for ms milliseconds, after the pulses in flight have settled
void captureEdges(unsigned long ms) {
  delay(45);  // past two 20ms frames, so a width just written is in effect
  noInterrupts();
  edgeCount = 0;
  capturing = true;
  interrupts();
  delay(ms);
  capturing = false;
}

PulseStats statsFor(uint8_t which) {
  PulseStats s = { 0, 0xFFFFFFFFUL, 0, 0, 0xFFFFFFFFUL, 0 };
  long lastRise = -1;
  for (int i = 0; i < edgeCount; i++) {
    if (edges[i].which != which)
      continue;
    if (edges[i].level) {
      if (lastRise >= 0) {
        unsigned long p = edges[i].t - edges[lastRise].t;
        s.periods++;
        if (p < s.minP) s.minP = p;
        if (p > s.maxP) s.maxP = p;
      }
      lastRise = i;
    } else if (lastRise >= 0) {
      unsigned long w = edges[i].t - edges[lastRise].t;
      s.pulses++;
      if (w < s.minW) s.minW = w;
      if (w > s.maxW) s.maxW = w;
    }
  }
  return s;
}

void printStats(const char *name, const PulseStats &s) {
  Serial.print(name);
  Serial.print(": "); Serial.print(s.pulses); Serial.print(" pulses ");
  Serial.print(s.minW); Serial.print("-"); Serial.print(s.maxW);
  Serial.print("us, "); Serial.print(s.periods); Serial.print(" periods ");
  Serial.print(s.minP); Serial.print("-"); Serial.print(s.maxP);
  Serial.println("us");
}

bool widthsNear(const PulseStats &s, unsigned long us) {
  return s.pulses >= 3 && s.minW + 15 >= us && s.maxW <= us + 15;
}

bool periodsNear20ms(const PulseStats &s) {
  return s.periods >= 3 && s.minP + 30 >= 20000UL && s.maxP <= 20030UL;
}

void checkTimedServos() {
  pinMode(TIMED_MONITOR_B, INPUT);  // was shiftIn()'s clock output: the servo on D4 drives it now
  pinMode(TIMED_MONITOR_A, INPUT);
  detachInterrupt(digitalPinToInterrupt(TIMED_MONITOR_A));
  attachInterrupt(digitalPinToInterrupt(TIMED_MONITOR_A), onTimedA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(TIMED_MONITOR_B), onTimedB, CHANGE);

  Servo a, b;

  a.attach(TIMED_SERVO_A);
  check("Servo on D2 (no PWM): attached()", a.attached());

  struct { int value; unsigned long expect_us; const char *label; } points[] = {
    {    0,  544, "Servo on D2: write(0) -> 544us pulses at 20ms"   },
    {   90, 1472, "Servo on D2: write(90) -> 1472us pulses at 20ms" },
    {  180, 2400, "Servo on D2: write(180) -> 2400us pulses at 20ms" },
    { 1000, 1000, "Servo on D2: write(1000) -> 1000us pulses at 20ms" },
  };

  for (auto &p : points) {
    a.write(p.value);
    captureEdges(110);
    PulseStats s = statsFor(0);
    printStats("  D3", s);
    check(p.label, widthsNear(s, p.expect_us) && periodsNear20ms(s));
  }

  // Two servos: D4's pulse follows D2's, one after the other
  b.attach(TIMED_SERVO_B);
  b.writeMicroseconds(2000);
  captureEdges(110);
  PulseStats sa = statsFor(0);
  PulseStats sb = statsFor(1);
  printStats("  D3", sa);
  printStats("  D5", sb);
  check("Two servos: D2 still 1000us at 20ms", widthsNear(sa, 1000) && periodsNear20ms(sa));
  check("Two servos: D4 2000us at 20ms", widthsNear(sb, 2000) && periodsNear20ms(sb));

  // Each D5 rise against the D3 fall nearest in time: the two pin
  // interrupts may record the edges in either order
  int follows = 0, pairs = 0;
  unsigned long worstGap = 0;
  for (int i = 0; i < edgeCount; i++) {
    if (edges[i].which != 1 || !edges[i].level)
      continue;
    unsigned long nearest = 0xFFFFFFFFUL;
    for (int j = 0; j < edgeCount; j++) {
      if (edges[j].which != 0 || edges[j].level)
        continue;
      unsigned long d = edges[i].t >= edges[j].t ? edges[i].t - edges[j].t : edges[j].t - edges[i].t;
      if (d < nearest)
        nearest = d;
    }
    pairs++;
    if (nearest <= 20)
      follows++;
    if (nearest > worstGap)
      worstGap = nearest;
  }
  Serial.print("  D4 rises within 20us of a D2 fall: ");
  Serial.print(follows); Serial.print(" of "); Serial.print(pairs);
  Serial.print(" (farthest "); Serial.print(worstGap); Serial.println("us)");
  check("Two servos: D4's pulse starts as D2's ends", pairs >= 3 && follows == pairs);

  // detach() stops D2's pulses and leaves it LOW; D4's go on
  a.detach();
  captureEdges(110);
  sa = statsFor(0);
  sb = statsFor(1);
  printStats("  D3", sa);
  printStats("  D5", sb);
  check("detach(): D2 stops, LOW", !a.attached() && sa.pulses == 0 && sa.periods == 0 && digitalRead(TIMED_MONITOR_A) == LOW);
  check("detach(): D4 goes on at 2000us, 20ms", widthsNear(sb, 2000) && periodsNear20ms(sb));

  // The last detach() stops the timer after its frame
  b.detach();
  captureEdges(110);
  check("Both detached: no pulses", edgeCount == 0 && digitalRead(TIMED_MONITOR_B) == LOW);

  // attach() again starts the timer again, with the width written before
  a.attach(TIMED_SERVO_A);
  captureEdges(110);
  sa = statsFor(0);
  printStats("  D3", sa);
  check("attach() again: D2 1000us at 20ms", widthsNear(sa, 1000) && periodsNear20ms(sa));
  a.detach();

  detachInterrupt(digitalPinToInterrupt(TIMED_MONITOR_A));
  detachInterrupt(digitalPinToInterrupt(TIMED_MONITOR_B));
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

  // ---- Servo on pins without PWM (D2->D3, D4->D5, no new jumper) ----
  checkTimedServos();

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
