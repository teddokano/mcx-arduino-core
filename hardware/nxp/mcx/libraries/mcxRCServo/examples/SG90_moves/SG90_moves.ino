/** SG90 moves
 *
 *  Five ways of moving an SG90 positional rotation servo, one after the
 *  other:
 *
 *    1. steps    -- position() jumps and the servo runs there flat out
 *    2. glide    -- small position() steps on a timer: a constant speed
 *    3. eased    -- the same trip with the steps spaced by a cosine, so it
 *                   accelerates away from one end and slows into the other
 *    4. wobble   -- a decaying sine, like a pendulum settling down
 *    5. flicks   -- random angles held for random lengths of time
 *
 *  The servo itself has one speed, so 2 to 5 work by feeding it a stream of
 *  intermediate angles rather than one final angle.
 *
 *  range() picks the unit position() takes -- degrees here. Without a
 *  range() call the servo takes 0.0 to 1.0.
 *
 *  The signal line goes to PWM0. This sketch was verified with the servo
 *  powered from the board, but give it its own 4.8V-6V supply (grounds tied
 *  together) once it has a load to move: stall current is past what the
 *  board can hand out.
 */

#include <Arduino.h>
#include <SG90.h>

SG90 servo(PWM0);

static constexpr double left_end = -90.00;
static constexpr double right_end = 90.00;

static constexpr int step_ms = 20;      //  one position() update per 20ms

/** Walk the servo from one angle to another over about duration_ms.
 *
 *  @param eased  false: constant speed. true: cosine spaced, so the move
 *                starts and ends gently
 */
static void glide(double from, double to, int duration_ms, bool eased = false)
{
    int steps = duration_ms / step_ms;

    for (int i = 0; i <= steps; i++) {
      double t = (double)i / (double)steps;

      if (eased)
        t = (1.00 - cos(t * PI)) / 2.00;

      servo.position(from + (to - from) * t);
      delay(step_ms);
    }
}

void setup() {
  servo.range(left_end, right_end);
}

void loop() {
  //  1. steps: nothing between the angles, so the servo moves at its own speed
  servo.position(0);
  delay(400);
  servo.position(left_end);
  delay(400);
  servo.position(right_end);
  delay(400);
  servo.position(0);
  delay(800);

  //  2. constant speed, out and back
  glide(0, right_end, 1000);
  glide(right_end, left_end, 2000);
  glide(left_end, 0, 1000);
  delay(800);

  //  3. the same trip, eased
  glide(0, right_end, 1000, true);
  glide(right_end, left_end, 2000, true);
  glide(left_end, 0, 1000, true);
  delay(800);

  //  4. wobble: a 1.2Hz swing, its amplitude decaying from 80 degrees
  for (int i = 0; i < 150; i++) {
    double t = (double)(i * step_ms) / 1000.00;   //  elapsed time [s]

    servo.position(80.00 * exp(-t * 0.80) * sin(t * 1.20 * 2.00 * PI));
    delay(step_ms);
  }

  servo.position(0);
  delay(800);

  //  5. flicks
  for (int i = 0; i < 8; i++) {
    servo.position(random(left_end, right_end));
    delay(random(150, 500));
  }

  servo.position(0);
  delay(1200);
}
