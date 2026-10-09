/** analogRead() with a channel number: analogRead( 0 ) is A0, as on AVR
 *
 *  randomSeed( analogRead( 0 ) ) is the usual way to seed random(), and
 *  sketches written for AVR pass 0..5 for A0..A5. Up to 0.9.0 this core
 *  took 0 as D0, which has no ADC input, and stopped in panic().
 *
 *  No wiring. analogRead() turns the pin's input buffer off, as the ADC
 *  needs, so reading the pin's PORT control register after analogRead( n )
 *  shows which pin the number reached, whatever the floating pin reads.
 *
 *  Leave the analog pin below unconnected.
 */

#include <Arduino.h>
#include "pin_registry.h"

#if defined(FRDM_MCXA153) || defined(FRDM_MCXA156) || defined(FRDM_MCXN236)
const int ANALOG_PIN = A0;
#elif defined(FRDM_MCXN947)
const int ANALOG_PIN = A2;  // A0/A1 are not analog inputs on FRDM-MCXN947
#else
#error "This sketch has no settings for this board yet"
#endif

const int CHANNEL = ANALOG_PIN - A0;

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

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("=== analogRead() with a channel number ===");

  pinMode(ANALOG_PIN, INPUT_PULLUP);
  check("before: the analog pin's input buffer is on", inputBufferOn(ANALOG_PIN));

  int v = analogRead(CHANNEL);
  Serial.print("  analogRead(");
  Serial.print(CHANNEL);
  Serial.print(") = ");
  Serial.println(v);
  check("analogRead() by number reached the analog pin (its input buffer is off)", !inputBufferOn(ANALOG_PIN));
  check("the reading is in range", 0 <= v && v <= 1023);

  int w = analogRead(ANALOG_PIN);
  Serial.print("  analogRead(A");
  Serial.print(CHANNEL);
  Serial.print(") = ");
  Serial.println(w);
  check("analogRead() by name still works after it", 0 <= w && w <= 1023);

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
