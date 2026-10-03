/** attachInterrupt() leaves the pin as pinMode() set it, as on AVR:
 *  pinMode(pin, INPUT_PULLUP) then attachInterrupt() keeps the pull-up,
 *  and an OUTPUT pin stays an output at the level it was driving.
 *
 *  Up to 0.7.1 the first attachInterrupt() on a pin set it to input with
 *  no pull, so a button or an open-drain INT line with no pull-up of its
 *  own was left floating. And CHANGE fired on the falling edge only: it
 *  set the pin to the rising edge and then to the falling edge, and a pin
 *  has one interrupt setting, so the second replaced the first.
 *
 *  An unconnected pin with no pull keeps whatever level it last had for a
 *  long while, so reading it proves nothing. Each pull is checked by
 *  driving the pin the other way through the GPIO registers -- which
 *  leaves the pull setting alone, unlike pinMode() -- letting it go, and
 *  seeing the pull bring it back. The output check counts the interrupts
 *  the pin's own level changes raise, which also checks CHANGE sees both
 *  edges.
 *
 *  Wiring: none. D2, D4 and D5 must be left unconnected.
 *
 *  Automatic: reads "ALL OK" or "N FAILED" at the end.
 */

#include <Arduino.h>
#include "pin_registry.h"

int failCount = 0;

void check(const char *label, bool ok) {
  Serial.print(label);
  Serial.print(": ");
  Serial.println(ok ? "OK" : "FAIL");
  if (!ok)
    failCount++;
}

// 0 none, 1 pull-down, 2 pull-up
uint8_t pullOf(int pin) {
  return pin_registry_read_pcr(arduino_pin_by_number[pin]).pull;
}

// Drive the pin to `level` for a moment, let go, and read it 200us later.
// A pull of the other polarity brings it back well within that; a pin
// with no pull still reads `level`.
bool levelAfterRelease(int pin, bool level) {
  GPIO_Type *port = digitalPinToPort(pin);
  uint32_t mask = digitalPinToBitMask(pin);
  if (level)
    *portOutputRegister(port) |= mask;
  else
    *portOutputRegister(port) &= ~mask;
  *portModeRegister(port) |= mask;
  delayMicroseconds(50);
  *portModeRegister(port) &= ~mask;
  delayMicroseconds(200);
  return (*portInputRegister(port) & mask) != 0;
}

volatile int falls = 0;
void onFall() { falls = falls + 1; }

volatile int rises = 0;
void onRise() { rises = rises + 1; }

volatile int changes = 0;
void onChange() { changes = changes + 1; }

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("=== attachInterrupt() keeps pinMode() (no wiring; D2, D4, D5 unconnected) ===");

  Serial.println("--- INPUT_PULLUP, then attachInterrupt(FALLING) ---");
  pinMode(D2, INPUT_PULLUP);
  delay(1);
  attachInterrupt(digitalPinToInterrupt(D2), onFall, FALLING);
  delay(1);
  check("no interrupt from attaching", falls == 0);
  check("pull-up still on (PCR)", pullOf(D2) == 2);
  check("pulled back HIGH after being driven LOW", levelAfterRelease(D2, LOW) == HIGH);
  check("... and that falling edge was caught", falls == 1);

  detachInterrupt(digitalPinToInterrupt(D2));
  attachInterrupt(digitalPinToInterrupt(D2), onFall, FALLING);  // the second attach, on the same pin
  check("attach again after detach: still pulled up", levelAfterRelease(D2, LOW) == HIGH && falls == 2);
  detachInterrupt(digitalPinToInterrupt(D2));

  Serial.println("--- INPUT_PULLDOWN, then attachInterrupt(RISING) ---");
  pinMode(D4, INPUT_PULLDOWN);
  delay(1);
  attachInterrupt(digitalPinToInterrupt(D4), onRise, RISING);
  delay(1);
  check("pull-down still on (PCR)", pullOf(D4) == 1);
  check("pulled back LOW after being driven HIGH", levelAfterRelease(D4, HIGH) == LOW);
  check("... and that rising edge was caught", rises == 1);
  detachInterrupt(digitalPinToInterrupt(D4));

  Serial.println("--- CHANGE on an input ---");
  attachInterrupt(digitalPinToInterrupt(D2), onChange, CHANGE);
  levelAfterRelease(D2, LOW);
  delay(1);
  check("driven LOW and let go: both edges caught", changes == 2);
  detachInterrupt(digitalPinToInterrupt(D2));

  Serial.println("--- OUTPUT HIGH, then attachInterrupt(CHANGE) ---");
  changes = 0;
  pinMode(D5, OUTPUT);
  digitalWrite(D5, HIGH);
  attachInterrupt(digitalPinToInterrupt(D5), onChange, CHANGE);
  GPIO_Type *port = digitalPinToPort(D5);
  uint32_t mask = digitalPinToBitMask(D5);
  check("still an output (PDDR)", (*portModeRegister(port) & mask) != 0);
  check("still driving HIGH", digitalRead(D5) == HIGH);
  digitalWrite(D5, LOW);
  delay(1);
  digitalWrite(D5, HIGH);
  delay(1);
  check("writing LOW then HIGH raises 2 interrupts on the pin itself", changes == 2);
  detachInterrupt(digitalPinToInterrupt(D5));

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
