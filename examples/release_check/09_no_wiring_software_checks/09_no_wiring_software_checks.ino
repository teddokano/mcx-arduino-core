/** Release check 09: automatic OK/FAIL checks of the core's software API,
 *  no physical wiring needed.
 *
 *  Split out of release_check/01 in 0.9.0, which keeps the checks that use
 *  a peripheral: 01 had grown to fill all but 40 bytes of FRDM-MCXA153's
 *  flash. Nothing here touches a peripheral beyond Serial for the report
 *  and, since 0.12.0, the timers of the bundled MsTimer2 and TimerOne
 *  (and tone() on D8 for a moment, with nothing wired to it), so it runs
 *  the same on every board.
 *
 *  Consolidates (from examples/Arduino_compatible_API/): test_math_constants,
 *  test_arduino_compat_macros, test_avr_compat_helpers,
 *  test_MOSI_MISO_SCK_macros, test_Print_writeError, test_String,
 *  test_String_64bit, test_Serial_print_time_t, test_function_local_static
 *  (since 0.9.1), test_MsTimer2_TimerOne (since 0.12.0), and the
 *  representative checks of test_print_float_rounding.
 *
 *  Read the final "ALL OK"/"N FAILED" line.
 */

#include <Arduino.h>
#include <cstring>
#include <MsTimer2.h>
#include <TimerOne.h>

// For the MsTimer2 and TimerOne section: each library's function counts
// its calls and keeps the time of the first and the last, and the
// shortest and longest gap between two. Ahead of every function, so that
// the prototypes the IDE adds above the first one can name it
struct Calls {
  volatile unsigned long n;
  volatile unsigned long first;
  volatile unsigned long last;
  volatile unsigned long gap_min;
  volatile unsigned long gap_max;
};
Calls ms2Calls;
Calls t1Calls;

// A minimal Print-derived class that can simulate a failing write(), used
// by the Print::*WriteError() section below. File-scope, not declared
// inside setup() -- a local class with a virtual override here tripped a
// placement-new/char_traits<char32_t> compile error in this toolchain
// that the identical class at file scope doesn't.
class FlakyPrint : public Print {
public:
  size_t write(uint8_t c) override {
    if (fail) {
      setWriteError();
      return 0;
    }
    return 1;
  }
  bool fail = false;
};

// Keeps what is printed to it, for the Print-in-HEX section
class TextPrint : public Print {
public:
  size_t write(uint8_t c) override {
    if (len < sizeof(text) - 1) {
      text[len++] = c;
      text[len] = '\0';
    }
    return 1;
  }
  const char *take() {
    len = 0;
    return text;
  }
  char text[40] = "";
  size_t len = 0;
};

// For the AVR-era helpers section: globals a sketch may name as on AVR
// (building at all is the check), and an overload on BitOrder, which
// Adafruit BusIO has and which a plain-integer BitOrder made ambiguous.
int index = 0;
int x0 = 1, y0 = 2, x1 = 3, y1 = 4;
int order_kind(BitOrder) { return 1; }
int order_kind(int8_t) { return 2; }

// For the function-local statics section: statics made the first time
// their function runs. Up to 0.9.0 these failed to link (__cxa_guard_*).
int lsConstructed = 0;
struct LocalStaticCounter {
  int n = 0;
  LocalStaticCounter() { lsConstructed++; }
  ~LocalStaticCounter() {}
  int next() { return ++n; }
};
int lsNextCount() {
  static LocalStaticCounter c;
  return c.next();
}
int lsInitialCalls = 0;
int lsInitial() {
  lsInitialCalls++;
  return 100;
}
int lsNextFromInitial() {
  static int v = lsInitial();
  return ++v;
}

