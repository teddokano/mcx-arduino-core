/** Wire.end(), Serial.find(target,length)/findUntil(), Serial.availableForWrite(),
 *  INPUT_PULLDOWN, OUTPUT_OPENDRAIN test.
 *
 *  Wiring needed:
 *   - D0-D1 jumper (Serial1 TX/RX loopback, for find()/findUntil())
 *   - D2-D3 jumper (used for both the INPUT_PULLDOWN/INPUT_PULLUP rigor
 *     test and the OUTPUT_OPENDRAIN test, below)
 *  Uses the on-board P3T1755 over Wire1 for the Wire.end() test -- no
 *  extra wiring needed for that part.
 */

#include <P3T1755.h>
#include <Wire.h>

P3T1755 sensor(Wire1, 0x48);

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
}

bool sane(float t) {
  return t > -40.0 && t < 125.0;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Wire.end() / Serial.find(len) / findUntil() / availableForWrite() / pull modes test");

  // ---- Wire1.end() ----
  Wire1.begin();
  float t1 = sensor.temp();
  Serial.print("temp before end(): ");
  Serial.println(t1, 2);
  check("Wire1 read before end()", sane(t1));

  Wire1.end();
  Serial.println("Wire1.end() called");

  Wire1.begin();
  float t2 = sensor.temp();
  Serial.print("temp after end()+begin(): ");
  Serial.println(t2, 2);
  check("Wire1 read after end()+begin()", sane(t2));

  // ---- Serial.availableForWrite() ----
  int free1 = Serial.availableForWrite();
  Serial.print("availableForWrite (idle): ");
  Serial.println(free1);
  check("availableForWrite in range [0,255]", free1 >= 0 && free1 <= 255);

  for (int i = 0; i < 50; i++)
    Serial.write('A');
  int free2 = Serial.availableForWrite();
  Serial.print("availableForWrite (after 50-byte burst): ");
  Serial.println(free2);
  check("availableForWrite decreased after burst", free2 <= free1);

  Serial.flush();
  int free3 = Serial.availableForWrite();
  Serial.print("availableForWrite (after flush): ");
  Serial.println(free3);
  check("availableForWrite recovers after flush", free3 > free2 || free3 == 255);

  // ---- Serial1 loopback: find(target,length) / findUntil(target,terminator) ----
  Serial1.begin(9600);
  Serial1.setTimeout(1000);

  Serial1.print("xxxHELLOyyy");
  delay(50);
  bool f1 = Serial1.find("HELLO", 5);
  check("find(target,length)", f1);

  Serial1.print("xxTARGETyy");
  delay(50);
  bool f2 = Serial1.findUntil("TARGET", "STOP");
  check("findUntil() finds target before terminator", f2);

  Serial1.print("STOPxxxTARGETyyy");
  delay(50);
  unsigned long t_start = millis();
  bool f3 = Serial1.findUntil("TARGET", "STOP");
  unsigned long elapsed = millis() - t_start;
  Serial.print("findUntil() early-abort elapsed (ms): ");
  Serial.println(elapsed);
  check("findUntil() aborts on terminator (returns false)", !f3);
  check("findUntil() aborts quickly (not full timeout)", elapsed < 500);

  // ---- INPUT_PULLDOWN / INPUT_PULLUP, rigorously (D2-D3 jumper) ----
  // Just reading a pin claimed to have a pull enabled isn't a reliable
  // test on its own: a genuinely floating pin can happen to read the same
  // value a real pull would produce. That's exactly what let a real
  // PORT_SetPinPullUpDown() bug hide here for several releases --
  // INPUT_PULLDOWN silently left pins with no pull enabled at all (this
  // chip's floating inputs read LOW anyway), while this test's previous
  // form (`pinMode(D4, INPUT_PULLDOWN); check(digitalRead(D4) == LOW)`,
  // no jumper, nothing forcing the pin either way) kept reporting "OK"
  // regardless. Instead, force the shared node to the *opposite* level
  // with D2 as a real push-pull output, release D2 to true Hi-Z, and
  // check that D3's internal pull actually pulls the line back -- a
  // floating pin can't do that.
  pinMode(D3, INPUT_PULLDOWN);
  pinMode(D2, OUTPUT);
  digitalWrite(D2, HIGH);
  delayMicroseconds(50);
  pinMode(D2, INPUT);  // release D2 to true Hi-Z -- stop forcing the node
  delay(2);
  bool pd = digitalRead(D3);
  check("INPUT_PULLDOWN pulls the line back low after an external HIGH drive releases", pd == LOW);

  pinMode(D3, INPUT_PULLUP);
  pinMode(D2, OUTPUT);
  digitalWrite(D2, LOW);
  delayMicroseconds(50);
  pinMode(D2, INPUT);  // release D2 to true Hi-Z
  delay(2);
  bool pu = digitalRead(D3);
  check("INPUT_PULLUP pulls the line back high after an external LOW drive releases", pu == HIGH);

  // ---- OUTPUT_OPENDRAIN (D2) jumpered to D3 ----
  // D3's pull modes were just verified above to actually work, so they're
  // now trustworthy as the observation side for open-drain's own
  // "HIGH means released, not driven" behavior.
  pinMode(D2, OUTPUT_OPENDRAIN);
  pinMode(D3, INPUT_PULLDOWN);
  delay(2);

  digitalWrite(D2, LOW);
  delay(2);
  bool od_low = digitalRead(D3);
  digitalWrite(D2, HIGH);
  delay(2);
  bool od_high_with_pulldown = digitalRead(D3);
  check("open-drain LOW pulls partner LOW", od_low == LOW);
  check("open-drain HIGH does not drive high (partner pulldown keeps it LOW)", od_high_with_pulldown == LOW);

  pinMode(D3, INPUT_PULLUP);
  delay(2);
  digitalWrite(D2, LOW);
  delay(2);
  bool od_low2 = digitalRead(D3);
  digitalWrite(D2, HIGH);
  delay(2);
  bool od_high2 = digitalRead(D3);
  check("open-drain LOW overrides partner's pull-up", od_low2 == LOW);
  check("open-drain HIGH released -- partner pull-up pulls it HIGH", od_high2 == HIGH);

  Serial.println("done");
}

void loop() {
}
