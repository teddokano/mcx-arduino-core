#include <Arduino.h>
// deliberately no #include <math.h> -- matches UNO R3/R4 behavior

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.print("PI=");
  Serial.println(PI, 6);

  Serial.print("HALF_PI=");
  Serial.println(HALF_PI, 6);

  Serial.print("TWO_PI=");
  Serial.println(TWO_PI, 6);

  Serial.print("radians(180)=");
  Serial.println(radians(180.0), 6);

  Serial.print("degrees(PI)=");
  Serial.println(degrees(PI), 6);

  Serial.print("sin(HALF_PI)=");
  Serial.println(sin(HALF_PI), 6);

  Serial.print("sqrt(2.0)=");
  Serial.println(sqrt(2.0), 6);
}

void loop() {
}
