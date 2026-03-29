/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

/** GPIO SPI device abstracting class
 *
 *	This class provides common methods for all I2C devices
 *
 *  @class   I2C_device
 *  @author  Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef ARDUINO_GPIO_SPI_H
#define ARDUINO_GPIO_SPI_H

#include <stdint.h>
#include "r01lib.h"
#include "I2C_device.h"

class GPIO_SPI : public Serial_device
{
public:
	/** Create a I2C_device instance with specified address
	 *
	 * @param interface SPI instance
	 * @param device_address device address
	 */
	GPIO_SPI( SPI& interface, uint8_t device_address );

    /** Destructor of I2C_device
     */
    virtual ~GPIO_SPI();
	
	/** Send and receive data
	 * 
	 * @param write_data pointer to write data buffer
	 * @param read_data pointer to read data buffer
	 * @param size data size
	 * @return transferred data size or error code in negative number: -1=data too long, -2=NACK for address, -3=NACK for data, -4=Other error, -5=Time out
	 */
	int txrx( const uint8_t *write_data, uint8_t *read_data, uint16_t size );

	/** Multiple register write
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	int reg_w( uint8_t reg_adr, const uint8_t *data, uint16_t size );

	/** Single register write
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	int reg_w( uint8_t reg_adr, uint8_t data );

	/** Multiple register read
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	int reg_r( uint8_t reg_adr, uint8_t *data, uint16_t size );

	/** Single register read
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return read data size
	 */
	uint8_t	reg_r( uint8_t reg_adr );

	/** Register write, 8 bit
	 *
	 * @param reg register index/address/pointer
	 * @return data value
	 */
	void write_r8( uint8_t reg, uint8_t val );

	/** Register write, 16 bit
	 * 
	 *	This 16 bit access may ot be common but it's useful for sensor devices
	 *
	 * @param reg register index/address/pointer
	 * @param val data value
	 */
	void write_r16( uint8_t reg, uint16_t val );

	/** Register read, 8 bit
	 *
	 * @param reg register index/address/pointer
	 * @return data value
	 */
	uint8_t read_r8( uint8_t reg );

	/** Register read, 16 bit
	 *	
	 *	This 16 bit access may ot be common but it's useful for sensor devices
	 *
	 * @param reg register index/address/pointer
	 * @return data value
	 */
	uint16_t read_r16( uint8_t reg );

	/** Register overwriting with bit-mask
	 *	
	 *	Register can be updated by bit level
	 *
	 * @param reg register index/address/pointer
	 * @param mask bit-mask to protect overwriting
	 * @param value value to overwrite
	 */
	void bit_op8(  uint8_t reg,  uint8_t mask,  uint8_t value );
	void bit_op16( uint8_t reg, uint16_t mask, uint16_t value );

private:
	SPI&						spi;
	uint8_t						dev_addr;
	constexpr static uint8_t	auto_increment	= 0x80;	
};

#endif //	ARDUINO_GPIO_SPI_H
