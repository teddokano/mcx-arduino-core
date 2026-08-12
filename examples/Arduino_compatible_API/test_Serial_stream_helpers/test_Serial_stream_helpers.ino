/** Serial Stream-style helpers test
 *
 *  setTimeout()/readBytes()/readBytesUntil()/parseInt()/parseFloat()/find()
 *  Uses Serial1 (D1->D0 jumper) as a loopback source for each case, plus a
 *  final no-data case to check the timeout path actually blocks for
 *  roughly the configured duration instead of returning immediately.
 */

#include <Arduino.h>
#include <cstring>

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial1.begin(9600);
  Serial1.setTimeout(500);

  Serial.println("Serial Stream-style helpers test");

  // ---- readBytes ----
  Serial1.print("Hello");
  delay(50);
  char buf1[16] = {0};
  size_t n1 = Serial1.readBytes(buf1, 5);
  Serial.print("readBytes: \"");
  Serial.print(buf1);
  Serial.println("\"");
  check("readBytes", n1 == 5 && strcmp(buf1, "Hello") == 0);

  // ---- readBytesUntil ----
  Serial1.print("abc;def");
  delay(50);
  char buf2[16] = {0};
  size_t n2 = Serial1.readBytesUntil(';', buf2, sizeof(buf2) - 1);
  Serial.print("readBytesUntil: \"");
  Serial.print(buf2);
  Serial.println("\"");
  check("readBytesUntil count/content", n2 == 3 && strcmp(buf2, "abc") == 0);

  char buf3[16] = {0};
  size_t n3 = Serial1.readBytes(buf3, 3);  // remainder after the terminator
  check("readBytesUntil leftover", n3 == 3 && strcmp(buf3, "def") == 0);

  // ---- parseInt ----
  Serial1.print("12345\n");
  delay(50);
  long v1 = Serial1.parseInt();
  Serial.print("parseInt: ");
  Serial.println(v1);
  check("parseInt positive", v1 == 12345);

  Serial1.print("-42\n");
  delay(50);
  long v2 = Serial1.parseInt();
  Serial.print("parseInt (negative): ");
  Serial.println(v2);
  check("parseInt negative", v2 == -42);

  // ---- parseFloat ----
  Serial1.print("3.14\n");
  delay(50);
  float f1 = Serial1.parseFloat();
  Serial.print("parseFloat: ");
  Serial.println(f1, 4);
  check("parseFloat", f1 > 3.139f && f1 < 3.141f);

  // ---- find ----
  Serial1.print("xxxTARGETyyy");
  delay(50);
  bool found = Serial1.find("TARGET");
  check("find", found);

  char buf4[8] = {0};
  size_t n4 = Serial1.readBytes(buf4, 3);
  Serial.print("find leftover: \"");
  Serial.print(buf4);
  Serial.println("\"");
  check("find leftover", n4 == 3 && strcmp(buf4, "yyy") == 0);

  // ---- timeout path (nothing sent) ----
  unsigned long t0 = millis();
  long v3 = Serial1.parseInt();
  unsigned long elapsed = millis() - t0;
  Serial.print("timeout parseInt() blocked for ");
  Serial.print(elapsed);
  Serial.println(" ms (setTimeout was 500 ms)");
  check("timeout path", v3 == 0 && elapsed >= 480 && elapsed <= 700);
}

void loop() {
}
