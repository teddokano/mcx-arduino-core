/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef Servo_h
#define Servo_h

#include <Arduino.h>

/** @name The official Servo library's constants
 *  Same names and values, so sketches written for it build unchanged.
 */
///@{
#define MIN_PULSE_WIDTH		544		///< default pulse width for 0 degrees [us]
#define MAX_PULSE_WIDTH		2400	///< default pulse width for 180 degrees [us]
#define DEFAULT_PULSE_WIDTH	1500	///< pulse width before the first write() [us]
#define REFRESH_INTERVAL	20000	///< pulse period [us] (50Hz)
#define SERVOS_PER_TIMER	12		///< as in the official library; nothing here depends on it
#define INVALID_SERVO		255		///< what attach() returns for an object beyond MAX_SERVOS
///@}

/** How many Servo objects can be made: one for each pin with PWM */
#if defined( FRDM_MCXA156 )
#define MAX_SERVOS			10
#elif defined( FRDM_MCXA153 ) || defined( FRDM_MCXN947 ) || defined( FRDM_MCXN236 )
#define MAX_SERVOS			6
#elif defined( FRDM_MCXC444 )
#define MAX_SERVOS			5	// D3, D5, D6, D9, D10 -- but D5/D6/D9 one at a time (see below)
#else
#error "Servo: no MAX_SERVOS for this board yet"
#endif

/** Drives a hobby RC servo, with the official Servo library's interface:
 *  attach()/detach(), write() in degrees or microseconds,
 *  writeMicroseconds(), read() and readMicroseconds().
 *
 *  The pulses come from the pin's PWM hardware (FlexPWM), at 50Hz, so their
 *  width doesn't jitter. That limits the servo to a pin with PWM:
 *  PWM0-PWM5 on every board, and also D3, D5, D6 and D9 on FRDM-MCXA156 and
 *  FRDM-MCXN236 (on FRDM-MCXN236 these are four of PWM0-PWM5), and on
 *  FRDM-MCXC444 D3, D5, D6, D9 and D10, as on an UNO. On any other
 *  pin, attach() stops the sketch with panic(), naming the pins to use.
 *  The official library on AVR uses any pin, which is why its examples use
 *  D9: on FRDM-MCXA153 and FRDM-MCXN947, change that to a PWM pin.
 *
 *  The two pins of a FlexPWM submodule share their period (see the board's
 *  PIN_MAPPING_*.md): attaching a servo sets the other pin's PWM to 50Hz
 *  too, and analogWriteFrequency() on the other pin changes the servo's.
 *  Another servo, or analogWrite() at 50Hz, on the other pin is fine.
 *  On FRDM-MCXC444 the period is shared by a whole TPM: D5, D6, D9 and D10
 *  (TPM0), and D3 (TPM1). D5, D6 and D9 share one TPM0 channel as well, so
 *  only one servo among them runs at a time: the one written last.
 *
 *  Unlike the bundled mcxRCServo library, this leaves analogWrite()'s
 *  resolution alone.
 */
class Servo
{
public:
	Servo();

	/** Start sending pulses on a pin, for 0-180 degrees as 544-2400us.
	 *  A value written before is sent from the start; otherwise 1500us.
	 * @param pin a pin with PWM (see above); any other pin stops the sketch with panic()
	 * @return this servo's index, or INVALID_SERVO if this object is beyond MAX_SERVOS
	 */
	uint8_t	attach( int pin );
	/** The same, with the pulse widths for 0 and 180 degrees.
	 * @param pin a pin with PWM
	 * @param min pulse width for 0 degrees [us]
	 * @param max pulse width for 180 degrees [us]
	 * @return this servo's index, or INVALID_SERVO if this object is beyond MAX_SERVOS
	 */
	uint8_t	attach( int pin, int min, int max );
	/** Stop the pulses; the pin stays LOW. attach() starts them again. */
	void	detach();

	/** Set the position.
	 * @param value an angle in degrees if below 544 (clamped to 0-180), otherwise a pulse width in microseconds
	 */
	void	write( int value );
	/** Set the pulse width, clamped to attach()'s min and max.
	 * @param value pulse width [us]
	 */
	void	writeMicroseconds( int value );
	/** @return the current position as an angle, 0-180 degrees */
	int		read();
	/** @return the current pulse width [us], or 0 if this object is beyond MAX_SERVOS */
	int		readMicroseconds();
	/** @return true between attach() and detach() */
	bool	attached();

private:
	void	send();

	uint8_t	index;
	int		pin			= -1;
	PwmOut	*pwm		= nullptr;
	int		min_us		= MIN_PULSE_WIDTH;
	int		max_us		= MAX_PULSE_WIDTH;
	int		pulse_us	= DEFAULT_PULSE_WIDTH;
};

#endif // Servo_h
