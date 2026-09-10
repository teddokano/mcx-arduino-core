#ifndef FS90R_H
#define FS90R_H

#include  "RotationServo.h"

/** FEETECH FS90R: a continuous rotation type RC servo.
 *
 *  Just gives its own device parameters to RotationServo.
 *  Use speed() to set the rotation speed within the range() setting and
 *  stop() to stop the shaft.
 *
 *  With the default range, speed(-1.0) turns the shaft CW at full speed,
 *  speed(+1.0) turns it CCW and speed(0.0) stops it. Note the servo has a
 *  90us dead band around the stop pulse: speed values within about +/-0.056
 *  of the center leave the shaft stopped.
 *
 *  @class FS90R
 */
class FS90R : public RotationServo
{
public:
	FS90R( int pwm_pin );

private:
	//  these parameters are from https://akizukidenshi.com/goodsaffix/fs90r_20201214.pdf
	//    pulse width range 700-2300us, stop position 1500(+/-45)us, dead band 90us
	//    CW when 1500-700us, CCW when 1500-2300us
	//  the datasheet gives no frame rate: 50Hz is the usual one for an analog servo
	static constexpr double	frequency_hz		= 50.00;
	static constexpr double	pulse_high_min_ms	=  0.70;
	static constexpr double	pulse_high_max_ms	=  2.30;
};

#endif // FS90R_H
