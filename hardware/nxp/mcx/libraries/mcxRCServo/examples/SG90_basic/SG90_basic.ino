/** SG90 basic
 *
 *  The shortest sketch that moves a positional rotation servo: hand
 *  position() an angle and the servo goes there at its own speed.
 *
 *  range() picks the unit position() takes -- degrees here. Without a
 *  range() call the servo takes 0.0 to 1.0.
 *
 *  See SG90_moves for slower, shaped movement.
 *
 *  The signal line goes to PWM0 (D3 on FRDM-MCXC444, which has no
 *  PWM0-PWM5). This sketch was verified with the servo powered from the
 *  board, but give it its own 4.8V-6V supply (grounds tied together) once
 *  it has a load to move: stall current is past what the board can hand
 *  out.
 */

#include <Arduino.h>
#include <SG90.h>

//  FRDM-MCXC444 has no PWM0-PWM5. D3 has a timer (TPM1) to itself there,
//  so the servo's 50Hz leaves the other PWM pins' period alone
#if defined(FRDM_MCXC444)
#define SERVO_PIN D3
#else
#define SERVO_PIN PWM0
#endif

SG90 servo(SERVO_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  servo.range(-90, 90);
}

void loop() {
  servo.position(0);
  delay(500);
  servo.position(-90);
  delay(500);
  servo.position(0);
  delay(500);
  servo.position(90);
  delay(500);
}
