/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef ARDUINO_AD5161_H
#define ARDUINO_AD5161_H

#include	"r01lib.h"
#include	"I2C_device.h"
#include	<stdint.h>


/** AD5161 class
 *	
 *  @class AD5161_I2C
 *
 */

class AD5161_base
{
public:
	AD5161_base();
	virtual ~AD5161_base();
	
	/** Begin the device operation
	 *
	 *	Dummy
	 */
	virtual	void begin( void );
protected:
	uint8_t	_value;
};

class AD5161_I2C : public AD5161_base, public I2C_device
{
public:
	/** Create a AD5161_I2C instance with specified address
	 *
	 * @param interface I2C instance
	 * @param i2c_address I2C-bus address (default: (0xA2>>1))
	 */
	AD5161_I2C( I2C& interface, uint8_t i2c_address = (0x5A >> 1) );
	virtual ~AD5161_I2C();
	
	/** Set value
	 *
	 * @param v for potentiometer position
	 */
	void value( uint8_t v );	

	/** Get value
	 *
	 * @return potentiometer position
	 */
	uint8_t value( void );	

	/** A short hand for setting pins
	 */
	AD5161_I2C&	operator=( uint8_t v );
	AD5161_I2C&	operator=( AD5161_I2C& rhs );

	/** A short hand for reading pins
	 */
	operator	int();
};

class AD5161_SPI : public AD5161_base
{
public:
	/** Create a AD5161_SPI instance with specified address
	 *
	 * @param interface  SPI instance
	 */
	AD5161_SPI( SPI& interface );
	virtual ~AD5161_SPI();

	/** Set/Get value
	 *
	 * @param v for potentiometer position
	 * @return potentiometer position
	 */
	uint8_t value( uint8_t v );	

	/** Get value
	 *
	 * @return potentiometer position
	 */
	uint8_t value( void );	
	
	/** Send/receive data
	 * 
	 * @param data pointer to data buffer
	 * @param size data size
	 */
	void txrx( uint8_t *data, int size );

	/** A short hand for setting pins
	 */
	AD5161_SPI&	operator=( uint8_t v );
	AD5161_SPI&	operator=( AD5161_SPI& rhs );

	/** A short hand for reading pins
	 */
	operator	int();

private:
	SPI&	spi;
};

#endif //	ARDUINO_AD5161_H
