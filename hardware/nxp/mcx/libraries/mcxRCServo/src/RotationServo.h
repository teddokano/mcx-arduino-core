#ifndef ROTATIONSERVO_H
#define ROTATIONSERVO_H

#include  "mcxRCServo.h"

/** Continuous rotation type RC servo, the counterpart of PositionServo.
 *
 *  Same pulse interface as a positional rotation servo but the pulse width
 *  sets the rotation speed instead of the shaft angle: the shortest pulse is
 *  full speed in one direction, the longest is full speed in the other and
 *  the middle stops the shaft.
 *
 *  The user value range defaults to -1.0 .. +1.0, so stop() outputs the
 *  center pulse. range() can be used to take any other unit (percent, RPM,
 *  ...) as long as it is symmetric around the stop point.
 *
 *  Concrete servo products derive from this class and supply their own pulse
 *  timing. See FS90R.
 *
 *  @class RotationServo
 */
class RotationServo : public mcxRCServo
{
public:
	/** @param pwm_pin        pin the servo signal is output from
	 *  @param high_min_ms    pulse high period for full speed one way [ms]
	 *  @param high_max_ms    pulse high period for full speed the other way [ms]
	 *  @param frequency_hz   PWM frequency [Hz]
	 *  @param pwm_resolution analogWrite() resolution [bits]
	 */
	RotationServo( int pwm_pin, double high_min_ms, double high_max_ms, double frequency_hz = 50.00, uint8_t pwm_resolution = 16 );

	/** Set rotation speed, in the unit given to range(). */
	void	speed( double s );

	/** Output the center pulse: the shaft stops. */
	void	stop();
};

#endif // ROTATIONSERVO_H
