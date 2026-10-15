/** Sweep: turns a servo back and forth between 0 and 180 degrees.
 *
 *  Wiring: the servo's signal wire to PWM0, its + to 5V, its - to GND.
 *  A small servo such as an SG90 can run from the board's 5V; for more
 *  than one, or a larger one, use a separate 5V supply and join its GND
 *  to the board's.
 *
 *  The servo has to be on a pin with PWM: PWM0-PWM5, and also D3, D5, D6
 *  and D9 on FRDM-MCXA156 and FRDM-MCXN236. The same sketch for the
 *  official Servo library on AVR uses D9, which has no PWM on FRDM-MCXA153
 *  and FRDM-MCXN947.
 */

#include <Servo.h>

#define SERVO_PIN PWM0

Servo servo;

void setup() {
  Serial.begin(115200);
  if (servo.attach(SERVO_PIN) == INVALID_SERVO)
    Serial.println("SERVO_PIN has no PWM");
}

void loop() {
  for (int angle = 0; angle <= 180; angle++) {
    servo.write(angle);
    delay(15);
  }
  for (int angle = 180; angle >= 0; angle--) {
    servo.write(angle);
    delay(15);
  }
}
