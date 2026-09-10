#ifndef POSITIONSERVO_H
#define POSITIONSERVO_H

#include  "mcxRCServo.h"

/** Positional rotation type RC servo.
 *
 *  The pulse width sets the shaft angle: the shortest pulse is one end of the
 *  travel, the longest is the other end. This is the "standard" servo type,
 *  the counterpart of RotationServo.
 *
 *  The user value range defaults to 0.0 .. 1.0. Give range() the unit the
 *  application wants to work in (degrees, for instance) and position() takes
 *  its values in that unit from then on.
 *
 *  Concrete servo products derive from this class and supply their own pulse
 *  timing. See SG90.
 *
 *  @class PositionServo
 */
class PositionServo : public mcxRCServo
{
public:
	/** @param pwm_pin        pin the servo signal is output from
	 *  @param high_min_ms    pulse high period for one end of the travel [ms]
	 *  @param high_max_ms    pulse high period for the other end [ms]
	 *  @param frequency_hz   PWM frequency [Hz]
	 *  @param pwm_resolution analogWrite() resolution [bits]
	 */
	PositionServo( int pwm_pin, double high_min_ms, double high_max_ms, double frequency_hz = 50.00, uint8_t pwm_resolution = 16 );

	//  set the shaft angle, in the unit given to range()
	using	mcxRCServo::position;
};

#endif // POSITIONSERVO_H
