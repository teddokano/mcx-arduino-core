/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef ARDUINO_TEMP_SENSOR_H
#define ARDUINO_TEMP_SENSOR_H

#include "I2C_device.h"
#include <stdint.h>

/** TempSensor class
 *	
 *  @class TempSensor
 *
 *	TempSensor class is a base class for all temperature sensors
 *	All actual device class will be inherited from this class
 */

class TempSensor : public I2C_device
{
public:
	using raw_t		= int16_t;
	using celsius_t	= float;
	
	enum mode {
		COMPARATOR,	/**< Comparator mode	*/
		INTERRUPT,	/**< Interrupt mode	*/
	};

	/*
	 *	Methods to define class fundamental features, overridden by sub-classes
	 */
	TempSensor( I2C& interface, uint8_t i2c_address );
	virtual ~TempSensor();
	
	/** Read temperature sensor
	 * 
	 *	Performs temperature sensor read. 
	 *	
	 *	This method need to be called with return type as 
	 *	    celsius_t value = read<TempSensor::celsius_t>();
	 *	    raw_t     value = read<TempSensor::raw_t>();
	 *	
	 * @return temperature sensor readout value
	 */
	template<class T>
	T read( void );
	
	/** Get temperature value in degree Celsius [°C] 
	 *
	 * @return temperature value in degree Celsius [°C] 
	 */
	virtual float temp( void );

	/** A short hand for reading pins
	 */
	operator	float();
	
private:	
	virtual int16_t	read_Temp_register( void ) = 0;
};


/** LM75B class
 *	
 *  @class LM75B

 *  About LM75B:
 *		https://www.nxp.com/products/sensors/i3c-ic-digital-temp-sensors/digital-temperature-sensor-and-thermal-watchdog:LM75B?_gl=1*kd25ib*_ga*NTA5NDE1NDA0LjE2NzgzNDYyNzA.*_ga_WM5LE0KMSH*MTY4MjEyMjc1OC42Mi4wLjE2ODIxMjI3NTguMC4wLjA.
 */

class LM75B : public TempSensor
{
public:
	/** Name of the LM75B registers */
	enum reg_num {
		Temp,	/**< Temp register	*/
		Conf,	/**< Conf register	*/
		Thyst,	/**< Thyst register	*/
		Tos,	/**< Tos registe	*/
	};

	/** Create a LM75B instance connected to specified I2C pins with specified address
	 *
	 * @param i2c_address I2C-bus address (default: (0x90>>1))
	 */
	LM75B( I2C& interface, uint8_t i2c_address = (0x90 >> 1) );

	/** Destructor of LM75B
	 */
	virtual ~LM75B();
	
	/** Set Overtemperature shutdown threshold (Tos) and hysteresis (Thyst) in degree Celsius [°C] 
	 *
	 *	This method takes 2 values and higher value will set as the threshold (Tos) and 
	 *	another will be the hysteresis (Thyst)
	 *
	 * @param v0 a value in degree Celsius
	 * @param v1 a value in degree Celsius
	 */	
	virtual void thresholds( float v0, float v1 );

	/** Set OS operation mode 
	 *
	 * @param flag use LM75B::COMPARATOR or LM75B::INTERRUPT values
	 */	
	virtual void os_mode( mode flag );
	
private:
	virtual int16_t read_Temp_register( void ) override;
	
public:
#if DOXYGEN_ONLY
	/** Get temperature value in degree Celsius [°C] 
	 *
	 *	This method simply calls "temp()" method	
	 *
	 * @return temperature value in degree Celsius [°C] 
	 */
	virtual float read( void );

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
	 * @return read data
	 */
	uint8_t	reg_r( uint8_t reg_adr );

	/** Register write, 8 bit
	 *
	 * @param reg register index/address/pointer
	 * @param val data value
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
#endif	// DOXYGEN_ONLY
};


/** PCT2075 class
 *	
 *  @class PCT2075

 *  About PCT2075:
 *    https://www.nxp.com/products/sensors/i3c-ic-digital-temp-sensors/ic-bus-fm-plus-1-degree-c-accuracy-digital-temperature-sensor-and-thermal-watchdog:PCT2075
 */

class PCT2075 : public LM75B
{
public:
	/** Name of the PCT2075 registers */
	enum reg_num {
		Temp,	/**< Temp register	*/
		Conf,	/**< Conf register	*/
		Thyst,	/**< Thyst register	*/
		Tos,	/**< Tos registe	*/
		Tidle,	/**< Tidle register	*/
	};
	
	/** Create a PCT2075 instance connected to specified I2C pins with specified address
	 *
	 * @param i2c_address I2C-bus address (default: (0x90>>1))
	 */
	PCT2075( I2C& interface, uint8_t i2c_address = (0x90 >> 1) );

    /** Destructor of PCT2075
     */
	virtual ~PCT2075();

#if DOXYGEN_ONLY
	/** Get temperature value in degree Celsius [°C] 
	 *
	 * @return temperature value in degree Celsius [°C] 
	 */
	virtual float temp( void );
	
