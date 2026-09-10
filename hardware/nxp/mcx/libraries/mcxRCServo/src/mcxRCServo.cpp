#include  "mcxRCServo.h"

mcxRCServo::mcxRCServo( int pwm_pin, double high_min_ms, double high_max_ms, double frequency_hz, uint8_t pwm_resolution )
	:	pin( pwm_pin ),
		pwm_frequency_hz( frequency_hz ),
		pwm_period_ms( (1.00 / frequency_hz) * 1000.00 ),
		pwm_high_min( high_min_ms ),
		pwm_high_max( high_max_ms ),
		resolution( pwm_resolution ),
		pwm_duty_min( (high_min_ms / pwm_period_ms) * (double)((1L << pwm_resolution) - 1) ),
		pwm_duty_max( (high_max_ms / pwm_period_ms) * (double)((1L << pwm_resolution) - 1) ),
		pwm_duty_range( pwm_duty_max - pwm_duty_min )
{
	analogWriteFrequency( pin, pwm_frequency_hz );
	analogWriteResolution( resolution );
}

mcxRCServo::~mcxRCServo() {}

void mcxRCServo::position( double p )
{
	p	= constrain( p, user_range_min, user_range_max );

	double pos	= (p - user_range_min) / user_range;

	analogWrite( pin, lround( pos * pwm_duty_range + pwm_duty_min ) );
}

int mcxRCServo::range( double left, double right )
{
	if ( left == right )
		return RANGE_SETTING;

	user_range_min	= left <= right ? left : right;
	user_range_max	= right > left ? right : left;

	user_range	= user_range_max - user_range_min;

	return NO_ERROR;
}
