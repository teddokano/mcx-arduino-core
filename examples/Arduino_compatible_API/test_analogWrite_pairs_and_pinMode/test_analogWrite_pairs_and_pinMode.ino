/** analogWrite(): paired channels, pinMode() in between, and the D-pins
 *
 *  No wiring: each PWM pin's own input buffer is on, so the sketch reads
 *  the pin's level straight from its GPIO port's PDIR and measures the
 *  frequency and duty there.
 *
 *  - Two channels of one FlexPWM submodule share its period. A pin that
 *    joins a running partner keeps the partner's frequency, and after
 *    analogWriteFrequency() on one of them, analogWrite() on either keeps
 *    both at the new frequency. (Before 0.9.0 the partner's analogWrite()
 *    put its own old period back and left the other's duty wrong.)
 *  - pinMode() switches a PWM pin to GPIO; the next analogWrite() puts it
 *    back on FlexPWM, and mcxPinState shows a single owner for it.
 *  - On FRDM-MCXA156, D3, D5, D6 and D9 are PWM pins of their own on
 *    FlexPWM1: D6 and D9 share a period, D3 and D5 have one each. On
 *    FRDM-MCXN236 they are four of the PWM0-PWM5 pins themselves.
 *  - digitalPinHasPWM() agrees with all of that.
 *
 *  Leave the PWM pins unconnected.
 *
 *  examples/release_check/08 runs the same checks; a change here goes
 *  there too.
 */

#include <Arduino.h>
#include <PinState.h>
#include <cstring>

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
  uint32_t m = 1u << c.bit, hi = 0, n = 0, rises = 0;
  bool prev = c.gpio->PDIR & m;
  uint32_t t0 = micros(), t = t0;
  while (t - t0 < 100000u) {
    for (int i = 0; i < 64; i++) {
      bool v = c.gpio->PDIR & m;
      hi += v;
      rises += (v && !prev);
      prev = v;
    }
    n += 64;
    t = micros();
  }
  hz = rises * 1.0e6f / (float)(t - t0);
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

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("=== analogWrite: paired channels, pinMode() in between, D-pins ===");

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
