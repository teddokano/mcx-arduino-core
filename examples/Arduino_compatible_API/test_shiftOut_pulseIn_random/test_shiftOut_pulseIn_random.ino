/** shiftOut/shiftIn, pulseIn/pulseInLong, random/randomSeed test
 *
 *  Wiring needed:
 *    1) Jumper D5 (SHIFTOUT_DATA) <-> D8 (MONITOR_DATA)
 *       Jumper D6 (SHIFTOUT_CLOCK) <-> D9 (MONITOR_CLOCK)
 *       shiftOut() and shiftIn() both try to drive the clock themselves, so
 *       they can't loop back to each other directly on one MCU. Instead,
 *       attachInterrupt() on MONITOR_CLOCK's rising edge captures
 *       MONITOR_DATA in real time -- the same "digitalRead synced to a
 *       clock edge" mechanism shiftIn() itself uses internally -- so the
 *       byte shiftOut() actually sent can be verified bit-for-bit.
 *    2) Jumper D10 (SHIFTIN_DATA) <-> D11 (SHIFTIN_CLOCK)
 *       Standalone shiftIn() sanity check: shiftIn() drives CLOCK high
 *       right before sampling DATA, so with DATA tied directly to CLOCK,
 *       every sampled bit reads 1 -- shiftIn() should return 0xFF.
 *    3) Jumper D13 (TONE_PIN) <-> D7 (PULSE_MONITOR_PIN)
 *       tone(D13, 1000) is a 1kHz square wave (500us high / 500us low).
 *       pulseIn()/pulseInLong() on D7 should read ~500 (microseconds).
 */

#include <Arduino.h>

#define SHIFTOUT_DATA      D5
#define SHIFTOUT_CLOCK     D6
#define MONITOR_DATA       D8
#define MONITOR_CLOCK      D9
#define SHIFTIN_DATA       D10
#define SHIFTIN_CLOCK      D11
#define TONE_PIN           D13
#define PULSE_MONITOR_PIN  D7

volatile uint8_t captured     = 0;
volatile uint8_t bitsCaptured = 0;

void onMonitorClockRise() {
  captured = (uint8_t)((captured << 1) | (digitalRead(MONITOR_DATA) ? 1 : 0));
  bitsCaptured = bitsCaptured + 1;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  pinMode(SHIFTOUT_DATA, OUTPUT);
  pinMode(SHIFTOUT_CLOCK, OUTPUT);
  pinMode(MONITOR_DATA, INPUT);
  pinMode(MONITOR_CLOCK, INPUT);
  pinMode(SHIFTIN_DATA, INPUT);
  pinMode(SHIFTIN_CLOCK, OUTPUT);
  pinMode(PULSE_MONITOR_PIN, INPUT);

  Serial.println("--- shiftOut + interrupt-captured shiftIn-equivalent (D5<->D8, D6<->D9) ---");
  attachInterrupt(digitalPinToInterrupt(MONITOR_CLOCK), onMonitorClockRise, RISING);

  uint8_t sent = 0xB6;
  captured     = 0;
  bitsCaptured = 0;

  shiftOut(SHIFTOUT_DATA, SHIFTOUT_CLOCK, MSBFIRST, sent);
  delay(5);  // let the last interrupt land

  Serial.print("sent     = 0x"); Serial.println(sent, HEX);
  Serial.print("captured = 0x"); Serial.println(captured, HEX);
  Serial.print("bits     = "); Serial.println(bitsCaptured);
  Serial.println(captured == sent && bitsCaptured == 8 ? "MATCH" : "MISMATCH");

  Serial.println("--- shiftIn standalone sanity (D10<->D11 jumper, expect 0xFF) ---");
  uint8_t sIn = shiftIn(SHIFTIN_DATA, SHIFTIN_CLOCK, MSBFIRST);
  Serial.print("shiftIn() = 0x");
  Serial.println(sIn, HEX);

  Serial.println("--- pulseIn / pulseInLong (D13<->D7 jumper, tone 1kHz) ---");
  tone(TONE_PIN, 1000);
  unsigned long w1 = pulseIn(PULSE_MONITOR_PIN, HIGH, 100000UL);
  Serial.print("pulseIn(HIGH) us = ");
  Serial.println(w1);

  unsigned long w2 = pulseInLong(PULSE_MONITOR_PIN, HIGH, 100000UL);
  Serial.print("pulseInLong(HIGH) us = ");
  Serial.println(w2);
  noTone(TONE_PIN);

  Serial.println("--- random / randomSeed ---");
  randomSeed(42);
  for (int i = 0; i < 5; i++) {
    Serial.print("random(100) = ");
    Serial.println(random(100));
  }
  for (int i = 0; i < 5; i++) {
    Serial.print("random(10,20) = ");
    Serial.println(random(10, 20));
  }
}

void loop() {
}
