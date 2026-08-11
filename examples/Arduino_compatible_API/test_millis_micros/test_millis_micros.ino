#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("millis / micros test");
}

void loop() {
  Serial.print("millis = ");
  Serial.print(millis());
  Serial.print("  micros = ");
  Serial.println(micros());

  delay(500);
}
