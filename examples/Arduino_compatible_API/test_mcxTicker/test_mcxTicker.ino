/** mcxTicker: the bundled library with the interface of the Ticker in the
 *  ESP32 and ESP8266 cores, any number of objects, on SysTick.
 *
 *  Checks, by micros(), that the functions come at the period asked for
 *  (attach() in seconds, attach_ms()), that the first call comes at least
 *  the period after attach(), that several run together and alongside
 *  MsTimer2 (the two share SysTick), that once()/once_ms() call once and
 *  active() follows, the argument forms (a pointer, an int, an enum
 *  value), detach(), attach() on a running object, a function detaching
 *  or re-attaching its own object, a period of 0, a null function, and an
 *  object going out of scope while it runs.
 *
 *  Wiring: none.
 *
 *  Automatic: reads "ALL OK" or "N FAILED" at the end.
 */

#include <Arduino.h>
#include <mcxTicker.h>
#include <MsTimer2.h>

//  Each function counts its calls and keeps the time of the first and the
//  last, and the shortest and longest gap between two
struct Calls {
  volatile unsigned long n;
  volatile unsigned long first;
  volatile unsigned long last;
  volatile unsigned long gap_min;
  volatile unsigned long gap_max;
};

Calls ca, cb, cc, cd;

int failures = 0;

void check(const char *name, bool ok) {
  Serial.print(ok ? "  OK    " : "  FAIL  ");
  Serial.println(name);
  if (!ok)
    failures++;
}

void note(Calls &c) {
  unsigned long now = micros();
  if (c.n) {
    unsigned long gap = now - c.last;
    if (gap < c.gap_min)
      c.gap_min = gap;
    if (gap > c.gap_max)
      c.gap_max = gap;
  } else {
    c.first = now;
  }
  c.last = now;
  c.n = c.n + 1;
}

void clear(Calls &c) {
  noInterrupts();
  c.n = 0;
  c.first = 0;
  c.last = 0;
  c.gap_min = 0xFFFFFFFFul;
  c.gap_max = 0;
  interrupts();
}

//  The pointer form of the argument: every test below uses it
void note_arg(Calls *c) {
  note(*c);
}

void on_ms2() {
  note(cd);
}

void print_gaps(const Calls &c, unsigned long period) {
  Serial.print("        ");
  Serial.print(c.n);
  Serial.print(" calls, gaps ");
  Serial.print(c.gap_min);
  Serial.print("..");
  Serial.print(c.gap_max);
  Serial.print(" us (period ");
  Serial.print(period);
  Serial.println(" us)");
}

bool gaps_within(const Calls &c, unsigned long period, unsigned long slack) {
  return c.n >= 2 && c.gap_min + slack >= period && c.gap_max <= period + slack;
}

bool wait_calls(const Calls &c, unsigned long n, unsigned long timeout_ms) {
  unsigned long start = millis();
  while (c.n < n)
    if (millis() - start > timeout_ms)
      return false;
  return true;
}

mcxTicker ta, tb, tc;

volatile int last_int = 0;
volatile int int_calls = 0;
void take_int(int v) {
  last_int = v;
  int_calls = int_calls + 1;
}

volatile int plain_calls = 0;
void plain() {
  plain_calls = plain_calls + 1;
}

//  Detaches its own object after 5 calls
void five_then_detach(Calls *c) {
  note(*c);
  if (c->n >= 5)
    ta.detach();
}

//  once_ms() again from its own function, 5 times: the calls come a whole
//  period apart
void chain(Calls *c) {
  note(*c);
  if (c->n < 5)
    tb.once_ms(10, chain, c);
}

void test_periods() {
  Serial.println("periods");

  clear(ca);
  unsigned long t0 = micros();
  ta.attach_ms(10, note_arg, &ca);
  delay(1000);
  ta.detach();
  print_gaps(ca, 10000);
  check("attach_ms(10): 99-100 calls in 1 s", ca.n >= 99 && ca.n <= 100);
  check("attach_ms(10): every 10 ms (within 0.5 ms)", gaps_within(ca, 10000, 500));
  unsigned long first = ca.first - t0;
  Serial.print("        first call after ");
  Serial.print(first);
  Serial.println(" us");
  check("attach_ms(10): the first call 10-11 ms after attach_ms()", first >= 10000 && first <= 11200);

  clear(ca);
  ta.attach(0.02f, note_arg, &ca);
  delay(500);
  ta.detach();
  print_gaps(ca, 20000);
  check("attach(0.02): every 20 ms (within 0.5 ms)", gaps_within(ca, 20000, 500));

  clear(ca);
  ta.attach_ms(0, note_arg, &ca);
  delay(50);
  ta.detach();
  print_gaps(ca, 1000);
  check("attach_ms(0) is taken as 1 ms", gaps_within(ca, 1000, 300));

  //  100 periods against micros(): both count SysTick
  clear(ca);
  ta.attach_ms(10, note_arg, &ca);
  check("attach_ms(10): 101 calls", wait_calls(ca, 101, 1500));
  ta.detach();
  unsigned long span = ca.last - ca.first;
  Serial.print("        100 periods of 10 ms: ");
  Serial.print(span);
  Serial.println(" us");
  check("attach_ms(10): 100 periods 1 s by micros() (within 0.1%)", span >= 999000 && span <= 1001000);
}