void noteCall(Calls &c) {
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
void clearCalls(Calls &c) {
  noInterrupts();
  c.n = 0;
  c.first = 0;
  c.last = 0;
  c.gap_min = 0xFFFFFFFFul;
  c.gap_max = 0;
  interrupts();
}
void onMsTimer2() { noteCall(ms2Calls); }
void onTimer1() { noteCall(t1Calls); }
bool gapsWithin(const Calls &c, unsigned long period, unsigned long slack) {
  return c.n >= 2 && c.gap_min + slack >= period && c.gap_max <= period + slack;
}
// Waits until the function has been called n times, or timeout_ms
bool waitCalls(const Calls &c, unsigned long n, unsigned long timeout_ms) {
  unsigned long start = millis();
  while (c.n < n)
    if (millis() - start > timeout_ms)
      return false;
  return true;
}
void printGaps(const char *label, const Calls &c) {
  Serial.print(label);
  Serial.print(c.n);
  Serial.print(" calls, gaps ");
  Serial.print(c.gap_min);
  Serial.print("..");
  Serial.print(c.gap_max);
  Serial.println(" us");
}

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

void setup() {
  // the sketch's first millis() call, after a delay(): before 0.11.0 the core
  // started the count at that call, so it read 0 here (AVR counts from boot)
  delay(200);
  unsigned long first_millis = millis();

  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("=== Release check 09: no-wiring software checks ===");

  // ---- millis() counts from before setup(), as on AVR (0.11.0) ----
  Serial.println("--- millis from boot ---");
  check("first millis() after delay(200) is 200 or more", first_millis >= 200);

  // ---- micros() stays right when SysTick runs late (0.11.0) ----
  // Interrupts off for 700 us hold back a tick that falls in that time; the
  // core took the time the late tick ran as the start of the millisecond, so
  // micros() read up to 700 us short until the next tick
  Serial.println("--- micros with a late tick ---");
  {
    unsigned long d_min = 0xFFFFFFFF, d_max = 0;
    for (int i = 0; i < 20; i++) {
      unsigned long t0 = micros();
      noInterrupts();
      delayMicroseconds(700);
      interrupts();
      delayMicroseconds(300);
      unsigned long d = micros() - t0;
      if (d < d_min) d_min = d;
      if (d > d_max) d_max = d;
      delayMicroseconds(137);  // a different phase against the tick each time
    }
    Serial.print("  1000 us measured as ");
    Serial.print(d_min);
    Serial.print("-");
    Serial.print(d_max);
    Serial.println(" us");
    check("micros() across 20 late ticks within 1000-1050 us", d_min >= 1000 && d_max <= 1050);
  }

  // ---- math constants / trig (was test_math_constants) ----
  Serial.println("--- math constants ---");
  check("PI", fabs(PI - 3.14159265) < 0.00001);
  check("HALF_PI", fabs(HALF_PI - 1.57079633) < 0.00001);
  check("TWO_PI", fabs(TWO_PI - 6.28318531) < 0.00001);
  check("radians(180) == PI", fabs(radians(180.0) - PI) < 0.00001);
  check("degrees(PI) == 180", fabs(degrees(PI) - 180.0) < 0.00001);
  check("sin(HALF_PI) == 1", fabs(sin(HALF_PI) - 1.0) < 0.00001);
  check("sqrt(2.0)", fabs(sqrt(2.0) - 1.41421356) < 0.00001);

  // ---- UNO R3/R4 compat macros (was test_arduino_compat_macros) ----
  Serial.println("--- compat macros ---");
  check("min(3,7)", min(3, 7) == 3);
  check("max(3,7)", max(3, 7) == 7);
  check("abs(-5)", abs(-5) == 5);
  check("constrain(15,0,10)", constrain(15, 0, 10) == 10);
  check("sq(4)", sq(4) == 16);
  check("map(512,0,1023,0,255)", map(512, 0, 1023, 0, 255) == 127);

  uint8_t v = 0;
  bitSet(v, 3);
  check("bitSet/bitRead", bitRead(v, 3) == 1);
  bitClear(v, 3);
  check("bitClear", bitRead(v, 3) == 0);

  check("lowByte(0x1234)", lowByte(0x1234) == 0x34);
  check("highByte(0x1234)", highByte(0x1234) == 0x12);
  check("bit(3)", bit(3) == 8);

  noInterrupts();
  interrupts();
  check("interrupts()/noInterrupts() (reached here without hanging)", true);

  byte b = 200;
  word w = 5000;
  boolean flag = true;
  check("byte/word/boolean types", b == 200 && w == 5000 && flag == true);

  // ---- AVR-era helpers (was test_avr_compat_helpers) ----
  Serial.println("--- AVR-era helpers ---");
  {
    char s[40];
    check("itoa(-123, 10)", strcmp(itoa(-123, s, 10), "-123") == 0);
    check("itoa(-1, 16) (32-bit int)", strcmp(itoa(-1, s, 16), "ffffffff") == 0);
    check("utoa(255, 2)", strcmp(utoa(255, s, 2), "11111111") == 0);
    check("ltoa(-2147483648, 10)", strcmp(ltoa(-2147483647L - 1, s, 10), "-2147483648") == 0);
    check("ultoa(4294967295, 16)", strcmp(ultoa(4294967295UL, s, 16), "ffffffff") == 0);
    check("dtostrf(3.14159, 7, 2)", strcmp(dtostrf(3.14159, 7, 2, s), "   3.14") == 0);
    check("dtostrf(1.999, 1, 2) rounds", strcmp(dtostrf(1.999, 1, 2, s), "2.00") == 0);
    check("dtostrf(-0.26, -7, 1) left-aligned", strcmp(dtostrf(-0.26, -7, 1, s), "-0.3   ") == 0);
    check("word(0x12, 0x34)", word(0x12, 0x34) == 0x1234);
    check("_BV(5)", _BV(5) == 32);
    analogReference(DEFAULT);
    analogReference(AR_DEFAULT);
    check("analogReference(DEFAULT/AR_DEFAULT) accepted", true);
    HardwareSerial &port = Serial;
    check("HardwareSerial& refers to Serial", &port == &Serial);
    char t[8];
    check("strlcpy truncates", strlcpy(t, "abcdefghij", sizeof t) == 10 && strcmp(t, "abcdefg") == 0);
    strlcpy(s, "a,b,,c", sizeof s);
    char *p = s;
    strsep(&p, ",");
    strsep(&p, ",");
    check("strsep keeps the empty field", strcmp(strsep(&p, ","), "") == 0 && strcmp(p, "c") == 0);
    char *save;
    check("strtok_r", strcmp(strtok_r(s, ",", &save), "a") == 0);
    check("strnlen", strnlen("abcdef", 4) == 4);
    check("M_PI == PI", M_PI == PI);
    check("MSBFIRST picks the BitOrder overload", order_kind(MSBFIRST) == 1);
    index++;
    check("globals index, x0, y0, x1, y1", index == 1 && x0 + y0 + x1 + y1 == 10);
  }

  // ---- MOSI/MISO/SCK bare macros (was test_MOSI_MISO_SCK_macros) ----
  Serial.println("--- MOSI/MISO/SCK macros ---");
  check("MOSI == ARD_MOSI", MOSI == ARD_MOSI);
  check("MISO == ARD_MISO", MISO == ARD_MISO);
  check("SCK == ARD_SCK", SCK == ARD_SCK);

  // ---- Print::set/get/clearWriteError() (was test_Print_writeError) ----
  Serial.println("--- Print::*WriteError() ---");
  {
    FlakyPrint flaky;
    check("initial getWriteError() == 0", flaky.getWriteError() == 0);
    flaky.fail = true;
    flaky.print("x");
    check("getWriteError() != 0 after failed write", flaky.getWriteError() != 0);
    flaky.clearWriteError();
    check("getWriteError() == 0 after clearWriteError()", flaky.getWriteError() == 0);
    flaky.fail = false;
    flaky.print("x");
    check("getWriteError() == 0 after successful write", flaky.getWriteError() == 0);
    // print(double) rounds (was test_print_float_rounding): "10.00" is 5
    // bytes where truncating gave "9.99", and "2" is 1 where it gave "1."
    check("print(9.9999) rounds up to \"10.00\"", flaky.print(9.9999) == 5);
    check("print(1.999, 0) is \"2\", no decimal point", flaky.print(1.999, 0) == 1);
  }

  // ---- Print in HEX and other bases: upper case, two's complement and base 0, as AVR (0.11.0) ----
  Serial.println("--- Print in HEX ---");
  {
    TextPrint t;
    t.print(255, HEX);
    check("print(255, HEX) is \"FF\"", strcmp(t.take(), "FF") == 0);
    t.print(0xDEADBEEFUL, HEX);
    check("print(0xDEADBEEF, HEX) is \"DEADBEEF\"", strcmp(t.take(), "DEADBEEF") == 0);
    t.print(-1L, HEX);
    check("print(-1L, HEX) is \"FFFFFFFF\"", strcmp(t.take(), "FFFFFFFF") == 0);
    t.print(0xABCDEF012345ULL, HEX);
    check("print(0xABCDEF012345ULL, HEX) is \"ABCDEF012345\"", strcmp(t.take(), "ABCDEF012345") == 0);
    t.print(35, 36);
    check("print(35, 36) is \"Z\"", strcmp(t.take(), "Z") == 0);
    // a negative number in another base is its two's complement, and base 0 writes one byte, as AVR
    t.print(-1, 3);
    check("print(-1, 3) is 4294967295 in base 3", strcmp(t.take(), "102002022201221111210") == 0);
    t.print(-2, BIN);
    check("print(-2, BIN) is 31 ones and a 0", strcmp(t.take(), "11111111111111111111111111111110") == 0);
    t.print(-1LL, 36);
    check("print(-1LL, 36) is 2^64-1 in base 36", strcmp(t.take(), "3W5E11264SGSF") == 0);
    t.print(-5, DEC);
    check("print(-5, DEC) stays \"-5\"", strcmp(t.take(), "-5") == 0);
    check("print(65, 0) writes one byte, 'A'", t.print(65, 0) == 1 && strcmp(t.take(), "A") == 0);
    check("print(0x141UL, 0) writes the low byte, 'A'", t.print(0x141UL, 0) == 1 && strcmp(t.take(), "A") == 0);
    check("String(255, HEX) stays \"ff\"", String(255, HEX) == "ff");
    check("String(-1, 3) is 4294967295 in base 3, as ArduinoCore-API", String(-1, 3) == "102002022201221111210");
    check("String(-1LL, 36) is 2^64-1 in base 36", String(-1LL, 36) == "3w5e11264sgsf");
    check("String(-5) stays \"-5\"", String(-5) == "-5");
  }

  // ---- String (was test_String) ----
  Serial.println("--- String ---");
  {
    String a = "Hello";
    String b = String(", ") + "world" + String('!');
    String c = a + b;
    check("concat", c == "Hello, world!");

    String num = String(42) + " / " + String(3.14, 2);
    check("numeric concat", num == "42 / 3.14");
    check("String(1.999) rounds to \"2.00\" (was test_print_float_rounding)", String(1.999) == "2.00");

    check("length", c.length() == 13);
    check("charAt", c.charAt(0) == 'H');
    check("indexOf", c.indexOf("world") == 7);
    check("substring", c.substring(7, 12) == "world");
    check("startsWith", c.startsWith("Hello"));
    check("endsWith", c.endsWith("!"));

    String upper = c;
    upper.toUpperCase();
    check("toUpperCase", upper == "HELLO, WORLD!");

    String spaced = "   trim me   ";
    spaced.trim();
    check("trim", spaced == "trim me");

    String replaced = c;
    replaced.replace("world", "there");
    check("replace", replaced == "Hello, there!");

    check("toInt", String("12345").toInt() == 12345);

    float ft = String("3.5").toFloat();
    check("toFloat", ft > 3.49f && ft < 3.51f);
  }

  // ---- String 64bit (was test_String_64bit) ----
  Serial.println("--- String 64bit ---");
  {
    long long bigNeg = -5000000000LL;
    unsigned long long bigPos = 10000000000ULL;

    check("String(long long)", String(bigNeg) == "-5000000000");
    check("String(unsigned long long)", String(bigPos) == "10000000000");

    String s3 = "value=";
    s3 += bigPos;
    check("operator+=(unsigned long long)", s3 == "value=10000000000");

    String s4;
    s4.concat(bigNeg);
    check("concat(long long)", s4 == "-5000000000");

    check("String(long long, HEX)", String(255LL, HEX) == "ff");
  }

  // ---- function-local statics made at run time (was
  //      test_function_local_static) -- building at all is the first check ----
  Serial.println("--- function-local statics ---");
  {
    int a = lsNextCount();
    int b = lsNextCount();
    int c = lsNextCount();
    check("object with a constructor keeps its state, constructed once",
          a == 1 && b == 2 && c == 3 && lsConstructed == 1);
    int d = lsNextFromInitial();
    int e = lsNextFromInitial();
    check("value set from a function call keeps its state, set once",
          d == 101 && e == 102 && lsInitialCalls == 1);
  }

  // ---- MsTimer2 and TimerOne, bundled with the AVR libraries' API (was
  //      test_MsTimer2_TimerOne, 0.12.0). MsTimer2 runs on SysTick; TimerOne
  //      on CTIMER1 (MCX) or PIT channel 1 (FRDM-MCXC444, sharing the PIT
  //      with tone()) ----
  Serial.println("--- MsTimer2 and TimerOne ---");
  {
    // MsTimer2: every 10 ms for 1 s
    clearCalls(ms2Calls);
    MsTimer2::set(10, onMsTimer2);
    unsigned long t0 = micros();
    MsTimer2::start();
    delay(1000);
    MsTimer2::stop();
    unsigned long n = ms2Calls.n;
    printGaps("MsTimer2 set(10): ", ms2Calls);
    check("MsTimer2 set(10)/start(): 99-101 calls in 1 s", n >= 99 && n <= 101);
    check("MsTimer2 set(10): every 10 ms (within 0.5 ms)", gapsWithin(ms2Calls, 10000, 500));
    check("MsTimer2 set(10): the first call 10 ms after start() (within 1 ms)",
          ms2Calls.first - t0 >= 9000 && ms2Calls.first - t0 <= 11000);
    delay(50);
    check("MsTimer2 stop(): no more calls", ms2Calls.n == n);
    MsTimer2::set(0, onMsTimer2);
    check("MsTimer2 set(0) is taken as 1 ms", MsTimer2::msecs == 1);
    clearCalls(ms2Calls);
    MsTimer2::set(5, onMsTimer2);
    MsTimer2::start();
    delay(30);
    MsTimer2::set(5, onMsTimer2);  // stops it, as on AVR
    n = ms2Calls.n;
    delay(30);
    check("MsTimer2 set() stops a running timer", n >= 4 && ms2Calls.n == n);

    // TimerOne: 1 kHz for 0.5 s
    clearCalls(t1Calls);
    Timer1.initialize(1000);
    Timer1.attachInterrupt(onTimer1);
    delay(500);
    Timer1.detachInterrupt();
    printGaps("Timer1 initialize(1000): ", t1Calls);
    check("Timer1 initialize(1000): 498-502 calls in 0.5 s", t1Calls.n >= 498 && t1Calls.n <= 502);
    check("Timer1 initialize(1000): every 1 ms (within 50 us)", gapsWithin(t1Calls, 1000, 50));

    // 100 periods against micros(): the timer counts a clock from the core
    // clock's oscillator, so the two agree closely
    Timer1.attachInterrupt(onTimer1, 10000);
    delay(15);  // past the call at once for the periods that ended detached
    clearCalls(t1Calls);
    check("Timer1 attachInterrupt(isr, 10000): 100 calls", waitCalls(t1Calls, 101, 1500));
    Timer1.detachInterrupt();
    unsigned long span = t1Calls.last - t1Calls.first;
    Serial.print("Timer1 100 periods of 10 ms: ");
    Serial.print(span);
    Serial.println(" us");
    check("Timer1 100 periods 1 s by micros() (within 0.1%)", span >= 999000 && span <= 1001000);

    // stop() and resume(): resume goes on with the rest of the period
    Timer1.initialize(100000);  // 100 ms
    Timer1.attachInterrupt(onTimer1);
    delay(30);
    Timer1.stop();
    clearCalls(t1Calls);
    delay(200);
    check("Timer1 stop(): no calls while stopped", t1Calls.n == 0);
    unsigned long t_resume = micros();
    Timer1.resume();
    bool resumed = waitCalls(t1Calls, 1, 200);
    unsigned long after = t1Calls.first - t_resume;
    Serial.print("Timer1 first call after resume(), 30 ms of 100 gone: ");
    Serial.print(after);
    Serial.println(" us");
    check("Timer1 resume(): the rest of the period, about 70 ms (within 5 ms)",
          resumed && after >= 65000 && after <= 75000);

    // start() begins a whole period; restart() is start()
    clearCalls(t1Calls);
    Timer1.stop();
    t0 = micros();
    Timer1.start();
    bool started = waitCalls(t1Calls, 1, 300);
    after = t1Calls.first - t0;
    check("Timer1 start(): the first call a whole period later, 100 ms (within 5 ms)",
          started && after >= 95000 && after <= 105000);
    Timer1.restart();
    check("Timer1 restart(): runs as start()", waitCalls(t1Calls, 2, 300));

    // setPeriod() leaves the timer running, even after stop(), as on AVR
    clearCalls(t1Calls);
    Timer1.stop();
    Timer1.setPeriod(5000);
    check("Timer1 setPeriod() after stop(): runs again", waitCalls(t1Calls, 3, 100));

    // From a long period to a short one: the next period is the new one
    clearCalls(t1Calls);
    Timer1.setPeriod(2000000);
    delay(20);
    t0 = micros();
    Timer1.setPeriod(10000);
    check("Timer1 setPeriod() from 2 s to 10 ms: a call within 15 ms",
          waitCalls(t1Calls, 1, 15) && t1Calls.first - t0 <= 15000);

    // A period that ended while detached is called for at once, as on AVR
    Timer1.detachInterrupt();
    delay(30);
    clearCalls(t1Calls);
    t0 = micros();
    Timer1.attachInterrupt(onTimer1);
    check("Timer1 attachInterrupt() after missed periods: a call at once (within 1 ms)",
          waitCalls(t1Calls, 1, 5) && t1Calls.first - t0 <= 1000);
    Timer1.detachInterrupt();
    n = t1Calls.n;
    delay(50);
    check("Timer1 detachInterrupt(): no more calls", t1Calls.n == n);

    // Both with tone() at once. On FRDM-MCXC444 this is tone()'s first use
    // of the PIT, with TimerOne already on it: neither may stop the other
    clearCalls(ms2Calls);
    clearCalls(t1Calls);
    MsTimer2::set(2, onMsTimer2);
    MsTimer2::start();
    Timer1.initialize(500);
    Timer1.attachInterrupt(onTimer1);
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
    unsigned long ms2_n = ms2Calls.n;
    unsigned long t1_n = t1Calls.n;
    delay(100);
    unsigned long ms2_after = ms2Calls.n;
    unsigned long t1_after = t1Calls.n;
    MsTimer2::stop();
    Timer1.detachInterrupt();
    Timer1.stop();
    pinMode(D8, INPUT);
    Serial.print("200 ms together: tone() ");
    Serial.print(edges);
    Serial.print(" edges, MsTimer2 ");
    Serial.print(ms2_n);
    Serial.print(" calls, Timer1 ");
    Serial.print(t1_n);
    Serial.println(" calls");
    check("tone(D8, 1000) alongside both: 390-410 edges in 200 ms", edges >= 390 && edges <= 410);
    check("MsTimer2 every 2 ms alongside: 98-103 calls in 200 ms", ms2_n >= 98 && ms2_n <= 103);
    check("Timer1 every 500 us alongside: 395-406 calls in 200 ms", t1_n >= 395 && t1_n <= 406);
    check("noTone() leaves MsTimer2 running", ms2_after - ms2_n >= 45);
    check("noTone() leaves Timer1 running", t1_after - t1_n >= 190);
  }

  // ---- Serial.print(time_t)/(long long)/(unsigned long long) overload
  //      resolution (was test_Serial_print_time_t) -- the original point
  //      of this test is that these lines *compile* at all (time_t is
  //      ambiguous between long/long long on this newlib), so there's no
  //      further runtime check beyond that; printed for a human to glance
  //      at if something looks wrong.
  Serial.println("--- Serial.print(time_t / long long / unsigned long long) ---");
  time_t current_time = 1734567890;
  Serial.print("time_t: ");
  Serial.println(current_time);
  Serial.print("long long: ");
  Serial.println(123456789012345LL);
  Serial.print("unsigned long long: ");
  Serial.println(18446744073709551615ULL);

  Serial.println();
  if (failCount == 0)
    Serial.println("ALL OK");
  else {
    Serial.print(failCount);
    Serial.println(" FAILED");
  }
}

void loop() {
}
