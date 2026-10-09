/** Release check 08: automatic OK/FAIL checks, no physical wiring needed,
 *  that a pin keeps working as pinMode() set it after attachInterrupt()
 *  and after analogRead(), and that analogWrite() keeps paired PWM
 *  channels and pins taken back from pinMode() right.
 *
 *  Consolidates (from examples/Arduino_compatible_API/):
 *  test_analogWrite_pairs_and_pinMode, test_attachInterrupt_keeps_pinMode
 *  and test_digitalRead_after_analogRead. It is a sketch of its own because
 *  release_check/01 already fills FRDM-MCXA153's flash.
 *
 *  The analogWrite() checks run first: on FRDM-MCXA156 and FRDM-MCXN236,
 *  D5 is a PWM pin too, and the later checks put an interrupt on it, which
 *  the pin-owner check at the end of the PWM part must not see. Before
 *  0.9.0, on every board, the partner's analogWrite() after
 *  analogWriteFrequency() put its own old period back and left the other
 *  channel's duty wrong, and analogWrite() after pinMode() left the pin
 *  a GPIO.
 *
 *  Up to 0.7.1, on every board:
 *    - the first attachInterrupt() on a pin made it an input with no pull,
 *      so pinMode(INPUT_PULLUP) then attachInterrupt() left it floating,
 *      and an OUTPUT pin stopped driving
 *    - CHANGE fired on the falling edge only
 *    - analogRead() turned the pin's input buffer off and only
 *      FRDM-MCXA156 turned it back on, so digitalRead() stayed at 0 on a
 *      pin once read with analogRead(), whatever pinMode() said
 *
 *  An unconnected pin with no pull keeps whatever level it last had for a
 *  long while, so reading it proves nothing. Each pull is checked by
 *  driving the pin the other way through the GPIO registers -- which
 *  leaves the pull setting alone, unlike pinMode() -- letting it go, and
 *  seeing the pull bring it back. The input buffer is checked with the
 *  pull-up on: a pin without one reads 0 whatever its level.
 *
 *  Wiring: none. Leave D2, D4, D5, the two analog pins below (A0/A1,
 *  A2/A3 on FRDM-MCXN947, A0/A4 on FRDM-MCXN236) and the PWM pins
 *  (PWM0-PWM5, and D3/D6/D9 on FRDM-MCXA156) unconnected. The PWM part
 *  reads each pin's level from its GPIO port's PDIR.
 *
 *  Automatic: reads "ALL OK" or "N FAILED" at the end.
 */

#include <Arduino.h>
#include "pin_registry.h"
#include <PinState.h>
#include <cstring>

#if defined(FRDM_MCXA153) || defined(FRDM_MCXA156)
// FRDM-MCXA156's A4/A5 are also on the CAN transceiver until R75/R76
// are removed, so stay on A0/A1
const int ANALOG_PIN_A = A0;
const int ANALOG_PIN_B = A1;
#elif defined(FRDM_MCXN947)
const int ANALOG_PIN_A = A2;  // A0/A1 are not analog inputs on FRDM-MCXN947
const int ANALOG_PIN_B = A3;
#elif defined(FRDM_MCXN236)
// A1/A2 are also on the CAN transceiver until R25/R67 are removed, and A3
// is not an analog input here (the blue LED's pin)
const int ANALOG_PIN_A = A0;
const int ANALOG_PIN_B = A4;
#else
#error "This sketch has no settings for this board yet"
#endif

struct Ch {
  int pin;
  const char *name;
  GPIO_Type *gpio;
  uint8_t bit;
};

#if defined(FRDM_MCXA153) || defined(FRDM_MCXA156)
Ch chs[] = {
  { PWM0, "PWM0", GPIO3, 11 }, { PWM1, "PWM1", GPIO3, 10 }, { PWM2, "PWM2", GPIO3, 9 },
  { PWM3, "PWM3", GPIO3, 8 },  { PWM4, "PWM4", GPIO3, 7 },  { PWM5, "PWM5", GPIO3, 6 },
#if defined(FRDM_MCXA156)
  { D3, "D3", GPIO3, 12 }, { D5, "D5", GPIO3, 14 }, { D6, "D6", GPIO3, 16 }, { D9, "D9", GPIO3, 17 },
#endif
};
#elif defined(FRDM_MCXN947)
Ch chs[] = {
  { PWM0, "PWM0", GPIO2, 3 }, { PWM1, "PWM1", GPIO2, 2 }, { PWM2, "PWM2", GPIO2, 5 },
  { PWM3, "PWM3", GPIO2, 4 }, { PWM4, "PWM4", GPIO2, 7 }, { PWM5, "PWM5", GPIO2, 6 },
};
#elif defined(FRDM_MCXN236)
Ch chs[] = {
  { PWM0, "PWM0", GPIO3, 17 }, { PWM1, "PWM1", GPIO3, 16 }, { PWM2, "PWM2", GPIO3, 15 },
  { PWM3, "PWM3", GPIO3, 14 }, { PWM4, "PWM4", GPIO2, 7 },  { PWM5, "PWM5", GPIO3, 12 },
};
#else
#error "This sketch has no settings for this board yet"
#endif

