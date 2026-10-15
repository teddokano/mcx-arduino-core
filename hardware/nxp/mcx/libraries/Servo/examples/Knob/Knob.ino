/** Knob: sets a servo's angle from a potentiometer.
 *
 *  Wiring: the servo's signal wire to PWM0 (D9 on FRDM-MCXC444), its + to 5V, its - to GND.
 *  The potentiometer's ends to 3.3V and GND, its wiper to A2. Use 3.3V,
 *  not 5V: the analog inputs take at most 3.3V.
 *
 *  A2 is an analog input on all five boards (FRDM-MCXN947 has none on A0
 *  or A1). For the servo pin, see the Sweep example.
 */

#include <Servo.h>

#if defined(FRDM_MCXC444)
#define SERVO_PIN D9    // no PWM0-PWM5 on this board; D9 has PWM, as on an UNO
#else
#define SERVO_PIN PWM0
#endif
#define KNOB_PIN  A2

Servo servo;

void setup() {
  servo.attach(SERVO_PIN);
}

void loop() {
  int value = analogRead(KNOB_PIN);   // 0-1023
  servo.write(map(value, 0, 1023, 0, 180));
  delay(15);
}
