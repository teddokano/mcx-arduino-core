#include  "SG90.h"

SG90::SG90( int pwm_pin ) : PositionServo( pwm_pin, pulse_high_min_ms, pulse_high_max_ms, frequency_hz ) {}
