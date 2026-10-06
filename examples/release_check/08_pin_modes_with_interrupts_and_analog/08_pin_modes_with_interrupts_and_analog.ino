/** Release check 08: automatic OK/FAIL checks, no physical wiring needed,
 *  that a pin keeps working as pinMode() set it after attachInterrupt()
 *  and after analogRead().
 *
 *  Consolidates (from examples/Arduino_compatible_API/):
 *  test_attachInterrupt_keeps_pinMode and test_digitalRead_after_analogRead.
 *  It is a sketch of its own because release_check/01 already fills
 *  FRDM-MCXA153's flash.
 *
 *  Up to 0.7.1, on every board:
 *    - the first attachInterrupt() on a pin made it an input with no pull,
 *      so pinMode(INPUT_PULLUP) then attachInterrupt() left it floating,
 *      and an OUTPUT pin stopped driving
 *    - CHANGE fired on the falling edge only
 *    - analogRead() turned the pin's input buffer off and only
 *      FRDM-MCXA156 turned it back on, so digitalRead() stayed at 0 on a
 *      pin once read with analogRead(), whatever pinMode() said
 *
 *  An unconnected pin with no pull keeps whatever level it last had for a
 *  long while, so reading it proves nothing. Each pull is checked by
 *  driving the pin the other way through the GPIO registers -- which
 *  leaves the pull setting alone, unlike pinMode() -- letting it go, and
 *  seeing the pull bring it back. The input buffer is checked with the
 *  pull-up on: a pin without one reads 0 whatever its level.
 *
 *  Wiring: none. Leave D2, D4, D5 and the two analog pins below (A0/A1,
 *  A2/A3 on FRDM-MCXN947, A0/A4 on FRDM-MCXN236) unconnected.
 *
 *  Automatic: reads "ALL OK" or "N FAILED" at the end.
 */

#include <Arduino.h>
#include "pin_registry.h"

#if defined(FRDM_MCXA153) || defined(FRDM_MCXA156)
// FRDM-MCXA156's A4/A5 are also on the CAN transceiver until R75/R76
// are removed, so stay on A0/A1
const int ANALOG_PIN_A = A0;
const int ANALOG_PIN_B = A1;
#elif defined(FRDM_MCXN947)
const int ANALOG_PIN_A = A2;  // A0/A1 are not analog inputs on FRDM-MCXN947
const int ANALOG_PIN_B = A3;
#elif defined(FRDM_MCXN236)
// A1/A2 are also on the CAN transceiver until R25/R67 are removed, and A3
// is not an analog input here (the blue LED's pin)
const int ANALOG_PIN_A = A0;
const int ANALOG_PIN_B = A4;
#else
#error "This sketch has no settings for this board yet"
#endif

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

bool inputBufferOn(int pin) {
  return pin_registry_read_pcr(arduino_pin_by_number[pin]).ibe;
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
  Serial.println("=== Release check 08: pin modes with interrupts and analogRead (no wiring) ===");

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

  // On a pin that hasn't had an interrupt yet: only the first attach
  // sets the pin up, so D2 and D4 can't show this any more
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
  pinMode(D5, INPUT);

  Serial.println("--- pinMode() after analogRead() ---");
  pinMode(ANALOG_PIN_A, INPUT_PULLUP);
  delay(1);
  check("before analogRead(): reads the pull-up's HIGH", digitalRead(ANALOG_PIN_A) == HIGH);
  int v = analogRead(ANALOG_PIN_A);
  Serial.print("  analogRead = ");
  Serial.println(v);
  check("analogRead() turned the input buffer off", !inputBufferOn(ANALOG_PIN_A));
  pinMode(ANALOG_PIN_A, INPUT_PULLUP);
  delay(1);
  check("pinMode(INPUT_PULLUP) again: the input buffer is back", inputBufferOn(ANALOG_PIN_A));
  check("... and it reads the pull-up's HIGH", digitalRead(ANALOG_PIN_A) == HIGH);

  Serial.println("--- pinMode() never called before analogRead() ---");
  analogRead(ANALOG_PIN_B);
  pinMode(ANALOG_PIN_B, INPUT_PULLUP);
  delay(1);
  check("reads the pull-up's HIGH", digitalRead(ANALOG_PIN_B) == HIGH);

  Serial.println("--- attachInterrupt() after analogRead() ---");
  analogRead(ANALOG_PIN_B);
  attachInterrupt(digitalPinToInterrupt(ANALOG_PIN_B), onRise, RISING);
  check("attachInterrupt() turned the input buffer back on", inputBufferOn(ANALOG_PIN_B));
  rises = 0;
  pinMode(ANALOG_PIN_B, INPUT_PULLDOWN);
  delay(1);
  pinMode(ANALOG_PIN_B, INPUT_PULLUP);  // pulled up again: a rising edge
  delay(1);
  check("the rising edge from the pull-up is caught", rises == 1);
  detachInterrupt(digitalPinToInterrupt(ANALOG_PIN_B));

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
