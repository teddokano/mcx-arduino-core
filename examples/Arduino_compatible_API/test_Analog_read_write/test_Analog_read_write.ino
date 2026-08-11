#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("analogRead / analogWrite test");
}

void loop() {
  int value = analogRead(A0);

  analogWrite(PWM0, value >> 2);   // 10bit -> 8bit

  Serial.print("A0 = ");
  Serial.println(value);

  delay(200);
}
