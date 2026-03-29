/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

/** GradationControl operation library for Arduino
 *
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef ARDUINO_GRADATION_CONTROL_NXP_ARD_H
#define ARDUINO_GRADATION_CONTROL_NXP_ARD_H

#include	"led/LEDDriver.h"

/** GradationControl class
 *	
 *  @class GradationControl
 *
 *	GradationControl class is an API to use PCA9955B and PCA9957 gradation control feature. 
 *	Instances are made for each gradation control groups
 */
class GradationControl
{
public:
	
	/** Commands for "control()" method */
	enum control : int
	{
		GRADATION_STOP,
		DUMMY,
		ONE_SHOT,
		CONTINUOUS,
	};
	
	/** Cycle period for the gradation control */
	float	cycle;

	/** Create a GradationControl instance for a group
	 *
	 * @param devp pointer to PCA9955B or PCA9957 device instance
	 * @param num_group int value to specify group number
	 * @param bitmap option. specifying channels assign into this group (bit 0 = channel 0). Assigning can be done later by "add_channel()" method
	 */
	GradationControl( PCA995x* devp, int num_group, uint32_t bitmap = 0x0000 );

	/** Destructor */
	virtual ~GradationControl();

	/** Gradation parameter settings
	 *
	 * @param max_iref Peak current value. 0.0 for 0%, 1.0 for 100%
	 * @param time ramp-time in second
	 * @param up option: ramp-up enable. default: true
	 * @param down option: ramp-down enable. default: true
	 * @param on option: hold-ON duration in second. default: 0.0
	 * @param off option: hold-OFF duration in second. default: 0.0
	 * @param hold_on option: hold-on enable. default: true
	 * @param hold_off option: hold-off enable. default: true
	 * @return Actual cycle period after calculation
	 */
	float set_gradation( float max_iref, float time, bool up = true, bool down = true, float on = 0.0, float off = 0.0, bool hold_on = true, bool hold_off = true );

	/** Channel assign into group
	 *
	 * @param bitmap Set 1 for the channel assigning to the group (bit 0 = channel 0)
	 */
	void gradation_group_assign( uint32_t bitmap );

	/** Adding channel
	 *
	 * @param ch Channel to be added to the group
	 */
	void add_channel( uint8_t ch );
	
	/** Enabling channels for gradation control
	 *
	 * @param bitmap Set 1 for the channel assigning to the group (bit 0 = channel 0)
	 */	
	void gradation_channel_enable( uint32_t bitmap );
	
	/** Start/Stop gradation control
	 *
	 * @param ctrl Can be STOP, ONE_SHOT, CONTINUOUS
	 */
	void control( int ctrl );

	/** Start gradation control
	 *
	 * @param continuous option. default: true
	 */
	void start( bool continuous = true );

	/** Stop gradation control */
	void stop( void );
	
private:
	PCA995x* 			devp;
	const int			group;
	static const float	HOLDTIME[];
};

#endif //	ARDUINO_GRADATION_CONTROL_NXP_ARD_H