	/** Get temperature value in degree Celsius [°C] 
	 *
	 *	This method simply calls "temp()" method	
	 *
	 * @return temperature value in degree Celsius [°C] 
	 */
	virtual float read( void );

	/** Set Overtemperature shutdown threshold (Tos) and hysteresis (Thyst) in degree Celsius [°C] 
	 *
	 *	This method takes 2 values and higher value will set as the threshold (Tos) and 
	 *	another will be the hysteresis (Thyst)
	 *
	 * @param v0 a value in degree Celsius
	 * @param v1 a value in degree Celsius
	 */	
	virtual void thresholds( float v0, float v1 );

	/** Set OS operation mode 
	 *
	 * @param flag use PCT2075::COMPARATOR or PCT2075::INTERRUPT values
	 */	
	virtual void os_mode( mode flag );	

	/** Register write, 8 bit
	 *
	 * @param reg register index/address/pointer
	 * @param val data value
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
#endif	// DOXYGEN_ONLY
};


/** P3T1755 class
 *	
 *  @class P3T1755

 *  About P3T1085:
 *		https://www.nxp.com/products/sensors/i3c-ic-digital-temp-sensors/i3c-ic-bus-0-5-c-accurate-digital-temperature-sensor:P3T1755DP
 */

class P3T1755 : public LM75B
{
public:
	/** Name of the P3T1755 registers */
	enum reg_num {
		Temp,	/**< Temp register	*/
		Conf,	/**< Conf register	*/
		T_LOW,	/**< Thyst register	*/
		T_HIGH,	/**< Tos registe	*/
	};
	
	/** Create a P3T1755 instance connected to specified I2C pins with specified address
	 *
	 * @param i2c_address I2C-bus address (default: (0x98>>1))
	 */
	P3T1755( I2C& interface, uint8_t i2c_address = (0x98 >> 1) );

	/** Destructor of P3T1755
	 */
	virtual ~P3T1755();

	/** Set Overtemperature shutdown threshold (Tos) and hysteresis (Thyst) in degree Celsius [°C] 
	 *
	 *	This method takes 2 values and higher value will set as the threshold (Tos) and 
	 *	another will be the hysteresis (Thyst)
	 *
	 * @param v0 a value in degree Celsius
	 * @param v1 a value in degree Celsius
	 */	
	virtual void thresholds( float v0, float v1 ) override;

#if DOXYGEN_ONLY
	/** Get temperature value in degree Celsius [°C] 
	 *
	 * @return temperature value in degree Celsius [°C] 
	 */
	virtual float temp( void );
	
	/** Get temperature value in degree Celsius [°C] 
	 *
	 *	This method simply calls "temp()" method	
	 *
	 * @return temperature value in degree Celsius [°C] 
	 */
	virtual float read( void );

	/** Set OS operation mode 
	 *
	 * @param flag use P3T1755::COMPARATOR or P3T1755::INTERRUPT values
	 */	
	virtual void os_mode( mode flag );	

	/** Register write, 8 bit
	 *
	 * @param reg register index/address/pointer
	 * @param val data value
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
#endif	// DOXYGEN_ONLY
};


/** P3T1085 class
 *	
 *  @class P3T1085

 *  About P3T1085:
 *    https://www.nxp.com/products/sensors/ic-digital-temperature-sensors/i3c-ic-bus-0-5-c-accurate-digital-temperature-sensor:P3T1085UK
 */

class P3T1085 : public P3T1755
{
public:
	/** Create a P3T1085 instance connected to specified I2C pins with specified address
	 *
	 * @param i2c_address I2C-bus address (default: (0x90>>1))
	 */
	P3T1085( I2C& interface, uint8_t i2c_address = (0x90 >> 1) );

	/** Destructor of P3T1085
	 */
	virtual ~P3T1085();

	/** Set OS operation mode 
	 *
	 * @param flag use P3T1085::COMPARATOR or P3T1085::INTERRUPT values
	 */	
	virtual void os_mode( mode flag ) override;

	/** Clear ALERT (Clear interurpt)
	 * 
	 * @return true if FH flag in Congiguration register is set 
	 */	
	virtual bool clear( void );

#if DOXYGEN_ONLY
	/** Get temperature value in degree Celsius [°C] 
	 *
	 * @return temperature value in degree Celsius [°C] 
	 */
	virtual float temp( void );
	
	/** Get temperature value in degree Celsius [°C] 
	 *
	 *	This method simply calls "temp()" method	
	 *
	 * @return temperature value in degree Celsius [°C] 
	 */
	virtual float read( void );

	/** Set Overtemperature shutdown threshold (Tos) and hysteresis (Thyst) in degree Celsius [°C] 
	 *
	 *	This method takes 2 values and higher value will set as the threshold (Tos) and 
	 *	another will be the hysteresis (Thyst)
	 *
	 * @param v0 a value in degree Celsius
	 * @param v1 a value in degree Celsius
	 */	
	virtual void thresholds( float v0, float v1 );

