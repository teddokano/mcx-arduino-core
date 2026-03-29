/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef ARDUINO_I2C_DEVICE_H
#define ARDUINO_I2C_DEVICE_H

#include <stdint.h>
#include "r01lib.h"

/** Serial_device class
 *	
 *  @class Serial_device
 *
 *	An abstraction class for all serial interface devices
 */

class Serial_device
{
public:
	Serial_device();
	virtual ~Serial_device();

	virtual int reg_w( uint8_t reg_adr, const uint8_t *data, uint16_t size )	= 0;
	virtual int reg_w( uint8_t reg_adr, uint8_t data )	= 0;
	virtual int reg_r( uint8_t reg_adr, uint8_t *data, uint16_t size )	= 0;
	virtual uint8_t	reg_r( uint8_t reg_adr )	= 0;

	virtual void write_r8( uint8_t reg, uint8_t val )	= 0;
	virtual uint8_t read_r8( uint8_t reg )	= 0;
	virtual void bit_op8(  uint8_t reg,  uint8_t mask,  uint8_t value )	= 0;

	virtual void write_r16( uint8_t reg, uint16_t val );
	virtual uint16_t read_r16( uint8_t reg );
	virtual void bit_op16( uint8_t reg, uint16_t mask, uint16_t value );
};

/** I2C_device class
 *	
 *  @class I2C_device
 *
 *	A base class for devices which supports I2C
 */

class I2C_device : public Serial_device
{
public:
	/** Create a I2C_device instance with specified address
	 *
	 * @param interface I2C instance
	 * @param i2c_address I2C-bus address
	 * @param repeated_start_enable repeated-START-condition enable between write and read transaction for register reading. Default: true
	 */
	I2C_device( I2C& interface, uint8_t i2c_address, bool repeated_start_enable = true );

    /** Destructor of I2C_device
     */
    virtual ~I2C_device();
	
	/** repeated-START-condition enable
	 *
	 * @param en repeated-START-condition enable between write and read transaction for register reading. Default: true
	 */
	void repeated_start_enable( bool en = true );
	
	/** Send data
	 * 
	 * @param data pointer to data buffer
	 * @param size data size
	 * @param stop option: generating STOP-condition after transaction. Defailt: true
	 * @return transferred data size or error code in negative number: -1=data too long, -2=NACK for address, -3=NACK for data, -4=Other error, -5=Time out
	 */
	int tx( const uint8_t *data, uint16_t size, bool stop = true );

	/** Receive data
	 * 
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	int rx( uint8_t *data, uint16_t size );
	
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

	/** ping
	 *		check device returns ACK
	 *
	 * @return true if ACKs
	 */
	bool ping( void );
	
	/** ping (class method)
	 *
	 * @param target address
	 * @return true, if ACKed
	 */
	static bool ping( uint8_t addr );
	
	/** scan (class method)
	 * 		thsi is a class-method to scan devices
	 * 		
	 * @param target_i2c I2C instance
	 * @param last	the last address to limit scan range
	 */
	static void scan( I2C& target_i2c, uint8_t last = 124 );

	/** address
	 * @return target address in 7bit notation (right justified)
	 */
	uint8_t address( void );

	/**
	 *	FOR I3C only
	 */
	
	/** Target address overwrite
	 *
	 * @param address new address given by Dynamic address assignment
	 */		
	void				address_overwrite( uint8_t address );

	/** Set Commion Command Code
	 *
	 * @param ccc Commion Command Code
	 * @param data data for setting
	 */		
	virtual void ccc_set( CCC ccc, uint8_t data );

	/** Get Commion Command Code
	 *
	 * @param ccc Commion Command Code
	 * @param dp pointer to data array
	 * @param length data array length
	 * @return pointer to data array
	 */		
	virtual uint8_t* ccc_get( CCC ccc, uint8_t *dp, uint8_t length );


protected:
	I2C&		i2c;
	uint8_t		i2c_addr;
private:
	bool		rs_dis;
};

#endif //	ARDUINO_I2C_DEVICE_H
