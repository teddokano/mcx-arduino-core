/** The bundled Servo library on every pin that has no PWM. No wiring, no
 *  servo motor: each pin is read back through its own GPIO input register
 *  (PDIR), which follows the pad, not through digitalRead() -- on a pin set
 *  to OUTPUT that returns the value last written.
 *
 *  Since v0.12.0 a pin without PWM gets its pulses from a timer interrupt
 *  that raises and lowers the pin through the GPIO's PSOR/PCOR. The pin
 *  tables and the pin's mux are per pin, so a check on D2 and D4
 *  (release_check/13) says nothing about the others. This one goes through
 *  all of the board's D pins and A pins:
 *    - the pad follows digitalWrite(HIGH/LOW) (a pin that doesn't can't be
 *      measured; that is reported on its own)
 *    - after analogRead(), for an A pin that has an ADC channel
 *    - attach(): 1000us and 2000us pulses, 20ms period
 *    - detach(): the pin stops at LOW, and digitalWrite() works again
 *  then up to MAX_SERVOS of the pins that passed run together: 1000us each
 *  (20ms period), 2000us each (back to back, so 12 x 2ms + 10us), and a
 *  different width on each, to catch two pins swapped.
 *
 *  Pins with hardware PWM are skipped here: test_Servo and release_check/13
 *  check those. The pulses are timed by polling, so a few microseconds of
 *  jitter is normal; the limits are 15us for a width, 100us for a period.
 *
 *  FRDM-MCXC444: D0 and D1 are left out, they are the USB serial port.
 *  FRDM-MCXN947: A0 and A1 have no pin routed to them.
 *
 *  Read the final "ALL PASS"/"N FAIL" line.
 */

#include <Arduino.h>
#include <Servo.h>
#include "types.h"  // the Arduino IDE puts the functions' prototypes above any struct in the sketch itself

struct PinInfo {
  int pin;
  const char *name;
  bool adc;  // analogRead() works on it
};

#define DP(p) { p, #p, false }
#define AP(p) { p, #p, true }

const PinInfo pins[] = {
#if !defined(FRDM_MCXC444)
  DP(D0), DP(D1),
#endif
  DP(D2), DP(D3), DP(D4), DP(D5), DP(D6), DP(D7), DP(D8), DP(D9), DP(D10), DP(D11), DP(D12), DP(D13),
  DP(D18), DP(D19),
#if defined(FRDM_MCXN947)
  AP(A2), AP(A3), AP(A4), AP(A5),
#elif defined(FRDM_MCXA153)
  AP(A0), AP(A1), AP(A2), AP(A3), DP(A4), DP(A5),
#elif defined(FRDM_MCXA156) || defined(FRDM_MCXN236) || defined(FRDM_MCXC444)
  AP(A0), AP(A1), AP(A2), AP(A3), AP(A4), AP(A5),
#else
#error "No pin list for this board yet"
#endif
};
const int PIN_COUNT = sizeof(pins) / sizeof(pins[0]);

const long WIDTH_TOLERANCE  = 15;
const long PERIOD_TOLERANCE = 100;

Servo servos[MAX_SERVOS];

int fails = 0;

bool high(const Pad &p) {
  return (*p.pdir & p.mask) != 0;
}

bool wait_for(const Pad &p, bool level, uint32_t timeout_us) {
  uint32_t t0 = micros();
  while (high(p) != level)
    if (micros() - t0 > timeout_us)
      return false;
  return true;
}

// Time `count` pulses on the pad
Stats measure(const Pad &p, int count) {
  Stats s = { 0, 1L << 30, 0, 1L << 30, 0 };

  if (!wait_for(p, false, 30000UL) || !wait_for(p, true, 60000UL))
    return s;

  uint32_t rise = micros();
  for (int i = 0; i < count; i++) {
    if (!wait_for(p, false, 5000UL))
      return s;
    long w = (long)(micros() - rise);
    if (!wait_for(p, true, 60000UL))
      return s;
    uint32_t next = micros();
    long per = (long)(next - rise);
    rise = next;
    s.pulses++;
    s.wmin = min(s.wmin, w);
    s.wmax = max(s.wmax, w);
    s.pmin = min(s.pmin, per);
    s.pmax = max(s.pmax, per);
  }
  return s;
}

void print_stats(const Stats &s) {
  Serial.print("width ");
  Serial.print(s.wmin);
  Serial.print("..");
  Serial.print(s.wmax);
  Serial.print("us, period ");
  Serial.print(s.pmin);
  Serial.print("..");
  Serial.print(s.pmax);
  Serial.print("us");
}

bool width_ok(const Stats &s, long expect, int count) {
  return s.pulses >= count && s.wmin >= expect - WIDTH_TOLERANCE && s.wmax <= expect + WIDTH_TOLERANCE;
}

bool period_ok(const Stats &s, long expect) {
  return s.pulses > 0 && s.pmin >= expect - PERIOD_TOLERANCE && s.pmax <= expect + PERIOD_TOLERANCE;
}

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    fails++;
}

