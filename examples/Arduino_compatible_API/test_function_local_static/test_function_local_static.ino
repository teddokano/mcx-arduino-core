/** Function-local statics that are made at run time
 *
 *  A static inside a function that holds an object with a constructor, or
 *  one set from a function call, is made the first time the function runs,
 *  and only then. Sketches and libraries use this for a lazily made object
 *  (a "singleton"). Up to 0.9.0 such a sketch failed to link, with
 *  "undefined reference to `__cxa_guard_acquire'": the compiler guarded the
 *  first-time setup for threads, and nothing here provides the guard. Since
 *  0.9.1 the core builds C++ with -fno-threadsafe-statics, as AVR's core
 *  does. Building at all is the first check.
 *
 *  No wiring. Read the final "ALL OK"/"N FAILED" line.
 */

#include <Arduino.h>

int constructed = 0;

struct Counter {
  int n;
  Counter()
    : n(0) {
    constructed++;
  }
  ~Counter() {}  //	registered to run at exit, which also has to link
  int next() {
    return ++n;
  }
};

int next_count() {
  static Counter c;
  return c.next();
}

int initial_calls = 0;

int initial() {
  initial_calls++;
  return 100;
}

int next_from_initial() {
  static int v = initial();
  return ++v;
}

String &greeting() {
  static String s("hello");
  return s;
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
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("=== function-local statics made at run time ===");

  int a = next_count();
  int b = next_count();
  int c = next_count();
  check("an object with a constructor keeps its state across calls", a == 1 && b == 2 && c == 3);
  check("its constructor ran once", constructed == 1);

  int d = next_from_initial();
  int e = next_from_initial();
  check("a value set from a function call keeps its state", d == 101 && e == 102);
  check("the function setting it ran once", initial_calls == 1);

  greeting() += "!";
  check("a String made on first use is the same one the next time", greeting() == "hello!");

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