const int NCH = sizeof(chs) / sizeof(chs[0]);

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

Ch &ch(int pin) {
  for (auto &c : chs)
    if (c.pin == pin)
      return c;
  return chs[0];
}

// Rising edges per second and the fraction of samples that read high,
// over about 100ms
void measure(const Ch &c, float &hz, float &duty) {
  uint32_t m = 1u << c.bit, hi = 0, n = 0, edges = 0;
  bool prev = c.gpio->PDIR & m;
  uint32_t t0 = micros(), t = t0;
  while (t - t0 < 100000u) {
    for (int i = 0; i < 64; i++) {
      bool v = c.gpio->PDIR & m;
      hi += v;
      edges += (v && !prev);
      prev = v;
    }
    n += 64;
    t = micros();
  }
  hz = edges * 1.0e6f / (float)(t - t0);
  duty = (float)hi / (float)n;
}

// A pin at hz (0: not toggling) and duty, both within a few percent
bool runs(int pin, float hz, float duty) {
  float h, d;
  measure(ch(pin), h, d);
  Serial.print("  ");
  Serial.print(ch(pin).name);
  Serial.print(": ");
  Serial.print(h, 0);
  Serial.print(" Hz, duty ");
  Serial.println(d, 3);
  bool hz_ok = (hz == 0.0f) ? (h == 0.0f) : (fabsf(h - hz) <= hz * 0.03f);
  return hz_ok && fabsf(d - duty) <= 0.03f;
}

class BufPrint : public Print {
public:
  char buf[4096];
  size_t n = 0;
  size_t write(uint8_t c) override {
    if (n < sizeof(buf) - 1)
      buf[n++] = (char)c;
    buf[n] = 0;
    return 1;
  }
};

BufPrint table;
PinState pins;

// 0 none, 1 pull-down, 2 pull-up
uint8_t pullOf(int pin) {
  return pin_registry_read_pcr(arduino_pin_by_number[pin]).pull;
}

bool inputBufferOn(int pin) {
  return pin_registry_read_pcr(arduino_pin_by_number[pin]).ibe;
}

// Drive the pin to `level` for a moment, let go, and read it 200us later.
// A pull of the other polarity brings it back well within that; a pin
// with no pull still reads `level`.
bool levelAfterRelease(int pin, bool level) {
  GPIO_Type *port = digitalPinToPort(pin);
  uint32_t mask = digitalPinToBitMask(pin);
  if (level)
    *portOutputRegister(port) |= mask;
  else
    *portOutputRegister(port) &= ~mask;
  *portModeRegister(port) |= mask;
  delayMicroseconds(50);
  *portModeRegister(port) &= ~mask;
  delayMicroseconds(200);
  return (*portInputRegister(port) & mask) != 0;
}

volatile int falls = 0;
void onFall() { falls = falls + 1; }

volatile int rises = 0;
void onRise() { rises = rises + 1; }