void test_together() {
  Serial.println("several together, and with MsTimer2");

  clear(ca);
  clear(cb);
  clear(cc);
  clear(cd);
  ta.attach_ms(3, note_arg, &ca);
  tb.attach_ms(7, note_arg, &cb);
  tc.attach(0.01f, note_arg, &cc);
  MsTimer2::set(5, on_ms2);
  MsTimer2::start();
  delay(1000);
  ta.detach();
  tb.detach();
  tc.detach();
  MsTimer2::stop();
  print_gaps(ca, 3000);
  print_gaps(cb, 7000);
  print_gaps(cc, 10000);
  print_gaps(cd, 5000);
  check("attach_ms(3) alongside: every 3 ms (within 0.5 ms)", gaps_within(ca, 3000, 500) && ca.n >= 332);
  check("attach_ms(7) alongside: every 7 ms (within 0.5 ms)", gaps_within(cb, 7000, 500) && cb.n >= 141);
  check("attach(0.01) alongside: every 10 ms (within 0.5 ms)", gaps_within(cc, 10000, 500) && cc.n >= 99);
  check("MsTimer2 set(5) alongside: every 5 ms (within 0.5 ms)", gaps_within(cd, 5000, 500) && cd.n >= 199);
}

void test_once() {
  Serial.println("once");

  clear(ca);
  unsigned long t0 = micros();
  ta.once_ms(30, note_arg, &ca);
  bool active_before = ta.active();
  delay(100);
  unsigned long first = ca.first - t0;
  Serial.print("        called after ");
  Serial.print(first);
  Serial.println(" us");
  check("once_ms(30): active() until the call", active_before);
  check("once_ms(30): one call, 30-31 ms later", ca.n == 1 && first >= 30000 && first <= 31200);
  check("once_ms(30): active() false after the call", !ta.active());

  plain_calls = 0;
  ta.once(0.01f, plain);
  delay(50);
  check("once(0.01) with a plain function: one call", plain_calls == 1);

  clear(cb);
  tb.once_ms(10, chain, &cb);
  delay(100);
  print_gaps(cb, 10000);
  check("once_ms() again from its function: 5 calls, 10 ms apart (within 0.2 ms)",
        cb.n == 5 && gaps_within(cb, 10000, 200));
  check("once_ms() again from its function: not active after the last", !tb.active());
}

void test_arguments() {
  Serial.println("arguments");

  int_calls = 0;
  ta.attach_ms(5, take_int, 1234);
  delay(30);
  ta.detach();
  check("attach_ms(5, f, 1234): f gets 1234", int_calls >= 4 && last_int == 1234);

  int_calls = 0;
  ta.attach_ms(5, take_int, RED);  // an enum value, for a function taking int
  delay(30);
  ta.detach();
  check("attach_ms(5, f, RED): f(int) gets RED", int_calls >= 4 && last_int == (int)RED);

  plain_calls = 0;
  ta.attach_ms(5, plain);
  delay(30);
  ta.detach();
  check("attach_ms(5, f): a function with no argument", plain_calls >= 4);
}

void test_detach_and_changes() {
  Serial.println("detach and changes");

  clear(ca);
  ta.attach_ms(5, note_arg, &ca);
  delay(50);
  check("active() while it runs", ta.active());
  ta.detach();
  unsigned long n = ca.n;
  delay(50);
  check("detach(): no more calls, active() false", ca.n == n && !ta.active());
  ta.detach();
  check("detach() again: nothing happens", !ta.active());

  //  attach on a running object changes its period
  clear(ca);
  ta.attach_ms(5, note_arg, &ca);
  delay(50);
  clear(ca);
  ta.attach_ms(20, note_arg, &ca);
  delay(200);
  ta.detach();
  print_gaps(ca, 20000);
  check("attach_ms() on a running object: the new period, 20 ms", gaps_within(ca, 20000, 500));

  clear(ca);
  ta.attach_ms(2, five_then_detach, &ca);
  delay(100);
  check("detach() from its own function: 5 calls", ca.n == 5 && !ta.active());

  ta.attach_ms(5, (mcxTicker::callback_t)nullptr);
  check("attach_ms() with a null function: not active", !ta.active());

  //  An object going out of scope while it runs takes itself off
  clear(ca);
  clear(cb);
  tb.attach_ms(5, note_arg, &cb);
  {
    mcxTicker scoped;
    scoped.attach_ms(2, note_arg, &ca);
    delay(20);
  }
  n = ca.n;
  delay(30);
  tb.detach();
  check("an object out of scope stops, the others go on", n >= 8 && ca.n == n && cb.n >= 8);
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(100);

  Serial.println("\r\nmcxTicker");

  test_periods();
  test_together();
  test_once();
  test_arguments();
  test_detach_and_changes();

  Serial.println();
  if (failures)
    Serial.print(failures), Serial.println(" FAILED");
  else
    Serial.println("ALL OK");
}

void loop() {
}
