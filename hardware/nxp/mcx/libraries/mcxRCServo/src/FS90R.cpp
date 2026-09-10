#include  "FS90R.h"

FS90R::FS90R( int pwm_pin ) : RotationServo( pwm_pin, pulse_high_min_ms, pulse_high_max_ms, frequency_hz ) {}
