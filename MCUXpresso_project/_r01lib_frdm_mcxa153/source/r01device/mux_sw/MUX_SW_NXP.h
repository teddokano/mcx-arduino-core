/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef ARDUINO_MUX_SW_H
#define ARDUINO_MUX_SW_H

#include	"r01lib.h"
#include	"I2C_device.h"
#include	<stdint.h>

/** PCA9846 class
 *	
 *  @class PCA9846
 *
 *	PCA9846 class is a sample code for the PCA9846PW-ARD
 *	It demonstrates the switch operation with EEPROM on the shield board 
 */

class PCA9846 : public I2C_device
{
public:
	enum channel {
		CH0	= 0x01,
		CH1	= 0x02,
		CH2	= 0x04,
		CH3	= 0x08,
	};
	
	/** Number of channels */
	constexpr static int	N_CH	= 4;
	
	/** Create a PCA9846 instance with specified address
	 *
	 * @param wire TwoWire instance
	 * @param i2c_address I2C-bus address (default: (0xE2>>1))
	 */
	PCA9846( I2C& wire, uint8_t i2c_address = (0xE2 >> 1) );
	virtual ~PCA9846();
	
	/** Begin the device operation
	 *
	 *	This method turns-on the device
	 */
	void begin( void );

	/** Channel select
	 *
	 * @param flags bitmap flags for enabling channels
	 */
	void select( uint8_t flags );	

	/** Channel select
	 *
	 * @return flags bitmap flags for enabling channels
	 */
	uint8_t select( void );	
};

#endif //	ARDUINO_MUX_SW_H