bool pad_follows(const Pad &p, int pin) {
  digitalWrite(pin, HIGH);
  delayMicroseconds(20);
  bool h = high(p);
  digitalWrite(pin, LOW);
  delayMicroseconds(20);
  bool l = !high(p);
  return h && l;
}

Pad pads[PIN_COUNT];
bool passed[PIN_COUNT];

void one_pin(int i) {
  const PinInfo &pi = pins[i];

  Serial.print(pi.name);
  Serial.print(": ");

  if (digitalPinHasPWM(pi.pin)) {
    Serial.println("SKIP (hardware PWM, see test_Servo)");
    return;
  }

  if (pi.adc)
    analogRead(pi.pin);  // the ADC takes the pad's input buffer away; the pin has to get it back

  pinMode(pi.pin, OUTPUT);
  digitalWrite(pi.pin, LOW);

  GPIO_Type *port = digitalPinToPort(pi.pin);
  if (port == nullptr) {
    check("no GPIO port for this pin", false);
    return;
  }
  Pad pad = { portInputRegister(port), digitalPinToBitMask(pi.pin) };
  pads[i] = pad;

  if (!pad_follows(pad, pi.pin)) {
    check("the pad does not follow digitalWrite() (nothing to measure)", false);
    return;
  }

  bool ok = true;
  Servo &s = servos[0];

  ok = ok && (s.attach(pi.pin) == 0);

  s.writeMicroseconds(1000);
  delay(60);
  Stats a = measure(pad, 5);
  ok = ok && width_ok(a, 1000, 5) && period_ok(a, 20000);

  s.writeMicroseconds(2000);
  delay(60);
  Stats b = measure(pad, 5);
  ok = ok && width_ok(b, 2000, 5) && period_ok(b, 20000);

  s.detach();
  delay(30);
  bool stopped = !high(pad) && !wait_for(pad, true, 45000UL);

  digitalWrite(pi.pin, HIGH);
  delayMicroseconds(20);
  bool again = high(pad);
  digitalWrite(pi.pin, LOW);
  delayMicroseconds(20);
  again = again && !high(pad);

  Serial.print("1000us: ");
  print_stats(a);
  Serial.print("; 2000us: ");
  print_stats(b);
  Serial.println();
  Serial.print("  ");
  check("  attach, widths and period", ok);
  Serial.print("  ");
  check("  detach: LOW, no more pulses, digitalWrite() works", stopped && again);

  passed[i] = ok && stopped && again;
}

void together() {
  int chosen[MAX_SERVOS];
  int n = 0;
  for (int i = 0; i < PIN_COUNT && n < MAX_SERVOS; i++)
    if (passed[i])
      chosen[n++] = i;

  Serial.print("--- ");
  Serial.print(n);
  Serial.println(" servos together ---");
  check("at least 12 pins to run together", n == MAX_SERVOS);
  if (n == 0)
    return;

  for (int k = 0; k < n; k++)
    servos[k].attach(pins[chosen[k]].pin, 500, 2400);

  struct Case {
    const char *label;
    long (*width)(int k);
    long period;
  };
  static const Case cases[] = {
    { "1000us each", [](int) -> long { return 1000; }, 20000 },
    { "2000us each (frame stretches past 20ms)", [](int) -> long { return 2000; }, -1 },
    { "1000us + 50us per pin", [](int k) -> long { return 1000 + 50 * k; }, 20000 },
  };

  for (const Case &c : cases) {
    for (int k = 0; k < n; k++)
      servos[k].writeMicroseconds(c.width(k));
    delay(150);

    bool wok = true, pok = true;
    long expect_period = c.period;
    if (expect_period < 0)  // back to back, then 10us before the next frame
      expect_period = 2000L * n + 10;

    Serial.println(c.label);
    for (int k = 0; k < n; k++) {
      Stats st = measure(pads[chosen[k]], 4);
      bool w = width_ok(st, c.width(k), 4);
      bool p = period_ok(st, expect_period);
      wok = wok && w;
      pok = pok && p;
      if (!w || !p) {
        Serial.print("  ");
        Serial.print(pins[chosen[k]].name);
        Serial.print(": ");
        print_stats(st);
        Serial.print(" (expect ");
        Serial.print(c.width(k));
        Serial.print(", ");
        Serial.print(expect_period);
        Serial.println(")");
      }
    }
    check("  every pin's width", wok);
    check("  every pin's period", pok);
  }

  bool low = true;
  for (int k = 0; k < n; k++)
    servos[k].detach();
  delay(60);
  for (int k = 0; k < n; k++)
    low = low && !high(pads[chosen[k]]);
  check("all detached: every pin LOW", low);
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Servo on every pin without PWM (no wiring)");
  Serial.println("--- one pin at a time ---");

  for (int i = 0; i < PIN_COUNT; i++)
    one_pin(i);

  together();

  Serial.println();
  if (fails == 0)
    Serial.println("ALL PASS");
  else {
    Serial.print(fails);
    Serial.println(" FAIL");
  }
}

void loop() {
}
