/** MsTimer2 and TimerOne: the bundled libraries with the same API as the
 *  libraries of those names for AVR.
 *
 *  Checks, by micros(), that the functions come at the period asked for,
 *  and that set()/start()/stop() (MsTimer2) and initialize()/setPeriod()/
 *  start()/stop()/resume()/attachInterrupt()/detachInterrupt() (TimerOne)
 *  do what they do on AVR: TimerOne's setPeriod() leaves the timer
 *  running, resume() goes on with the rest of the period, and a period that
 *  ended while nothing was attached is called for as soon as something is.
 *  Then that the two run together, and with tone() (on FRDM-MCXC444,
 *  tone() and TimerOne share the PIT).
 *
 *  Wiring: none (tone() plays on D8 for a moment).
 *
 *  Automatic: reads "ALL OK" or "N FAILED" at the end.
 */

#include <Arduino.h>
#include <MsTimer2.h>
#include <TimerOne.h>

//  Each library's function counts its calls and keeps the time of the
//  first and the last
struct Calls {
  volatile unsigned long n;
  volatile unsigned long first;
  volatile unsigned long last;
  volatile unsigned long gap_min;
  volatile unsigned long gap_max;
};

Calls ms2;
Calls t1;

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

void on_ms2() {
  note(ms2);
}

void on_t1() {
  note(t1);
}

//  The gaps between calls, against the period in microseconds
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

//  Waits until the function has been called n times, or timeout_ms
bool wait_calls(const Calls &c, unsigned long n, unsigned long timeout_ms) {
  unsigned long start = millis();
  while (c.n < n)
    if (millis() - start > timeout_ms)
      return false;
  return true;
}

void test_mstimer2() {
  Serial.println("MsTimer2");

  clear(ms2);
  MsTimer2::set(10, on_ms2);
  unsigned long t0 = micros();
  MsTimer2::start();
  delay(1000);
  MsTimer2::stop();
  unsigned long n = ms2.n;
  print_gaps(ms2, 10000);
  check("set(10)/start(): 99-101 calls in 1 s", n >= 99 && n <= 101);
  check("set(10): every 10 ms (within 0.5 ms)", gaps_within(ms2, 10000, 500));
  check("set(10): the first call 10 ms after start() (within 1 ms)",
        ms2.first - t0 >= 9000 && ms2.first - t0 <= 11000);

  delay(50);
  check("stop(): no more calls", ms2.n == n);

  MsTimer2::set(0, on_ms2);
  check("set(0) is taken as 1 ms", MsTimer2::msecs == 1);

  clear(ms2);
  MsTimer2::set(5, on_ms2);
  MsTimer2::start();
  delay(30);
  MsTimer2::set(5, on_ms2);  // stops it, as on AVR
  n = ms2.n;
  delay(30);
  check("set() stops a running timer", n >= 4 && ms2.n == n);
}

