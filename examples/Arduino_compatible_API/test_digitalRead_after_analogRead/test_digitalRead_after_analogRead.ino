/** A pin used by analogRead() works as a digital pin again after
 *  pinMode(), as on AVR.
 *
 *  analogRead() turns the pin's input buffer off, as the ADC needs. Up to
 *  0.7.1 nothing turned it back on except on FRDM-MCXA156, so after
 *  analogRead(A0), pinMode(A0, INPUT_PULLUP) left digitalRead(A0) at 0 for
 *  good on FRDM-MCXA153 and FRDM-MCXN947, and an interrupt on the pin never
 *  fired.
 *
 *  The check reads the pin with its pull-up on: a pin with no input buffer
 *  reads 0 whatever its level, while a working one reads the pull-up's
 *  HIGH. (A pull-down would read 0 either way, so it would prove nothing.)
 *  Last, an interrupt is attached straight after analogRead(), and the pin
 *  pulled down and up again: the interrupt has to see that rising edge.
 *
 *  Wiring: none. Leave the two analog pins below unconnected.
 *
 *  FRDM-MCXC444 has no input buffer switch: a pin's digital input is off
 *  only while it is in its analog function (ALT0), which is what the
 *  "input buffer" checks read there. Its A0-A3 are on PORTB, which has no
 *  pin interrupts, so the second pin is A5 (PORTC). A5 has a 4.7k pull-up
 *  on the board (R27) that the pull-down cannot overcome, so there the
 *  rising edge comes from driving the pin LOW for a moment and letting go.
 *
 *  Automatic: reads "ALL OK" or "N FAILED" at the end.
 */

#include <Arduino.h>
#include "pin_registry.h"

#if defined(FRDM_MCXA153) || defined(FRDM_MCXA156)
// FRDM-MCXA156's A4/A5 are also on the CAN transceiver until R75/R76
// are removed, so stay on A0/A1
const int PIN_A = A0;
const int PIN_B = A1;
#elif defined(FRDM_MCXN947)
const int PIN_A = A2;  // A0/A1 are not analog inputs on FRDM-MCXN947
const int PIN_B = A3;
#elif defined(FRDM_MCXN236)
// A1/A2 are also on the CAN transceiver until R25/R67 are removed, and A3
// is not an analog input here (the blue LED's pin)
const int PIN_A = A0;
const int PIN_B = A4;
#elif defined(FRDM_MCXC444)
// A0-A3 have no pin interrupts (PORTB): the second pin is A5
const int PIN_A = A0;
const int PIN_B = A5;
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

bool inputBufferOn(int pin) {
  return pin_registry_read_pcr(arduino_pin_by_number[pin]).ibe;
}

volatile int rises = 0;
void onRise() { rises = rises + 1; }

#if defined(FRDM_MCXC444)
// Drive the pin LOW for a moment through the GPIO registers and let go
void pulseLow(int pin) {
  GPIO_Type *port = digitalPinToPort(pin);
  uint32_t mask = digitalPinToBitMask(pin);
  *portOutputRegister(port) &= ~mask;
  *portModeRegister(port) |= mask;
  delayMicroseconds(50);
  *portModeRegister(port) &= ~mask;
}
#endif

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("=== digitalRead() after analogRead() (no wiring) ===");

  Serial.println("--- pinMode() after analogRead() ---");
  pinMode(PIN_A, INPUT_PULLUP);
  delay(1);
  check("before analogRead(): reads the pull-up's HIGH", digitalRead(PIN_A) == HIGH);
  int v = analogRead(PIN_A);
  Serial.print("  analogRead = ");
  Serial.println(v);
  check("analogRead() turned the input buffer off", !inputBufferOn(PIN_A));
  pinMode(PIN_A, INPUT_PULLUP);
  delay(1);
  check("pinMode(INPUT_PULLUP) again: the input buffer is back", inputBufferOn(PIN_A));
  check("... and it reads the pull-up's HIGH", digitalRead(PIN_A) == HIGH);

  Serial.println("--- pinMode() never called before analogRead() ---");
  analogRead(PIN_B);
  pinMode(PIN_B, INPUT_PULLUP);
  delay(1);
  check("reads the pull-up's HIGH", digitalRead(PIN_B) == HIGH);

  Serial.println("--- attachInterrupt() after analogRead() ---");
  analogRead(PIN_B);
  attachInterrupt(digitalPinToInterrupt(PIN_B), onRise, RISING);
  check("attachInterrupt() turned the input buffer back on", inputBufferOn(PIN_B));
#if defined(FRDM_MCXC444)
  pulseLow(PIN_B);  // R27 pulls it back up: a rising edge
#else
  pinMode(PIN_B, INPUT_PULLDOWN);
  delay(1);
  pinMode(PIN_B, INPUT_PULLUP);  // pulled up again: a rising edge
#endif
  delay(1);
  check("the rising edge from the pull-up is caught", rises == 1);
  detachInterrupt(digitalPinToInterrupt(PIN_B));

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