volatile int changes = 0;
void onChange() { changes = changes + 1; }

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("=== Release check 08: pin modes with interrupts, analogRead and analogWrite (no wiring) ===");

  Serial.println("--- a pin joining a running partner ---");
  analogWriteFrequency(PWM5, 4000);
  analogWrite(PWM5, 64);
  analogWrite(PWM4, 191);  // PWM4's first analogWrite(), PWM5 already at 4kHz
  check("PWM5 stays at 4kHz", runs(PWM5, 4000, 0.25f));
  check("PWM4 joins it at 4kHz", runs(PWM4, 4000, 0.75f));

  Serial.println("--- distinct duties at the default 1kHz ---");
  analogWrite(PWM0, 26);
  analogWrite(PWM1, 77);
  analogWrite(PWM2, 128);
  analogWrite(PWM3, 230);
  check("PWM0 10%", runs(PWM0, 1000, 0.10f));
  check("PWM1 30%", runs(PWM1, 1000, 0.30f));
  check("PWM2 50%", runs(PWM2, 1000, 0.50f));
  check("PWM3 90%", runs(PWM3, 1000, 0.90f));

  Serial.println("--- analogWriteFrequency() on one, then analogWrite() on both ---");
  analogWriteFrequency(PWM3, 2500);
  analogWrite(PWM2, 179);
  analogWrite(PWM3, 51);
  check("PWM2 at 2.5kHz, 70%", runs(PWM2, 2500, 0.70f));
  check("PWM3 at 2.5kHz, 20%", runs(PWM3, 2500, 0.20f));
  analogWrite(PWM2, 77);  // used to put PWM2's own 1kHz back
  check("PWM2 again: still 2.5kHz, 30%", runs(PWM2, 2500, 0.30f));
  check("PWM3 untouched: 2.5kHz, 20%", runs(PWM3, 2500, 0.20f));

  Serial.println("--- a shorter period on the partner ---");
  analogWriteFrequency(PWM0, 5000);  // PWM1's 300us no longer fits in 200us
  analogWrite(PWM0, 128);
  analogWrite(PWM1, 77);
  check("PWM0 at 5kHz, 50%", runs(PWM0, 5000, 0.50f));
  check("PWM1 at 5kHz, 30%", runs(PWM1, 5000, 0.30f));

  Serial.println("--- pinMode() in between ---");
  pinMode(PWM0, OUTPUT);
  digitalWrite(PWM0, HIGH);
  check("pinMode(OUTPUT) + HIGH: steady high", runs(PWM0, 0, 1.0f));
  digitalWrite(PWM0, LOW);
  check("... LOW: steady low", runs(PWM0, 0, 0.0f));
  analogWrite(PWM0, 64);
  check("analogWrite() puts PWM back: 5kHz, 25%", runs(PWM0, 5000, 0.25f));
  check("PWM1 kept running throughout", runs(PWM1, 5000, 0.30f));

#if defined(FRDM_MCXA156)
  Serial.println("--- D3, D5, D6, D9 on FlexPWM1 ---");
  analogWrite(D3, 64);
  analogWrite(D5, 128);
  analogWrite(D6, 191);
  analogWriteFrequency(D9, 3000);
  analogWrite(D9, 51);
  analogWrite(D6, 191);
  check("D6 shares D9's 3kHz, 75%", runs(D6, 3000, 0.75f));
  check("D9 3kHz, 20%", runs(D9, 3000, 0.20f));
  analogWriteFrequency(D3, 1500);
  analogWrite(D3, 64);
  check("D3 1.5kHz on a period of its own, 25%", runs(D3, 1500, 0.25f));
  check("D5 still 1kHz, 50%", runs(D5, 1000, 0.50f));
  check("FlexPWM0 untouched: PWM2 2.5kHz, 30%", runs(PWM2, 2500, 0.30f));
  pinMode(D9, OUTPUT);
  digitalWrite(D9, HIGH);
  check("D9 pinMode(OUTPUT) + HIGH: steady high", runs(D9, 0, 1.0f));
  analogWrite(D9, 128);
  check("D9 analogWrite() again: 3kHz, 50%", runs(D9, 3000, 0.50f));
#elif defined(FRDM_MCXN236)
  Serial.println("--- D3, D5, D6, D9 are PWM5, PWM4, PWM0, PWM3 ---");
  analogWrite(D9, 128);
  check("analogWrite(D9) drives PWM3: 2.5kHz, 50%", runs(PWM3, 2500, 0.50f));
  analogWrite(D3, 191);
  check("analogWrite(D3) drives PWM5: 4kHz, 75%", runs(PWM5, 4000, 0.75f));
#endif

  Serial.println("--- digitalPinHasPWM() ---");
  bool all = true;
  for (int p = PWM0; p <= PWM5; p++)
    all = all && digitalPinHasPWM(p);
  check("PWM0-PWM5", all);
  check("not D2", !digitalPinHasPWM(D2));
#if defined(FRDM_MCXA156) || defined(FRDM_MCXN236)
  check("D3, D5, D6, D9", digitalPinHasPWM(D3) && digitalPinHasPWM(D5) && digitalPinHasPWM(D6) && digitalPinHasPWM(D9));
#elif defined(FRDM_MCXA153) || defined(FRDM_MCXN947)
  check("not D3, D5, D6, D9", !digitalPinHasPWM(D3) && !digitalPinHasPWM(D5) && !digitalPinHasPWM(D6) && !digitalPinHasPWM(D9));
