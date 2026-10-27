/** MultipleTickers: several functions, each at a period of its own, from
 *  MsTimer2's one timer, the way mbed's Ticker works.
 *
 *  MsTimer2 calls one function, at one period. The Ticker class below
 *  hooks any number of functions to it: each Ticker object calls its own
 *  function every given number of milliseconds, from attach() until
 *  detach(), as in mbed. MsTimer2 runs Ticker::tick() every 1 ms, and
 *  tick() counts down each Ticker.
 *
 *  The functions run in the timer interrupt, as in mbed and as MsTimer2's
 *  own do: keep them short. Slow work, such as printing on Serial, is
 *  better left to loop(), with the function only raising a flag, as
 *  report() does here.
 *
 *  Wiring: none. The on-board LED changes every 500 ms, 20 times, and
 *  stops at 10.25 s; every 2 s, loop() prints how many times each
 *  function has run.
 */

#include <MsTimer2.h>

class Ticker {
public:
  using Callback = void (*)();

  ~Ticker() {
    detach();
  }

  // Calls f every period_ms milliseconds, the first time period_ms from
  // now. Calling it again on a Ticker that runs changes its function and
  // period.
  void attach(Callback f, unsigned long period_ms) {
    if (f == nullptr || period_ms == 0)
      return;

    uint32_t primask = __get_PRIMASK();  // works from a Ticker's own function too
    __disable_irq();
    func = f;
    period = period_ms;
    left = period_ms;
    if (!linked) {
      next = head;
      head = this;
      linked = true;
    }
    __set_PRIMASK(primask);
  }

  // Stops calling the function. A Ticker's own function may detach it.
  void detach() {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    for (Ticker **p = &head; *p != nullptr; p = &(*p)->next) {
      if (*p == this) {
        *p = next;  // next stays as it is, so that tick() can go on past it
        break;
      }
    }
    linked = false;
    __set_PRIMASK(primask);
  }

  // Counts down every Ticker by one millisecond. MsTimer2 calls it.
  // Tickers that come due in the same millisecond run in the reverse
  // order of their first attach()
  static void tick() {
    for (Ticker *t = head; t != nullptr; t = t->next) {
      if (--t->left == 0) {
        t->left = t->period;
        t->func();
      }
    }
  }

private:
  static Ticker *head;
  Ticker *next = nullptr;
  Callback func = nullptr;
  unsigned long period = 0;
  unsigned long left = 0;
  bool linked = false;
};

Ticker *Ticker::head = nullptr;

Ticker led;      // every 500 ms
Ticker counter;  // every 100 ms
Ticker reporter; // every 2 s
Ticker stopper;  // once, after 10.25 s

volatile unsigned long ledChanges = 0;
volatile unsigned long counts = 0;
volatile bool reportDue = false;
volatile bool ledStopped = false;

void blink() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  ledChanges = ledChanges + 1;
}

void count() {
  counts = counts + 1;
}

void report() {
  reportDue = true;  // the printing is done in loop()
}

void stopLed() {
  led.detach();
  stopper.detach();  // only once
  ledStopped = true;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  led.attach(blink, 500);
  counter.attach(count, 100);
  reporter.attach(report, 2000);
  stopper.attach(stopLed, 10250);

  MsTimer2::set(1, Ticker::tick);  // every 1 ms
  MsTimer2::start();
}

void loop() {
  if (reportDue) {
    reportDue = false;

    noInterrupts();  // a consistent pair: neither changes halfway through
    unsigned long c = counts;
    unsigned long l = ledChanges;
    interrupts();

    Serial.print("100 ms ticks: ");
    Serial.print(c);
    Serial.print(", LED changes (500 ms): ");
    Serial.print(l);
    Serial.println(ledStopped ? " (stopped)" : "");
  }
}
