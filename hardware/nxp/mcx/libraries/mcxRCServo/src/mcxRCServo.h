#ifndef MCXRCSERVO_H
#define MCXRCSERVO_H

#include  "Arduino.h"

/** Base class for RC servo drivers.
 *
 *  Holds everything that is common to any RC servo: the PWM pin, the pulse
 *  timing of the servo (frequency and the high-period range) and the mapping
 *  from a user defined value range onto that pulse range.
 *
 *  What the mapped value *means* is left to the derived classes: PositionServo
 *  takes it as a shaft angle, RotationServo as a rotation speed. That is why
 *  position() is protected here -- each of them publishes it under the name
 *  that fits its servo type.
 *
 *  A derived class just needs to hand its own device parameters to this
 *  constructor. See PositionServo and RotationServo.
 *
 *  @class mcxRCServo
 */
class mcxRCServo : public Obj
{
public:
	enum {
		NO_ERROR,
		RANGE_SETTING
	};

	/** @param pwm_pin        pin the servo signal is output from
	 *  @param high_min_ms    pulse high period for the "minimum" position [ms]
	 *  @param high_max_ms    pulse high period for the "maximum" position [ms]
	 *  @param frequency_hz   PWM frequency [Hz]
	 *  @param pwm_resolution analogWrite() resolution [bits]
	 */
	mcxRCServo( int pwm_pin, double high_min_ms, double high_max_ms, double frequency_hz = 50.00, uint8_t pwm_resolution = 16 );
	virtual ~mcxRCServo();

	int		range( double left, double right );

protected:
	/** Output the pulse for value p, mapped from the user value range. */
	void	position( double p );

	//  user value range: what position() takes
	double	user_range_min	= 0.00;
	double	user_range_max	= 1.00;
	double	user_range		= user_range_max - user_range_min;

	//  target pin
	const int	pin;

	//  device parameters, given by the derived class
	const double	pwm_frequency_hz;
	const double	pwm_period_ms;
	const double	pwm_high_min;
	const double	pwm_high_max;
	const uint8_t	resolution;

	//  duty values derived from the parameters above
	const double	pwm_duty_min;
	const double	pwm_duty_max;
	const double	pwm_duty_range;
};

#endif // MCXRCSERVO_H
