#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("delayMicroseconds test");
}

void loop() {
  for (unsigned long target = 10; target <= 10000; target *= 10) {
    unsigned long t0 = micros();
    delayMicroseconds(target);
    unsigned long measured = micros() - t0;

    Serial.print("requested=");
    Serial.print(target);
    Serial.print(" measured=");
    Serial.print(measured);
    Serial.println(" us");

    if (measured < target)
      Serial.println("  <-- WARNING: measured shorter than requested!");
  }

  delay(1000);
}
