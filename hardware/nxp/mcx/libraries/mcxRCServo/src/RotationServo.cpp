#include  "RotationServo.h"

RotationServo::RotationServo( int pwm_pin, double high_min_ms, double high_max_ms, double frequency_hz, uint8_t pwm_resolution )
	:	mcxRCServo( pwm_pin, high_min_ms, high_max_ms, frequency_hz, pwm_resolution )
{
	range( -1.00, 1.00 );
}

void RotationServo::speed( double s )
{
	position( s );
}

void RotationServo::stop()
{
	position( (user_range_min + user_range_max) / 2.00 );
}
