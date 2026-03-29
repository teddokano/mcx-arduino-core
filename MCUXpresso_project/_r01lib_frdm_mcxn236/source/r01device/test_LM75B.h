/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef ARDUINO_TEMP_SENSOR_H
#define ARDUINO_TEMP_SENSOR_H

#include <stdint.h>

#include "I2C_device.h"

/** test_LM75B class
 *	
 *  @class test_LM75B
 *
 *	test class for I2C_device

 *  About LM75B:
 *    https://www.nxp.jp/products/sensors/ic-digital-temperature-sensors/digital-temperature-sensor-and-thermal-watchdog:LM75B
 */

class test_LM75B : public I2C_device
{
public:
	enum mode {
		COMPARATOR,	/**< Comparator mode	*/
		INTERRUPT,	/**< Interrupt mode	*/
	};

	/** Name of the PCT2075 registers */
	enum reg_num {
		Temp,	/**< Temp register	*/
		Conf,	/**< Conf register	*/
		Thyst,	/**< Thyst register	*/
		Tos,	/**< Tos registe	*/
	};

	/** Create a PCT2075 instance connected to specified I2C pins with specified address
	 *
	 * @param wire TwoWire instance
	 * @param i2c_address I2C-bus address (default: (0x90>>1))
	 */
	test_LM75B( I2C& interface, const uint8_t i2c_address = (0x90 >> 1) );

	/** Destructor of PCT2075
	 */
	~test_LM75B();
	
	/** Get temperature value in degree Celsius [°C] 
	 *
	 * @return temperature value in degree Celsius [°C] 
	 */
	float read( void );
	
	/** Set Overtemperature shutdown threshold (Tos) and hysteresis (Thyst) in degree Celsius [°C] 
	 *
	 *	This method takes 2 values and higher value will set as the threshold (Tos) and 
	 *. another will be the hysteresis (Thyst)
	 *
	 * @param v0 a value in degree Celsius
	 * @param v1 a value in degree Celsius
	 */	
	void thresholds( const float v0, const float v1 );

	/** Set OS operation mode 
	 *
	 * @param flag use PCT2075::COMPARATOR or PCT2075::INTERRUPT values
	 */	
	void os_mode( const mode flag );
};

#endif //	ARDUINO_TEMP_SENSOR_H
