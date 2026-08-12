#include <Arduino.h>

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("String test");

  String a = "Hello";
  String b = String(", ") + "world" + String('!');
  String c = a + b;
  Serial.println(c);
  check("concat", c == "Hello, world!");

  String num = String(42) + " / " + String(3.14, 2);
  Serial.println(num);
  check("numeric concat", num == "42 / 3.14");

  check("length", c.length() == 13);
  check("charAt", c.charAt(0) == 'H');
  check("indexOf", c.indexOf("world") == 7);
  check("substring", c.substring(7, 12) == "world");
  check("startsWith", c.startsWith("Hello"));
  check("endsWith", c.endsWith("!"));

  String upper = c;
  upper.toUpperCase();
  Serial.println(upper);
  check("toUpperCase", upper == "HELLO, WORLD!");

  String spaced = "   trim me   ";
  spaced.trim();
  check("trim", spaced == "trim me");

  String replaced = c;
  replaced.replace("world", "there");
  Serial.println(replaced);
  check("replace", replaced == "Hello, there!");

  String n = "12345";
  check("toInt", n.toInt() == 12345);

  String f = "3.5";
  check("toFloat", f.toFloat() > 3.49f && f.toFloat() < 3.51f);
}

void loop() {
}
