/** FS90R rotation
 *
 *  Runs an FS90R continuous rotation servo both ways at full speed, then
 *  ramps the speed through the whole range.
 *
 *  speed() takes -1.0 (CW at full speed) through +1.0 (CCW at full speed)
 *  and stop() outputs the center pulse. range() can swap in another unit:
 *  range(-100, 100) would make speed() take percent.
 *
 *  The servo has a 90us dead band around its stop pulse, so speed() values
 *  within about +/-0.056 of the center leave the shaft stopped -- the ramp
 *  below pauses around zero for that reason, not because it stalled.
 *
 *  The signal line goes to PWM0 (D3 on FRDM-MCXC444, which has no
 *  PWM0-PWM5). Give this servo its own 4.8V-6V supply, with the grounds
 *  tied together -- running it from the board's 5V on an FRDM-MCXA153
 *  pulled the rail down far enough to make the USB link to the PC unstable.
 *  A continuous rotation servo draws its running current for as long as it
 *  turns, unlike a positional servo that only moves in bursts.
 */

#include <Arduino.h>
#include <FS90R.h>

//  FRDM-MCXC444 has no PWM0-PWM5. D3 has a timer (TPM1) to itself there,
//  so the servo's 50Hz leaves the other PWM pins' period alone
#if defined(FRDM_MCXC444)
#define SERVO_PIN D3
#else
#define SERVO_PIN PWM0
#endif

FS90R servo(SERVO_PIN);

void setup() {
}

void loop() {
  servo.speed(1.0);   //  CCW, full speed
  delay(2000);
  servo.stop();
  delay(500);

  servo.speed(-1.0);  //  CW, full speed
  delay(2000);
  servo.stop();
  delay(500);

  for (int i = -20; i <= 20; i++) {   //  CW full speed -> stop -> CCW full speed
    servo.speed(i / 20.00);
    delay(100);
  }

  servo.stop();
  delay(500);
}
