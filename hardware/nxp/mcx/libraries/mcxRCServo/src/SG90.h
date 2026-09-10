#ifndef SG90_H
#define SG90_H

#include  "PositionServo.h"

/** Tower Pro SG90: a positional rotation type RC servo.
 *
 *  Just gives its own device parameters to PositionServo.
 *  Use position() to set the shaft angle within the range() setting.
 *
 *  @class SG90
 */
class SG90 : public PositionServo
{
public:
	SG90( int pwm_pin );

private:
	//  these parameters are from https://akizukidenshi.com/goodsaffix/SG90_a.pdf
	static constexpr double	frequency_hz		= 50.00;
	static constexpr double	pulse_high_min_ms	=  0.50;
	static constexpr double	pulse_high_max_ms	=  2.40;
};

#endif // SG90_H
