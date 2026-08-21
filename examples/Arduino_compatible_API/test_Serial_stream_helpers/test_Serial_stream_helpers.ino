/** Serial/Stream helper family, consolidated into one sketch for easier
 *  hardware verification (previously four separate examples).
 *
 *  Covers:
 *   - setTimeout()/readBytes()/readBytesUntil()/parseInt()/parseFloat()/
 *     find(), plus the timeout path actually blocking for roughly the
 *     configured duration instead of returning immediately
 *   - readString()/readStringUntil(), plus String::reserve/getBytes/
 *     toCharArray/startsWith(offset) (pure logic, no hardware needed)
 *   - flush() blocking for the real hardware TX-complete time, not just
 *     the software TX ring buffer draining
 *   - peek() not consuming the byte it returns, and available() staying
 *     unchanged until read() actually consumes it
 *
 *  Wiring needed: D0-D1 jumper (Serial1 loopback) for everything except
 *  the String extras section.
 */

#include <Arduino.h>
#include <cstring>

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

  Serial.println("Serial/Stream helpers test (readBytes/parseInt/parseFloat/find/readString/flush/peek)");

  Serial1.begin(9600);
  Serial1.setTimeout(500);

  // ---- readBytes ----
  Serial1.print("Hello");
  delay(50);
  char buf1[16] = {0};
  size_t n1 = Serial1.readBytes(buf1, 5);
  check("readBytes", n1 == 5 && strcmp(buf1, "Hello") == 0);

  // ---- readBytesUntil ----
  Serial1.print("abc;def");
  delay(50);
  char buf2[16] = {0};
  size_t n2 = Serial1.readBytesUntil(';', buf2, sizeof(buf2) - 1);
  check("readBytesUntil count/content", n2 == 3 && strcmp(buf2, "abc") == 0);

  char buf3[16] = {0};
  size_t n3 = Serial1.readBytes(buf3, 3);  // remainder after the terminator
  check("readBytesUntil leftover", n3 == 3 && strcmp(buf3, "def") == 0);

  // ---- parseInt ----
  Serial1.print("12345\n");
  delay(50);
  long v1 = Serial1.parseInt();
  check("parseInt positive", v1 == 12345);

  Serial1.print("-42\n");
  delay(50);
  long v2 = Serial1.parseInt();
  check("parseInt negative", v2 == -42);

  // ---- parseFloat ----
  Serial1.print("3.14\n");
  delay(50);
  float f1 = Serial1.parseFloat();
  check("parseFloat", f1 > 3.139f && f1 < 3.141f);

  // ---- find ----
  Serial1.print("xxxTARGETyyy");
  delay(50);
  check("find", Serial1.find("TARGET"));

  char buf4[8] = {0};
  size_t n4 = Serial1.readBytes(buf4, 3);
  check("find leftover", n4 == 3 && strcmp(buf4, "yyy") == 0);

  // ---- timeout path (nothing sent) ----
  unsigned long tTimeout0 = millis();
  long v3 = Serial1.parseInt();
  unsigned long timeoutElapsed = millis() - tTimeout0;
  check("timeout path", v3 == 0 && timeoutElapsed >= 480 && timeoutElapsed <= 700);

  // ---- readString ----
  Serial1.print("Hello");
  delay(300);  // let it fully arrive, then let readString() time out collecting it
  String s1 = Serial1.readString();
  check("readString", s1 == "Hello");

  // ---- readStringUntil ----
  Serial1.print("abc;def");
  delay(50);
  String s2 = Serial1.readStringUntil(';');
  check("readStringUntil", s2 == "abc");

  String s3 = Serial1.readString();  // leftover, then times out
  check("readStringUntil leftover", s3 == "def");

  // ---- String::reserve/getBytes/toCharArray/startsWith(offset) ----
  String s4 = "Hello, world!";
  check("String::reserve (no-op, always true)", s4.reserve(64));

  char bufA[8] = {0};
  s4.getBytes((unsigned char *)bufA, sizeof(bufA));
  check("String::getBytes truncates + null-terminates", strcmp(bufA, "Hello, ") == 0);

  char bufB[20] = {0};
  s4.toCharArray(bufB, sizeof(bufB));
  check("String::toCharArray full copy", strcmp(bufB, "Hello, world!") == 0);

  check("String::startsWith(s, offset)", s4.startsWith("world", 7) && !s4.startsWith("world", 0));

  // ---- flush() ----
  // A real hardware UART (not the USB-CDC bridge) at a slow baud rate, so
  // the physical transmission time is large enough to measure. If flush()
  // only waited for the software TX ring buffer to drain (and not for the
  // hardware shift register to actually finish), the measured time would
  // be much shorter than the expected transmission time.
  const char *flushMsg = "Hello, Serial1 world!\r\n";  // 24 bytes
  Serial1.print(flushMsg);
  unsigned long tFlush0 = micros();
  Serial1.flush();
  unsigned long flushElapsed = micros() - tFlush0;
  unsigned long flushExpected_us = (unsigned long)strlen(flushMsg) * 10UL * 1000000UL / 9600UL;  // 10 bits/byte @ 9600 baud
  Serial.print("flush() blocked for ");
  Serial.print(flushElapsed);
  Serial.print(" us (expected roughly ");
  Serial.print(flushExpected_us);
  Serial.println(" us)");
  check("flush() blocks for real hardware TX time", flushElapsed >= flushExpected_us / 2);

  // ---- peek() ----
  // peek() must return the next byte without consuming it -- calling it
  // repeatedly must keep returning the same byte, and available() must
  // stay unchanged, until read() actually consumes it.
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

  Serial.println();
  Serial.println(failCount == 0 ? "ALL OK" : "SOME FAILED");
}

void loop() {
}
