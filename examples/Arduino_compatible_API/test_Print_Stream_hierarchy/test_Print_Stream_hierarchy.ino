/** Print/Stream abstract base class hierarchy test.
 *
 *  Verifies the refactor from "Print/Printable as a SerialClass-only
 *  alias" to a real, hardware-independent Print/Stream class hierarchy:
 *   - a class can inherit Print directly (no hardware, no tx/rx pins)
 *   - Stream& works as a genuine polymorphic reference (not just a
 *     SerialClass-shaped alias)
 *   - print()/println() now return byte counts (size_t), not void
 *   - Printable::printTo()'s real-Arduino `n += p.print(x)` idiom, which
 *     didn't compile-correctly before (print() returned void), now
 *     accumulates real byte counts
 *
 *  Wiring needed: D0-D1 jumper (Serial1 loopback, for the Stream&
 *  polymorphism check).
 */

#include <Arduino.h>
#include <cstring>

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
}

// A Print-derived class with no hardware/pins at all -- this is exactly
// the pattern (e.g. ArduinoJson's internal StringBuilderPrint,
// LiquidCrystal) that couldn't compile when Print was just an alias for
// the pin-constructor-requiring SerialClass.
class BufferPrint : public Print {
public:
  BufferPrint() { buf[0] = '\0'; }
  size_t write(uint8_t c) override {
    if (len < sizeof(buf) - 1) {
      buf[len++] = (char)c;
      buf[len] = '\0';
    }
    return 1;
  }
  const char *c_str() const { return buf; }

private:
  char buf[64];
  size_t len = 0;
};

class Point : public Printable {
public:
  Point(int x, int y) : _x(x), _y(y) {}
  size_t printTo(Print &p) const override {
    size_t n = 0;
    n += p.print('(');
    n += p.print(_x);
    n += p.print(',');
    n += p.print(_y);
    n += p.print(')');
    return n;
  }

private:
  int _x, _y;
};

bool findViaStreamRef(Stream &s, const char *target) {
  return s.find(target);
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Print/Stream hierarchy test");

  // ---- class Foo : public Print (no hardware) ----
  BufferPrint bp;
  size_t bn = bp.print("hello ");
  bn += bp.print(42);
  check("Print-derived class with no hardware works", strcmp(bp.c_str(), "hello 42") == 0);
  check("BufferPrint byte count", bn == 8);

  // ---- print()/println() now return real byte counts ----
  size_t n1 = Serial.print("abc");
  check("Serial.print(const char*) byte count", n1 == 3);

  size_t n2 = Serial.println("abcd");
  check("Serial.println byte count (incl CRLF)", n2 == 6);

  size_t n3 = Serial.print(12345);
  check("Serial.print(int) byte count", n3 == 5);

  // ---- Printable's byte-counting idiom now compiles AND works ----
  Point pt(3, 4);
  size_t pn = Serial.print(pt);
  Serial.println();
  check("Printable printTo() byte-count accumulation", pn == 5);

  // ---- Stream& genuine polymorphism ----
  Serial1.begin(9600);
  Serial1.setTimeout(1000);
  Serial1.print("xxxHELLOyyy");
  delay(50);
  bool found = findViaStreamRef(Serial1, "HELLO");
  check("Stream& polymorphism (find() via base reference)", found);

  // ---- Stream-inherited parseInt still works after the move ----
  Serial1.print("value=123;");
  delay(50);
  Serial1.find("=");
  long v = Serial1.parseInt();
  check("Stream::parseInt() after refactor", v == 123);

  Serial.println("done");
}

void loop() {
}
