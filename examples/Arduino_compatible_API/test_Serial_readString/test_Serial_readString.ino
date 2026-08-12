/** Serial.readString()/readStringUntil(), and String::reserve/getBytes/
 *  toCharArray/startsWith(offset)
 *
 *  The readString parts need Serial1 (D1->D0 jumper) as a loopback source;
 *  the String parts are pure logic and need no hardware.
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

  Serial.println("Serial.readString / String extras test");

  // ---- Serial1.readString() ----
  Serial1.print("Hello");
  delay(300);  // let it fully arrive, then let readString() time out collecting it
  String s1 = Serial1.readString();
  Serial.print("readString: \"");
  Serial.print(s1);
  Serial.println("\"");
  check("readString", s1 == "Hello");

  // ---- Serial1.readStringUntil() ----
  Serial1.print("abc;def");
  delay(50);
  String s2 = Serial1.readStringUntil(';');
  Serial.print("readStringUntil: \"");
  Serial.print(s2);
  Serial.println("\"");
  check("readStringUntil", s2 == "abc");

  String s3 = Serial1.readString();  // leftover, then times out
  check("readStringUntil leftover", s3 == "def");

  // ---- String::reserve/getBytes/toCharArray/startsWith(offset) ----
  String s4 = "Hello, world!";
  check("reserve (no-op, always true)", s4.reserve(64));

  char buf[8] = {0};
  s4.getBytes((unsigned char *)buf, sizeof(buf));
  Serial.print("getBytes (7 of 13 chars): \"");
  Serial.print(buf);
  Serial.println("\"");
  check("getBytes truncates + null-terminates", strcmp(buf, "Hello, ") == 0);

  char buf2[20] = {0};
  s4.toCharArray(buf2, sizeof(buf2));
  check("toCharArray full copy", strcmp(buf2, "Hello, world!") == 0);

  check("startsWith(s, offset)", s4.startsWith("world", 7) && !s4.startsWith("world", 0));
}

void loop() {
}
