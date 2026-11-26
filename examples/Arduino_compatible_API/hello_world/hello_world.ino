#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Hello, world!");

  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
}

#define WAIT  100

void loop() {
  digitalWrite(LEDR, LOW);
  delay(WAIT);
  digitalWrite(LEDR, HIGH);
  delay(WAIT);
  digitalWrite(LEDG, LOW);
  delay(WAIT);
  digitalWrite(LEDG, HIGH);
  delay(WAIT);
  digitalWrite(LEDB, LOW);
  delay(WAIT);
  digitalWrite(LEDB, HIGH);
  delay(WAIT);
}