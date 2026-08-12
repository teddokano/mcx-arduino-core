/** String operator+(numeric/F()), Printable, NOT_AN_INTERRUPT,
 *  digitalPinToPort/BitMask + portOutput/Input/ModeRegister test.
 *
 *  Wiring needed: D2-D3 jumper (fast-GPIO register test drives D2,
 *  observes it on D3).
 */

#include <Arduino.h>

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
}

class Point : public Printable {
public:
  Point(int x, int y) : _x(x), _y(y) {}
  size_t printTo(Print &p) const override {
    p.print('(');
    p.print(_x);
    p.print(',');
    p.print(_y);
    p.print(')');
    return 0;  // this core's print() overloads return void, not a byte
               // count, so there's nothing meaningful to accumulate here
  }

private:
  int _x, _y;
};

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("String+numeric/F(), Printable, NOT_AN_INTERRUPT, fast GPIO test");

  // ---- String operator+ with numeric types / F() ----
  String s1 = String("x=") + 42;
  check("String + int", s1 == "x=42");

  String s2 = String("n=") + (-7);
  check("String + negative int", s2 == "n=-7");

  String s3 = String("u=") + 4000000000UL;
  check("String + unsigned long", s3 == "u=4000000000");

  String s4 = String("ll=") + 123456789012LL;
  check("String + long long", s4 == "ll=123456789012");

  String s5 = String("pi=") + 3.5f;
  Serial.print("String + float result: ");
  Serial.println(s5);
  check("String + float", s5.startsWith("pi=3.5"));

  String s6 = String("f=") + F("flash");
  check("String + F()", s6 == "f=flash");

  // ---- Printable ----
  Point pt(3, 4);
  Serial.print("Printable point: ");
  Serial.println(pt);

  // ---- NOT_AN_INTERRUPT ----
  check("NOT_AN_INTERRUPT == -1", NOT_AN_INTERRUPT == -1);
  check("NOT_AN_INTERRUPT != a real pin's digitalPinToInterrupt()",
        digitalPinToInterrupt(D2) != NOT_AN_INTERRUPT);

  // ---- digitalPinToPort/BitMask + portOutput/Input/ModeRegister ----
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

  Serial.println("done");
}

void loop() {
}
