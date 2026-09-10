#include  "PositionServo.h"

PositionServo::PositionServo( int pwm_pin, double high_min_ms, double high_max_ms, double frequency_hz, uint8_t pwm_resolution )
	:	mcxRCServo( pwm_pin, high_min_ms, high_max_ms, frequency_hz, pwm_resolution ) {}