	/** Register write, 16 bit
	 * 
	 *	This 16 bit access may ot be common but it's useful for sensor devices
	 *
	 * @param reg register index/address/pointer
	 * @param val data value
	 */
	void write_r16( uint8_t reg, uint16_t val );

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
#endif	// DOXYGEN_ONLY
};


/** P3T1035 class
 *	
 *  @class P3T1035

 *  About P3T1035:
 *    https://www.nxp.com/products/sensors/i3c-ic-digital-temp-sensors/i3c-ic-bus-0-5-c-accuracy-digital-temperature-sensor:P3T1035xUK
 */

class P3T1035 : public P3T1755
{
public:
	/** Name of the P3T1755 registers */
	enum reg_num {
		Temp,	/**< Temp register	*/
		Conf,	/**< Conf register	*/
		T_LOW,	/**< Thyst register	*/
		T_HIGH,	/**< Tos registe	*/
	};
	
	/** Create a P3T1035 instance connected to specified I2C pins with specified address
	 *
	 * @param i2c_address I2C-bus address (default: (0xE0>>1))
	 */
	P3T1035( I2C& interface, uint8_t i2c_address = (0xE4 >> 1) );

	/** Destructor of P3T1035
	 */
	virtual ~P3T1035();
	
	/** Set OS operation mode 
	 * 
	 *	This is dummy method since P3T1035 doesn't have the thermostat mode
	 *
	 * @param flag use P3T1755::COMPARATOR or P3T1755::INTERRUPT values
	 */	
	virtual void os_mode( mode flag );	

#if DOXYGEN_ONLY
	/** Get temperature value in degree Celsius [°C] 
	 *
	 * @return temperature value in degree Celsius [°C] 
	 */
	virtual float temp( void );
	
	/** Get temperature value in degree Celsius [°C] 
	 *
	 *	This method simply calls "temp()" method	
	 *
	 * @return temperature value in degree Celsius [°C] 
	 */
	virtual float read( void );

	/** Set Overtemperature shutdown threshold (Tos) and hysteresis (Thyst) in degree Celsius [°C] 
	 *
	 *	This method takes 2 values and higher value will set as the threshold (Tos) and 
	 *	another will be the hysteresis (Thyst)
	 *
	 * @param v0 a value in degree Celsius
	 * @param v1 a value in degree Celsius
	 */	
	virtual void thresholds( float v0, float v1 ) override;

	/** Register write, 8 bit
	 *
	 * @param reg register index/address/pointer
	 * @param val data value
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
#endif	// DOXYGEN_ONLY
};


/** P3T2030 class
 *	
 *  @class P3T2030

 *  About P3T2030:
 *    https://www.nxp.com/products/sensors/i3c-ic-digital-temp-sensors/i3c-ic-bus-2-0-c-accuracy-digital-temperature-sensor:P3T2030xUK
 */

class P3T2030 : public P3T1035
{
public:
	/** Name of the P3T2030 registers */
	enum reg_num {
		Temp,	/**< Temp register	*/
		Conf,	/**< Conf register	*/
		T_LOW,	/**< Thyst register	*/
		T_HIGH,	/**< Tos registe	*/
	};
	
	/** Create a P3T2030 instance connected to specified I2C pins with specified address
	 *
	 * @param i2c_address I2C-bus address (default: (0xE0>>1))
	 */
	P3T2030( I2C& interface, uint8_t i2c_address = (0xE0 >> 1) );

	/** Destructor of P3T1035
	 */
	virtual ~P3T2030();
	
#if DOXYGEN_ONLY
	/** Get temperature value in degree Celsius [°C] 
	 *
	 * @return temperature value in degree Celsius [°C] 
	 */
	virtual float temp( void );
	
	/** Get temperature value in degree Celsius [°C] 
	 *
	 *	This method simply calls "temp()" method	
	 *
	 * @return temperature value in degree Celsius [°C] 
	 */
	virtual float read( void );

	/** Set Overtemperature shutdown threshold (Tos) and hysteresis (Thyst) in degree Celsius [°C] 
	 *
	 *	This method takes 2 values and higher value will set as the threshold (Tos) and 
	 *	another will be the hysteresis (Thyst)
	 *
	 * @param v0 a value in degree Celsius
	 * @param v1 a value in degree Celsius
	 */	
	virtual void thresholds( float v0, float v1 ) override;

	/** Set OS operation mode 
	 * 
	 *	This is dummy method since P3T2030 doesn't have the thermostat mode
	 *
	 * @param flag use P3T2030::COMPARATOR or P3T1755::INTERRUPT values
	 */	
	virtual void os_mode( mode flag );	

	/** Register write, 8 bit
	 *
	 * @param reg register index/address/pointer
	 * @param val data value
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
#endif	// DOXYGEN_ONLY
};

#endif //	ARDUINO_TEMP_SENSOR_H
