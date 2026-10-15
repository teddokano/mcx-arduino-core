/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Original implementation of the official Servo library's interface on
 *  FlexPWM; none of its source (LGPL 2.1) is copied.
 */

#include "Servo.h"

static uint8_t	servo_count	= 0;

Servo::Servo()
{
	index	= servo_count < MAX_SERVOS ? servo_count++ : INVALID_SERVO;
}

uint8_t Servo::attach( int pin )
{
	return	attach( pin, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH );
}

uint8_t Servo::attach( int pin, int min, int max )
{
	if ( index == INVALID_SERVO )
		return	INVALID_SERVO;

	PwmOut	*p	= analog_pwm_out( pin );

	//	The official library takes any pin, so sketches don't check what
	//	attach() returns: failing quietly here would leave the servo still
	//	with no hint why. Stop and say so, as analogWriteFrequency() does.
	if ( p == nullptr )
		panic( "Servo: pin has no PWM -- use PWM0-PWM5 (or D3/D5/D6/D9 on FRDM-MCXA156/N236)" );

	if ( pwm && pwm != p )
		detach();	// moving to another pin: stop the old one's pulses

	this->pin	= pin;
	pwm			= p;
	min_us		= min;
	max_us		= max;

	pwm->period_us( REFRESH_INTERVAL );
	writeMicroseconds( pulse_us );	// clamps a width written before to the new range

	return	index;
}

void Servo::detach()
{
	if ( pwm )
		pwm->pulsewidth_us( 0 );

	pwm	= nullptr;
	pin	= -1;
}

void Servo::write( int value )
{
	if ( value < MIN_PULSE_WIDTH )
	{
		value	= constrain( value, 0, 180 );
		value	= map( value, 0, 180, min_us, max_us );
	}

	writeMicroseconds( value );
}

void Servo::writeMicroseconds( int value )
{
	if ( index == INVALID_SERVO )
		return;

	pulse_us	= constrain( value, min_us, max_us );
	send();
}

void Servo::send()
{
	if ( pwm )
		pwm->pulsewidth_us( pulse_us );
}

int Servo::read()
{
	// The +1 makes a width written from an angle read back as that angle:
	// map() rounds down both ways
	return	map( readMicroseconds() + 1, min_us, max_us, 0, 180 );
}

int Servo::readMicroseconds()
{
	return	index == INVALID_SERVO ? 0 : pulse_us;
}

bool Servo::attached()
{
	return	pwm != nullptr;
}