#endif

  Serial.println("--- owners (mcxPinState) ---");
  pins.print(table);
  check("no CONFLICT after pinMode() and analogWrite()", strstr(table.buf, "CONFLICT") == nullptr);
  check("table captured whole", table.n < sizeof(table.buf) - 1);

  Serial.println("--- INPUT_PULLUP, then attachInterrupt(FALLING) ---");
  pinMode(D2, INPUT_PULLUP);
  delay(1);
  attachInterrupt(digitalPinToInterrupt(D2), onFall, FALLING);
  delay(1);
  check("no interrupt from attaching", falls == 0);
  check("pull-up still on (PCR)", pullOf(D2) == 2);
  check("pulled back HIGH after being driven LOW", levelAfterRelease(D2, LOW) == HIGH);
  check("... and that falling edge was caught", falls == 1);

  detachInterrupt(digitalPinToInterrupt(D2));
  attachInterrupt(digitalPinToInterrupt(D2), onFall, FALLING);  // the second attach, on the same pin
  check("attach again after detach: still pulled up", levelAfterRelease(D2, LOW) == HIGH && falls == 2);
  detachInterrupt(digitalPinToInterrupt(D2));

  Serial.println("--- INPUT_PULLDOWN, then attachInterrupt(RISING) ---");
  pinMode(D4, INPUT_PULLDOWN);
  delay(1);
  attachInterrupt(digitalPinToInterrupt(D4), onRise, RISING);
  delay(1);
  check("pull-down still on (PCR)", pullOf(D4) == 1);
  check("pulled back LOW after being driven HIGH", levelAfterRelease(D4, HIGH) == LOW);
  check("... and that rising edge was caught", rises == 1);
  detachInterrupt(digitalPinToInterrupt(D4));

  Serial.println("--- CHANGE on an input ---");
  attachInterrupt(digitalPinToInterrupt(D2), onChange, CHANGE);
  levelAfterRelease(D2, LOW);
  delay(1);
  check("driven LOW and let go: both edges caught", changes == 2);
  detachInterrupt(digitalPinToInterrupt(D2));

  // On a pin that hasn't had an interrupt yet: only the first attach
  // sets the pin up, so D2 and D4 can't show this any more
  Serial.println("--- OUTPUT HIGH, then attachInterrupt(CHANGE) ---");
  changes = 0;
  pinMode(D5, OUTPUT);
  digitalWrite(D5, HIGH);
  attachInterrupt(digitalPinToInterrupt(D5), onChange, CHANGE);
  GPIO_Type *port = digitalPinToPort(D5);
  uint32_t mask = digitalPinToBitMask(D5);
  check("still an output (PDDR)", (*portModeRegister(port) & mask) != 0);
  check("still driving HIGH", digitalRead(D5) == HIGH);
  digitalWrite(D5, LOW);
  delay(1);
  digitalWrite(D5, HIGH);
  delay(1);
  check("writing LOW then HIGH raises 2 interrupts on the pin itself", changes == 2);
  detachInterrupt(digitalPinToInterrupt(D5));
  pinMode(D5, INPUT);

  Serial.println("--- pinMode() after analogRead() ---");
  pinMode(ANALOG_PIN_A, INPUT_PULLUP);
  delay(1);
  check("before analogRead(): reads the pull-up's HIGH", digitalRead(ANALOG_PIN_A) == HIGH);
  int v = analogRead(ANALOG_PIN_A);
  Serial.print("  analogRead = ");
  Serial.println(v);
  check("analogRead() turned the input buffer off", !inputBufferOn(ANALOG_PIN_A));
  pinMode(ANALOG_PIN_A, INPUT_PULLUP);
  delay(1);
  check("pinMode(INPUT_PULLUP) again: the input buffer is back", inputBufferOn(ANALOG_PIN_A));
  check("... and it reads the pull-up's HIGH", digitalRead(ANALOG_PIN_A) == HIGH);

  Serial.println("--- pinMode() never called before analogRead() ---");
  analogRead(ANALOG_PIN_B);
  pinMode(ANALOG_PIN_B, INPUT_PULLUP);
  delay(1);
  check("reads the pull-up's HIGH", digitalRead(ANALOG_PIN_B) == HIGH);

  Serial.println("--- attachInterrupt() after analogRead() ---");
  analogRead(ANALOG_PIN_B);
  attachInterrupt(digitalPinToInterrupt(ANALOG_PIN_B), onRise, RISING);
  check("attachInterrupt() turned the input buffer back on", inputBufferOn(ANALOG_PIN_B));
  rises = 0;
  pinMode(ANALOG_PIN_B, INPUT_PULLDOWN);
  delay(1);
  pinMode(ANALOG_PIN_B, INPUT_PULLUP);  // pulled up again: a rising edge
  delay(1);
  check("the rising edge from the pull-up is caught", rises == 1);
  detachInterrupt(digitalPinToInterrupt(ANALOG_PIN_B));

  Serial.println();
  if (failCount == 0)
    Serial.println("ALL OK");
  else {
    Serial.print(failCount);
    Serial.println(" FAILED");
    Serial.println(table.buf);
  }
}

void loop() {
}
