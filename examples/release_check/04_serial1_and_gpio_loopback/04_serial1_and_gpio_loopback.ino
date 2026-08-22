/** Release check 4/N: automatic OK/FAIL checks needing D0-D1 (Serial1
 *  loopback) and D2-D3 (fast-GPIO) jumpers -- both can stay installed at
 *  once, one flash covers everything here.
 *
 *  Consolidates (from examples/Arduino_compatible_API/):
 *  test_Print_Stream_hierarchy, test_Serial1, test_Serial_BIN_and_write,
 *  test_Serial_stream_helpers, test_String_plus_numeric_Printable_fastGPIO.
 *
 *  Wiring needed: D0-D1 jumper (Serial1 TX/RX loopback), D2-D3 jumper
 *  (fast-GPIO register test drives D2, observes it on D3).
 */

#include <Arduino.h>
#include <cstring>
#include "fsl_gpio.h"

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

// A Print-derived class with no hardware/pins at all -- this is exactly
// the pattern (e.g. ArduinoJson's internal StringBuilderPrint,
// LiquidCrystal) that couldn't compile when Print was just an alias for
// the pin-constructor-requiring SerialClass. File-scope, not declared
// inside setup() -- see 01_no_wiring_checks's FlakyPrint for why.
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

  Serial.println("=== Release check 4/N: Serial1 + fast-GPIO checks (D0-D1, D2-D3 jumpers) ===");

  Serial1.begin(9600);
  Serial1.setTimeout(500);

  // ---- Print without hardware, byte-count return values, Printable
  //      (was test_Print_Stream_hierarchy, part 1) ----
  Serial.println("--- Print/Printable, no hardware needed ---");
  {
    BufferPrint bp;
    size_t bn = bp.print("hello ");
    bn += bp.print(42);
    check("Print-derived class with no hardware works", strcmp(bp.c_str(), "hello 42") == 0);
    check("BufferPrint byte count", bn == 8);

    size_t n1 = Serial.print("abc");
    check("Serial.print(const char*) byte count", n1 == 3);

    size_t n2 = Serial.println("abcd");
    check("Serial.println byte count (incl CRLF)", n2 == 6);

    size_t n3 = Serial.print(12345);
    check("Serial.print(int) byte count", n3 == 5);

    Point pt(3, 4);
    size_t pn = Serial.print(pt);
    Serial.println();
    check("Printable printTo() byte-count accumulation", pn == 5);
  }

  // ---- basic Serial1 loopback sanity (was test_Serial1) ----
  Serial.println("--- Serial1 basic loopback ---");
  {
    Serial1.println("hello from Serial1");
    delay(50);
    char buf[24] = {0};
    int i = 0;
    while (Serial1.available() && i < 23)
      buf[i++] = (char)Serial1.read();
    check("Serial1 basic loopback", strncmp(buf, "hello from Serial1", 18) == 0);
  }

  // ---- Serial.print(x, BIN) + Serial1.write() overloads
  //      (was test_Serial_BIN_and_write) ----
  Serial.println("--- Serial print BIN / write() overloads ---");
  check("255 BIN == 11111111", String(255, BIN) == "11111111");
  check("5 BIN == 101", String(5, BIN) == "101");
  check("0 BIN == 0", String(0, BIN) == "0");
  check("-5 BIN == -101", String(-5, BIN) == "-101");

  {
    size_t n1 = Serial1.write((uint8_t)'A');
    delay(30);
    int r1 = Serial1.read();
    check("write(uint8_t) returns 1 and round-trips", n1 == 1 && r1 == 'A');

    size_t n2 = Serial1.write("Hello");
    delay(30);
    char buf[8] = {0};
    int i = 0;
    while (Serial1.available() && i < 7)
      buf[i++] = (char)Serial1.read();
    check("write(const char*) returns strlen and round-trips", n2 == 5 && strcmp(buf, "Hello") == 0);

    uint8_t raw[4] = {0x41, 0x00, 0x42, 0xFF};  // includes a null byte
    size_t n3 = Serial1.write(raw, sizeof(raw));
    delay(30);
    uint8_t rawback[4] = {0};
    int j = 0;
    while (Serial1.available() && j < 4)
      rawback[j++] = (uint8_t)Serial1.read();
    bool rawMatch = (j == 4) && rawback[0] == 0x41 && rawback[1] == 0x00 && rawback[2] == 0x42 && rawback[3] == 0xFF;
    check("write(buffer, size) handles null byte, count-based", n3 == 4 && rawMatch);
  }

  // ---- Stream& genuine polymorphism (was test_Print_Stream_hierarchy,
  //      part 2 -- find()/parseInt() themselves are covered more
  //      thoroughly in the Stream-helpers section below, so only the
  //      unique part -- calling find() through a real Stream& parameter,
  //      not SerialClass directly -- is kept here) ----
  Serial.println("--- Stream& polymorphism ---");
  {
    Serial1.print("xxxHELLOyyy");
    delay(50);
    bool found = findViaStreamRef(Serial1, "HELLO");
    check("Stream& polymorphism (find() via base reference)", found);

    // find() only consumes up through the match ("xxxHELLO"); "yyy" is
    // still sitting unread. Drain it before the Stream-helpers section
    // below starts, or its first readBytes() would see "yyyHello"
    // instead of "Hello".
    while (Serial1.available())
      Serial1.read();
  }

  // ---- Stream-style helpers: readBytes/readBytesUntil/parseInt/
  //      parseFloat/find/timeout, readString/readStringUntil + String
  //      extras, flush(), peek() (was test_Serial_stream_helpers) ----
  Serial.println("--- Stream helpers (readBytes/parseInt/parseFloat/find/readString/flush/peek) ---");
  {
    Serial1.print("Hello");
    delay(50);
    char buf1[16] = {0};
    size_t n1 = Serial1.readBytes(buf1, 5);
    check("readBytes", n1 == 5 && strcmp(buf1, "Hello") == 0);

    Serial1.print("abc;def");
    delay(50);
    char buf2[16] = {0};
    size_t n2 = Serial1.readBytesUntil(';', buf2, sizeof(buf2) - 1);
    check("readBytesUntil count/content", n2 == 3 && strcmp(buf2, "abc") == 0);

    char buf3[16] = {0};
    size_t n3 = Serial1.readBytes(buf3, 3);
    check("readBytesUntil leftover", n3 == 3 && strcmp(buf3, "def") == 0);

    Serial1.print("12345\n");
    delay(50);
    long v1 = Serial1.parseInt();
    check("parseInt positive", v1 == 12345);

    Serial1.print("-42\n");
    delay(50);
    long v2 = Serial1.parseInt();
    check("parseInt negative", v2 == -42);

    Serial1.print("3.14\n");
    delay(50);
    float f1 = Serial1.parseFloat();
    check("parseFloat", f1 > 3.139f && f1 < 3.141f);

    Serial1.print("xxxTARGETyyy");
    delay(50);
    check("find", Serial1.find("TARGET"));

    char buf4[8] = {0};
    size_t n4 = Serial1.readBytes(buf4, 3);
    check("find leftover", n4 == 3 && strcmp(buf4, "yyy") == 0);

    unsigned long tTimeout0 = millis();
    long v3 = Serial1.parseInt();
    unsigned long timeoutElapsed = millis() - tTimeout0;
    check("timeout path", v3 == 0 && timeoutElapsed >= 480 && timeoutElapsed <= 700);

    Serial1.print("Hello");
    delay(300);
    String s1 = Serial1.readString();
    check("readString", s1 == "Hello");

    Serial1.print("abc;def");
    delay(50);
    String s2 = Serial1.readStringUntil(';');
    check("readStringUntil", s2 == "abc");

    String s3 = Serial1.readString();
    check("readStringUntil leftover", s3 == "def");

    String s4 = "Hello, world!";
    check("String::reserve (no-op, always true)", s4.reserve(64));

    char bufA[8] = {0};
    s4.getBytes((unsigned char *)bufA, sizeof(bufA));
    check("String::getBytes truncates + null-terminates", strcmp(bufA, "Hello, ") == 0);

    char bufB[20] = {0};
    s4.toCharArray(bufB, sizeof(bufB));
    check("String::toCharArray full copy", strcmp(bufB, "Hello, world!") == 0);

    check("String::startsWith(s, offset)", s4.startsWith("world", 7) && !s4.startsWith("world", 0));

    const char *flushMsg = "Hello, Serial1 world!\r\n";
    Serial1.print(flushMsg);
    unsigned long tFlush0 = micros();
    Serial1.flush();
    unsigned long flushElapsed = micros() - tFlush0;
    unsigned long flushExpected_us = (unsigned long)strlen(flushMsg) * 10UL * 1000000UL / 9600UL;
    check("flush() blocks for real hardware TX time", flushElapsed >= flushExpected_us / 2);

    // flush() only waits for TX; drain the loopback echo before peek().
    while (Serial1.available())
      Serial1.read();

    Serial1.print("AB");
    delay(50);
    int availBeforePeek = Serial1.available();
    int p1 = Serial1.peek();
    int p2 = Serial1.peek();
    int availAfterPeek = Serial1.available();
    int r = Serial1.read();
    int p3 = Serial1.peek();
    check("peek() doesn't consume; matches read(); advances after read()",
          p1 == 'A' && p2 == 'A' && availAfterPeek == availBeforePeek && r == 'A' && p3 == 'B');
  }

  // ---- String + numeric/F(), NOT_AN_INTERRUPT, fast GPIO register
  //      access (was test_String_plus_numeric_Printable_fastGPIO) ----
  Serial.println("--- String+numeric/F(), NOT_AN_INTERRUPT, fast GPIO ---");
  {
    check("String + int", (String("x=") + 42) == "x=42");
    check("String + negative int", (String("n=") + (-7)) == "n=-7");
    check("String + unsigned long", (String("u=") + 4000000000UL) == "u=4000000000");
    check("String + long long", (String("ll=") + 123456789012LL) == "ll=123456789012");

    String s5 = String("pi=") + 3.5f;
    check("String + float", s5.startsWith("pi=3.5"));

    check("String + F()", (String("f=") + F("flash")) == "f=flash");

    check("NOT_AN_INTERRUPT == -1", NOT_AN_INTERRUPT == -1);
    check("NOT_AN_INTERRUPT != a real pin's digitalPinToInterrupt()",
          digitalPinToInterrupt(D2) != NOT_AN_INTERRUPT);

    pinMode(D2, OUTPUT);
    pinMode(D3, INPUT);

    GPIO_Type *port = digitalPinToPort(D2);
    uint32_t mask = digitalPinToBitMask(D2);
    check("digitalPinToPort/BitMask non-null/nonzero", port != nullptr && mask != 0);

    volatile uint32_t *out = portOutputRegister(port);
    volatile uint32_t *in = portInputRegister(port);
    volatile uint32_t *modeReg = portModeRegister(port);
    check("portOutput/Input/ModeRegister non-null", out != nullptr && in != nullptr && modeReg != nullptr);
    check("portModeRegister reflects OUTPUT direction", (*modeReg & mask) != 0);

    *out |= mask;  // fast-GPIO set, bypassing digitalWrite()
    delay(2);
    bool fast_high = digitalRead(D3);
    bool self_read_high = (*in & mask) != 0;

    *out &= ~mask;  // fast-GPIO clear
    delay(2);
    bool fast_low = digitalRead(D3);
    bool self_read_low = (*in & mask) != 0;

    check("fast GPIO set drives partner (D3) HIGH", fast_high == HIGH);
    check("portInputRegister reflects own pin HIGH", self_read_high == true);
    check("fast GPIO clear drives partner (D3) LOW", fast_low == LOW);
    check("portInputRegister reflects own pin LOW", self_read_low == false);
  }

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