void test_timerone() {
  Serial.println("TimerOne");

  //  1 kHz
  clear(t1);
  Timer1.initialize(1000);
  Timer1.attachInterrupt(on_t1);
  delay(500);
  Timer1.detachInterrupt();
  print_gaps(t1, 1000);
  check("initialize(1000): 498-502 calls in 0.5 s", t1.n >= 498 && t1.n <= 502);
  check("initialize(1000): every 1 ms (within 50 us)", gaps_within(t1, 1000, 50));

  //  The period over 100 calls, against micros(). The timer counts a
  //  clock from the core clock's oscillator, so the two agree closely
  Timer1.attachInterrupt(on_t1, 10000);
  delay(15);  // past the call at once for the periods that ended detached
  clear(t1);
  check("attachInterrupt(isr, 10000): 100 calls", wait_calls(t1, 101, 1500));
  Timer1.detachInterrupt();
  unsigned long span = t1.last - t1.first;  // 100 periods
  Serial.print("        100 periods of 10 ms: ");
  Serial.print(span);
  Serial.println(" us");
  check("attachInterrupt(isr, 10000): 100 periods 1 s by micros() (within 0.1%)",
        span >= 999000 && span <= 1001000);

  //  stop() and resume(): resume goes on with the rest of the period.
  //  The timer ran on detached above, so attaching calls once at once
  //  for those periods (checked further down); the count starts after
  Timer1.initialize(100000);  // 100 ms
  Timer1.attachInterrupt(on_t1);
  delay(30);
  Timer1.stop();
  clear(t1);
  delay(200);
  check("stop(): no calls while stopped", t1.n == 0);
  unsigned long t_resume = micros();
  Timer1.resume();
  check("resume(): a call", wait_calls(t1, 1, 200));
  unsigned long after = t1.first - t_resume;
  Serial.print("        first call ");
  Serial.print(after);
  Serial.println(" us after resume() (30 ms of 100 had gone)");
  check("resume(): the rest of the period, about 70 ms (within 5 ms)",
        after >= 65000 && after <= 75000);

  //  start() begins a whole period
  clear(t1);
  Timer1.stop();
  unsigned long t0 = micros();
  Timer1.start();
  check("start(): a call", wait_calls(t1, 1, 300));
  after = t1.first - t0;
  check("start(): the first call a whole period later, 100 ms (within 5 ms)",
        after >= 95000 && after <= 105000);
  Timer1.restart();
  check("restart(): runs as start()", wait_calls(t1, 2, 300));

  //  setPeriod() leaves the timer running, even after stop()
  clear(t1);
  Timer1.stop();
  Timer1.setPeriod(5000);
  check("setPeriod() after stop(): runs again", wait_calls(t1, 3, 100));

  //  From a long period to a short one: the next period is the new one
  clear(t1);
  Timer1.setPeriod(2000000);  // 2 s
  delay(20);
  t0 = micros();
  Timer1.setPeriod(10000);
  check("setPeriod() from 2 s to 10 ms: a call within 15 ms",
        wait_calls(t1, 1, 15) && t1.first - t0 <= 15000);

  //  A period that ended while detached is called for at once on attach
  Timer1.detachInterrupt();
  delay(30);  // three periods of 10 ms with nothing attached
  clear(t1);
  t0 = micros();
  Timer1.attachInterrupt(on_t1);
  bool soon = wait_calls(t1, 1, 5) && t1.first - t0 <= 1000;
  check("attachInterrupt() after missed periods: a call at once (within 1 ms)", soon);

  //  detachInterrupt(): the timer keeps running
  Timer1.detachInterrupt();
  unsigned long n = t1.n;
  delay(50);
  check("detachInterrupt(): no more calls", t1.n == n);
}

void test_together() {
  Serial.println("MsTimer2, TimerOne and tone() together");

  clear(ms2);
  clear(t1);
  MsTimer2::set(2, on_ms2);
  MsTimer2::start();
  Timer1.initialize(500);
  Timer1.attachInterrupt(on_t1);

  //  On FRDM-MCXC444 this is tone()'s first use of the PIT, with
  //  TimerOne already on it: neither may stop the other
  pinMode(D8, INPUT);
  tone(D8, 1000);
  unsigned long edges = 0;
  int was = digitalRead(D8);
  unsigned long start = millis();
  while (millis() - start < 200) {
    int now = digitalRead(D8);
    if (now != was)
      edges++;
    was = now;
  }
  noTone(D8);
  unsigned long ms2_n = ms2.n;
  unsigned long t1_n = t1.n;
  delay(100);
  unsigned long ms2_after = ms2.n;
  unsigned long t1_after = t1.n;
  MsTimer2::stop();
  Timer1.detachInterrupt();
  Timer1.stop();

  Serial.print("        200 ms: tone() ");
  Serial.print(edges);
  Serial.print(" edges, MsTimer2 ");
  Serial.print(ms2_n);
  Serial.print(" calls, TimerOne ");
  Serial.print(t1_n);
  Serial.println(" calls");
  check("tone(D8, 1000) plays: 390-410 edges in 200 ms", edges >= 390 && edges <= 410);
  check("MsTimer2 every 2 ms alongside: 98-103 calls in 200 ms", ms2_n >= 98 && ms2_n <= 103);
  check("TimerOne every 500 us alongside: 395-406 calls in 200 ms", t1_n >= 395 && t1_n <= 406);
  check("noTone() leaves MsTimer2 running", ms2_after - ms2_n >= 45);
  check("noTone() leaves TimerOne running", t1_after - t1_n >= 190);
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  delay(100);

  Serial.println("\r\nMsTimer2 and TimerOne");

  test_mstimer2();
  test_timerone();
  test_together();

  Serial.println();
  if (failures)
    Serial.print(failures), Serial.println(" FAILED");
  else
    Serial.println("ALL OK");
}

void loop() {
}
