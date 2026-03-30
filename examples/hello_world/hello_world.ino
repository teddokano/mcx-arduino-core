#include "arduino.h"

void setup() {
  Serial.begin(115200);
  Serial.println("Hello, world!");

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
}

#define WAIT  100

void loop() {
  digitalWrite(RED, LOW);
  delay(WAIT);
  digitalWrite(RED, HIGH);
  delay(WAIT);
  digitalWrite(GREEN, LOW);
  delay(WAIT);
  digitalWrite(GREEN, HIGH);
  delay(WAIT);
  digitalWrite(BLUE, LOW);
  delay(WAIT);
  digitalWrite(BLUE, HIGH);
  delay(WAIT);
}