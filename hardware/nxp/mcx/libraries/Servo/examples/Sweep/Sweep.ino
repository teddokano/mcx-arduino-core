/** Sweep: turns a servo back and forth between 0 and 180 degrees.
 *
 *  Wiring: the servo's signal wire to D9, its + to 5V, its - to GND.
 *  A small servo such as an SG90 can run from the board's 5V; for more
 *  than one, or a larger one, use a separate 5V supply and join its GND
 *  to the board's.
 *
 *  D9, as in the official library's example, on every board. Any digital
 *  pin works. On a pin with PWM -- PWM0-PWM5, and also D3, D5, D6 and D9
 *  on FRDM-MCXA156 and FRDM-MCXN236, and D3, D5, D6, D9 and D10 on
 *  FRDM-MCXC444 -- the pulses come from the PWM hardware and don't jitter.
 *  On any other pin, D9 on FRDM-MCXA153 and FRDM-MCXN947 among them, a
 *  timer interrupt makes them, as on AVR, and other interrupts can move an
 *  edge by a few microseconds.
 */

#include <Servo.h>

#define SERVO_PIN D9

Servo servo;

void setup() {
  servo.attach(SERVO_PIN);
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
