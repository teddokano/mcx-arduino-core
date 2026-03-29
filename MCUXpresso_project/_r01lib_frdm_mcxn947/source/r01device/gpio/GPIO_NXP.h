/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef ARDUINO_GPIO_NXP_ARD_H
#define ARDUINO_GPIO_NXP_ARD_H

#include	"r01lib.h"
#include	"I2C_device.h"
#include	"GPIO_SPI.h"
#include	<stdint.h>
#include	<memory>

/** Descriptors for accessing GPIO
 *
 *	'access_words' are used as first argument of write_portN(), read_portN() methods
 *	These are abstracting register address for each device types
 */
enum access_word : uint8_t
{
	IN,
	OUT,
	POLARITY,
	CONFIG,
	DRIVE_STRENGTH,
	LATCH,
	PULL_UD_EN,
	PULL_UD_SEL,
	INT_MASK,
	INT_STATUS,
	OUTPUT_PORT_CONFIG,
	NUM_access_word, 
};


/** GPIO_base class
 *	
 *  @class GPIO_base
 *
 *	This class is a base class for all GPIO devices
 *	All actual device class will be derived from this
 */
class GPIO_base
{
public:
	enum board {
		NONE,
		ARDUINO_SHIELD,
	};
	
	/** Number of IO bits */
	const int	n_bits;

	/** Number of IO ports */
	const int	n_ports;
	
	/** Constractor
	 * 
	 * @param interface I2C instance
	 * @param i2c_address I2C target address
	 * @param nbits	Number of IO bits
	 * @param arp	Pointer to register access reference table
	 * @param ai	Auto-increment flag
	 */
	GPIO_base( I2C& interface, uint8_t i2c_address, int nbits, const uint8_t* arp, uint8_t ai );

	/** Constractor
	 * 
	 * @param interface I2C instance
	 * @param i2c_address I2C target address
	 * @param nbits	Number of IO bits
	 * @param arp	Pointer to register access reference table
	 * @param ai	Auto-increment flag
	 */
	GPIO_base( SPI& interface, uint8_t dev_address, int nbits, const uint8_t* arp, uint8_t ai );

	/** Destractor */
	virtual ~GPIO_base();

	/** Device/board initialization
	 *
	 * This method is needed to initialize Arduino-shield type evaluation boards from NXP. 
	 * This method takes one argument of "GPIO_base::ARDUINO_SHIELD" to set RESET and ADDRESS pins. 
	 * 
	 * If the devoce is used as it self, this method doesn't need to be called. 
	 *	
	 * @param env	This argument can be given as "GPIO_base::NONE" ot "GPIO_base::ARDUINO_SHIELD"
	 */
	virtual void	begin( board env = NONE );
	
	/** Output, single port
	 * 
	 *	Basic GPIO port access function for single port output
	 *
	 * @param port	Port number
	 * @param value	Value to be output
	 * @param mask	Bit mask. Value will not be changed in bit positions '1' in mask
	 */
	void		output( int port, uint8_t value, uint8_t mask = 0 );

	/** Output, all ports
	 * 
	 *	Basic GPIO port access function for all ports output
	 *
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		output( const uint8_t *vp );
	
	/** Input, single port
	 * 
	 *	Basic GPIO port access function for single port input
	 *
	 * @param port	Port number
	 * @return Port read value
	 */
	uint8_t		input( int port );

	/** Input, all ports
	 * 
	 *	Basic GPIO port access function for all ports input
	 *
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return Pointer to vp
	 */
	uint8_t*	input( uint8_t *vp );

	/** Config, single port
	 * 
	 *	Basic GPIO port access function for single port configuration
	 *
	 * @param port	Port number
	 * @param value	Value to be written into configuration register
	 * @param mask	Bit mask. Value will not be changed in bit positions '1' in mask
	 */
	void		config( int port, uint8_t config, uint8_t mask = 0 );

	/** Config, all ports
	 * 
	 *	Basic GPIO port access function for all port configuration
	 *
	 * @param vp	Pointer to array of configuration values. The array should have 'n_ports' length
	 */
	void		config( const uint8_t* vp );

	/** Write all port method
	 * 
	 *	All port register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		write_port( access_word w, const uint8_t* vp );

	/** Write all port method
	 * 
	 *	All port 16 bit register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		write_port16( access_word w, const uint16_t* vp );

	/** Read all port method
	 * 
	 *	All port register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return	Pointer to vp
	 */
	uint8_t*	read_port( access_word w, uint8_t* vp );

	/** Read all port method
	 * 
	 *	All port 16 bit register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return	Pointer to vp
	 */
	uint16_t*	read_port16( access_word w, uint16_t* vp );

	/** Write single port method
	 * 
	 *	Single port register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param value		Value to be written into a register
	 * @param port_num	Option, to specify port number
	 */
	void		write_port( access_word w, uint8_t value, int port_num = 0 );

	/** Write single port method
	 * 
	 *	Single port 16 bit register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param value		Value to be written into a register
	 * @param port_num	Option, to specify port number
	 */
	void		write_port16( access_word w, uint16_t value, int port_num = 0 );

	/** Read single port method
	 * 
	 *	Single port register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param port_num	Option, to specify port number
	 * @return Register read value
	 */
	uint8_t		read_port( access_word w, int port_num = 0 );

	/** Read single port method
	 * 
	 *	Single port 16 bit register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param port_num	Option, to specify port number
	 * @return Register read value
	 */
	uint16_t	read_port16( access_word w, int port_num = 0 );

	static void	print_bin( uint8_t v );
	
	void init( void );

private:
	const uint8_t*	arp;
	const uint8_t	auto_increment;
	bool			endian;
	std::unique_ptr<Serial_device>	intfp;
	
	static constexpr int RESET_PIN	= D8;
	static constexpr int ADDR_PIN	= D9;
};

/** PCA9554 class
 *	
 *  @class PCA9554
 */
class PCA9554 : public GPIO_base
{
public:
	/** Name of the PCA9554 registers */
	enum reg_num {
		Input_Port,
		Output_Port,
		Polarity_Inversion,
		Configuration,
	};
	
	/** Constractor
	 * 
	 * @param interface I2C instance
	 * @param i2c_address I2C target address
	 */
	PCA9554( I2C& interface, uint8_t i2c_address = (0x40 >> 1) + 0 );

	/** Destractor */
	virtual ~PCA9554();

	static constexpr uint8_t	access_ref[ NUM_access_word ]	= {
		Input_Port,			//	IN,
		Output_Port,		//	OUT
		Polarity_Inversion,	//	POLARITY
		Configuration,		//	CONFIG
		0xFF,	//	DRIVE_STRENGTH		** CANNOT BE USED **
		0xFF,	//	LATCHLATCH			** CANNOT BE USED **
		0xFF,	//	PULL_UD_EN			** CANNOT BE USED **
		0xFF,	//	PULL_UD_SEL			** CANNOT BE USED **
		0xFF,	//	INT_MASK			** CANNOT BE USED **
		0xFF,	//	INT_STATUS			** CANNOT BE USED **
		0xFF,	//	OUTPUT_PORT_CONFIG	** CANNOT BE USED **
	};
	
#if DOXYGEN_ONLY
	/** Constants for begin() method */
	enum board {
		NONE,
		ARDUINO_SHIELD,
	};

	/** Number of IO bits */
	const int	n_bits;

	/** Number of IO ports */
	const int	n_ports;

	/** Device/board initialization
	 *
	 * This method is needed to initialize Arduino-shield type evaluation boards from NXP. 
	 * This method takes one argument of "PCA9554::ARDUINO_SHIELD" to set RESET and ADDRESS pins. 
	 * 
	 * If the devoce is used as it self, this method doesn't need to be called. 
	 *	
	 * @param env	This argument can be given as "PCA9554::NONE" ot "PCA9554::ARDUINO_SHIELD"
	 */
	virtual void	begin( board env = NONE );
	
	/** Output, single port
	 * 
	 *	Basic GPIO port access function for single port output
	 *
	 * @param port	Port number
	 * @param value	Value to be output
	 * @param mask	Bit mask. Value will not be changed in bit positions '1' in mask
	 */
	void		output( int port, uint8_t value, uint8_t mask = 0 );

	/** Output, all ports
	 * 
	 *	Basic GPIO port access function for all ports output
	 *
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		output( const uint8_t *vp );
	
	/** Input, single port
	 * 
	 *	Basic GPIO port access function for single port input
	 *
	 * @param port	Port number
	 * @return Port read value
	 */
	uint8_t		input( int port );

	/** Input, all ports
	 * 
	 *	Basic GPIO port access function for all ports input
	 *
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return Pointer to vp
	 */
	uint8_t*	input( uint8_t *vp );

	/** Config, single port
	 * 
	 *	Basic GPIO port access function for single port configuration
	 *
	 * @param port	Port number
	 * @param value	Value to be written into configuration register
	 * @param mask	Bit mask. Value will not be changed in bit positions '1' in mask
	 */
	void		config( int port, uint8_t config, uint8_t mask = 0 );

	/** Config, all ports
	 * 
	 *	Basic GPIO port access function for all port configuration
	 *
	 * @param vp	Pointer to array of configuration values. The array should have 'n_ports' length
	 */
	void		config( const uint8_t* vp );

	/** Write all port method
	 * 
	 *	All port register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		write_port( access_word w, const uint8_t* vp );

	/** Write all port method
	 * 
	 *	All port 16 bit register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		write_port16( access_word w, const uint16_t* vp );

	/** Read all port method
	 * 
	 *	All port register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return	Pointer to vp
	 */
	uint8_t*	read_port( access_word w, uint8_t* vp );

	/** Read all port method
	 * 
	 *	All port 16 bit register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return	Pointer to vp
	 */
	uint16_t*	read_port16( access_word w, uint16_t* vp );

	/** Write single port method
	 * 
	 *	Single port register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param value		Value to be written into a register
	 * @param port_num	Option, to specify port number
	 */
	void		write_port( access_word w, uint8_t value, int port_num = 0 );

	/** Write single port method
	 * 
	 *	Single port 16 bit register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param value		Value to be written into a register
	 * @param port_num	Option, to specify port number
	 */
	void		write_port16( access_word w, uint16_t value, int port_num = 0 );

	/** Read single port method
	 * 
	 *	Single port register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param port_num	Option, to specify port number
	 * @return Register read value
	 */
	uint8_t		read_port( access_word w, int port_num = 0 );

	/** Read single port method
	 * 
	 *	Single port 16 bit register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param port_num	Option, to specify port number
	 * @return Register read value
	 */
	uint16_t	read_port16( access_word w, int port_num = 0 );

	/** Ping the device
	 *
	 * @return true when ACK 
	 */
	bool ping( void );
		
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
#endif	//	DOXYGEN_ONLY

};

/** PCA9555 class
 *	
 *  @class PCA9555
 */
class PCA9555 : public GPIO_base
{
public:
	/** Name of the PCA9555 registers */
	enum reg_num {
		Input_Port_0, Input_Port_1, 
		Output_Port_0, Output_Port_1, 
		Polarity_Inversion_Port_0, Polarity_Inversion_Port_1, 
		Configuration_Port_0, Configuration_Port_1, 
	};
	
	/** Constractor
	 * 
	 * @param interface I2C instance
	 * @param i2c_address I2C target address
	 */
	PCA9555( I2C& interface, uint8_t i2c_address = (0x40 >> 1) + 0 );

	/** Destractor */
	virtual ~PCA9555();

	static constexpr uint8_t	access_ref[ NUM_access_word ]	= {
		Input_Port_0,				//	IN,
		Output_Port_0,				//	OUT
		Polarity_Inversion_Port_0,	//	POLARITY
		Configuration_Port_0,		//	CONFIG
		0xFF,	//	DRIVE_STRENGTH		** CANNOT BE USED **
		0xFF,	//	LATCHLATCH			** CANNOT BE USED **
		0xFF,	//	PULL_UD_EN			** CANNOT BE USED **
		0xFF,	//	PULL_UD_SEL			** CANNOT BE USED **
		0xFF,	//	INT_MASK			** CANNOT BE USED **
		0xFF,	//	INT_STATUS			** CANNOT BE USED **
		0xFF,	//	OUTPUT_PORT_CONFIG	** CANNOT BE USED **
	};

#if DOXYGEN_ONLY
	/** Constants for begin() method */
	enum board {
		NONE,
		ARDUINO_SHIELD,
	};

	/** Number of IO bits */
	const int	n_bits;

	/** Number of IO ports */
	const int	n_ports;

	/** Device/board initialization
	 *
	 * This method is needed to initialize Arduino-shield type evaluation boards from NXP. 
	 * This method takes one argument of "PCA9555::ARDUINO_SHIELD" to set RESET and ADDRESS pins. 
	 * 
	 * If the devoce is used as it self, this method doesn't need to be called. 
	 *	
	 * @param env	This argument can be given as "PCA9555::NONE" ot "PCA9555::ARDUINO_SHIELD"
	 */
	virtual void	begin( board env = NONE );
	
	/** Output, single port
	 * 
	 *	Basic GPIO port access function for single port output
	 *
	 * @param port	Port number
	 * @param value	Value to be output
	 * @param mask	Bit mask. Value will not be changed in bit positions '1' in mask
	 */
	void		output( int port, uint8_t value, uint8_t mask = 0 );

	/** Output, all ports
	 * 
	 *	Basic GPIO port access function for all ports output
	 *
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		output( const uint8_t *vp );
	
	/** Input, single port
	 * 
	 *	Basic GPIO port access function for single port input
	 *
	 * @param port	Port number
	 * @return Port read value
	 */
	uint8_t		input( int port );

	/** Input, all ports
	 * 
	 *	Basic GPIO port access function for all ports input
	 *
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return Pointer to vp
	 */
	uint8_t*	input( uint8_t *vp );

	/** Config, single port
	 * 
	 *	Basic GPIO port access function for single port configuration
	 *
	 * @param port	Port number
	 * @param value	Value to be written into configuration register
	 * @param mask	Bit mask. Value will not be changed in bit positions '1' in mask
	 */
	void		config( int port, uint8_t config, uint8_t mask = 0 );

	/** Config, all ports
	 * 
	 *	Basic GPIO port access function for all port configuration
	 *
	 * @param vp	Pointer to array of configuration values. The array should have 'n_ports' length
	 */
	void		config( const uint8_t* vp );

	/** Write all port method
	 * 
	 *	All port register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		write_port( access_word w, const uint8_t* vp );

	/** Write all port method
	 * 
	 *	All port 16 bit register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		write_port16( access_word w, const uint16_t* vp );

	/** Read all port method
	 * 
	 *	All port register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return	Pointer to vp
	 */
	uint8_t*	read_port( access_word w, uint8_t* vp );

	/** Read all port method
	 * 
	 *	All port 16 bit register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return	Pointer to vp
	 */
	uint16_t*	read_port16( access_word w, uint16_t* vp );

	/** Write single port method
	 * 
	 *	Single port register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param value		Value to be written into a register
	 * @param port_num	Option, to specify port number
	 */
	void		write_port( access_word w, uint8_t value, int port_num = 0 );

	/** Write single port method
	 * 
	 *	Single port 16 bit register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param value		Value to be written into a register
	 * @param port_num	Option, to specify port number
	 */
	void		write_port16( access_word w, uint16_t value, int port_num = 0 );

	/** Read single port method
	 * 
	 *	Single port register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param port_num	Option, to specify port number
	 * @return Register read value
	 */
	uint8_t		read_port( access_word w, int port_num = 0 );

	/** Read single port method
	 * 
	 *	Single port 16 bit register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param port_num	Option, to specify port number
	 * @return Register read value
	 */
	uint16_t	read_port16( access_word w, int port_num = 0 );
	
	/** Ping the device
	 *
	 * @return true when ACK 
	 */
	bool ping( void );
		
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
#endif	//	DOXYGEN_ONLY
};

/** PCAL6xxx_base class
 *	
 *  @class PCAL6xxx_base
 *
 *	Yet another abstraction class for PCAL6xxx devices
 *	This class is just passing parameters to GPIO_base class in this version
 */
class PCAL6xxx_base : public GPIO_base
{
public:
	PCAL6xxx_base( I2C& interface, uint8_t i2c_address, const int nbits, const uint8_t arp[], uint8_t ai );
	virtual ~PCAL6xxx_base();
};


/** PCAL6408A class
 *	
 *  @class PCAL6408A
 */
class PCAL6408A : public PCAL6xxx_base
{
public:
	/** Name of the PCAL6408A registers */
	enum reg_num {
		Input_Port,
		Output_Port,
		Polarity_Inversion,
		Configuration,
		Output_drive_strength_0 = 0x40,
		Output_drive_strength_1,
		Input_latch,
		Pull_up_pull_down_enable,
		Pull_up_pull_down_selection,
		Interrupt_mask,
		Interrupt_status,
		Output_port_configuration,
	};
	
	/** Constractor
	 * 
	 * @param interface I2C instance
	 * @param i2c_address I2C target address
	 */
	PCAL6408A( I2C& interface, uint8_t i2c_address = (0x40 >> 1) + 0 );

	/** Destractor */
	virtual ~PCAL6408A();

	static constexpr uint8_t	access_ref[ NUM_access_word ]	= {
		Input_Port,						//	IN,
		Output_Port,					//	OUT
		Polarity_Inversion,				//	POLARITY
		Configuration,					//	CONFIG
		Output_drive_strength_0,		//	DRIVE_STRENGTH
		Input_latch,					//	LATCHLATCH
		Pull_up_pull_down_enable,		//	PULL_UD_EN
		Pull_up_pull_down_selection,	//	PULL_UD_SEL
		Interrupt_mask,					//	INT_MASK
		Interrupt_status,				//	INT_STATUS
		Output_port_configuration,		//	OUTPUT_PORT_CONFIG
	};

#if DOXYGEN_ONLY
	/** Constants for begin() method */
	enum board {
		NONE,
		ARDUINO_SHIELD,
	};

	/** Number of IO bits */
	const int	n_bits;

	/** Number of IO ports */
	const int	n_ports;

	/** Device/board initialization
	 *
	 * This method is needed to initialize Arduino-shield type evaluation boards from NXP. 
	 * This method takes one argument of "PCAL6408A::ARDUINO_SHIELD" to set RESET and ADDRESS pins. 
	 * 
	 * If the devoce is used as it self, this method doesn't need to be called. 
	 *	
	 * @param env	This argument can be given as "PCAL6408A::NONE" ot "PCAL6408A::ARDUINO_SHIELD"
	 */
	virtual void	begin( board env = NONE );
	
	/** Output, single port
	 * 
	 *	Basic GPIO port access function for single port output
	 *
	 * @param port	Port number
	 * @param value	Value to be output
	 * @param mask	Bit mask. Value will not be changed in bit positions '1' in mask
	 */
	void		output( int port, uint8_t value, uint8_t mask = 0 );

	/** Output, all ports
	 * 
	 *	Basic GPIO port access function for all ports output
	 *
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		output( const uint8_t *vp );
	
	/** Input, single port
	 * 
	 *	Basic GPIO port access function for single port input
	 *
	 * @param port	Port number
	 * @return Port read value
	 */
	uint8_t		input( int port );

	/** Input, all ports
	 * 
	 *	Basic GPIO port access function for all ports input
	 *
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return Pointer to vp
	 */
	uint8_t*	input( uint8_t *vp );

	/** Config, single port
	 * 
	 *	Basic GPIO port access function for single port configuration
	 *
	 * @param port	Port number
	 * @param value	Value to be written into configuration register
	 * @param mask	Bit mask. Value will not be changed in bit positions '1' in mask
	 */
	void		config( int port, uint8_t config, uint8_t mask = 0 );

	/** Config, all ports
	 * 
	 *	Basic GPIO port access function for all port configuration
	 *
	 * @param vp	Pointer to array of configuration values. The array should have 'n_ports' length
	 */
	void		config( const uint8_t* vp );

	/** Write all port method
	 * 
	 *	All port register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		write_port( access_word w, const uint8_t* vp );

	/** Write all port method
	 * 
	 *	All port 16 bit register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		write_port16( access_word w, const uint16_t* vp );

	/** Read all port method
	 * 
	 *	All port register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return	Pointer to vp
	 */
	uint8_t*	read_port( access_word w, uint8_t* vp );

	/** Read all port method
	 * 
	 *	All port 16 bit register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return	Pointer to vp
	 */
	uint16_t*	read_port16( access_word w, uint16_t* vp );

	/** Write single port method
	 * 
	 *	Single port register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param value		Value to be written into a register
	 * @param port_num	Option, to specify port number
	 */
	void		write_port( access_word w, uint8_t value, int port_num = 0 );

	/** Write single port method
	 * 
	 *	Single port 16 bit register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param value		Value to be written into a register
	 * @param port_num	Option, to specify port number
	 */
	void		write_port16( access_word w, uint16_t value, int port_num = 0 );

	/** Read single port method
	 * 
	 *	Single port register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param port_num	Option, to specify port number
	 * @return Register read value
	 */
	uint8_t		read_port( access_word w, int port_num = 0 );

	/** Read single port method
	 * 
	 *	Single port 16 bit register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param port_num	Option, to specify port number
	 * @return Register read value
	 */
	uint16_t	read_port16( access_word w, int port_num = 0 );

	/** Ping the device
	 *
	 * @return true when ACK 
	 */
	bool ping( void );
		
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
#endif	//	DOXYGEN_ONLY
};

/** PCAL6416A class
 *	
 *  @class PCAL6416A
 */
class PCAL6416A : public PCAL6xxx_base
{
public:
	/** Name of the PCAL6416A registers */
	enum reg_num {
		Input_Port_0, Input_Port_1,
		Output_Port_0, Output_Port_1,
		Polarity_Inversion_port_0, Polarity_Inversion_port_1,
		Configuration_port_0, Configuration_port_1,
		Output_drive_strength_register_0=0x40, Output_drive_strength_register_0B,
		Output_drive_strength_register_1, Output_drive_strength_register_1B,
		Input_latch_register_0, Input_latch_register_1, 
		Pull_up_pull_down_enable_register_0, Pull_up_pull_down_enable_register_1, 
		Pull_up_pull_down_selection_register_0, Pull_up_pull_down_selection_register_1, 
		Interrupt_mask_register_0, Interrupt_mask_register_1, 
		Interrupt_status_register_0, Interrupt_status_register_1, 
		Output_port_configuration_register,  
	};
	
	/** Constractor
	 * 
	 * @param interface I2C instance
	 * @param i2c_address I2C target address
	 */
	PCAL6416A( I2C& interface, uint8_t i2c_address = (0x40 >> 1) + 0 );

	/** Destractor */
	virtual ~PCAL6416A();

	static constexpr uint8_t	access_ref[ NUM_access_word ]	= {
		Input_Port_0,							//	IN,
		Output_Port_0,							//	OUT
		Polarity_Inversion_port_0,				//	POLARITY
		Configuration_port_0,					//	CONFIG
		Output_drive_strength_register_0,		//	DRIVE_STRENGTH
		Input_latch_register_0,					//	LATCHLATCH
		Pull_up_pull_down_enable_register_0,	//	PULL_UD_EN
		Pull_up_pull_down_selection_register_0,	//	PULL_UD_SEL
		Interrupt_mask_register_0,				//	INT_MASK
		Interrupt_status_register_0,			//	INT_STATUS
		Output_port_configuration_register,		//	OUTPUT_PORT_CONFIG
	};

#if DOXYGEN_ONLY
	/** Constants for begin() method */
	enum board {
		NONE,
		ARDUINO_SHIELD,
	};

	/** Number of IO bits */
	const int	n_bits;

	/** Number of IO ports */
	const int	n_ports;

	/** Device/board initialization
	 *
	 * This method is needed to initialize Arduino-shield type evaluation boards from NXP. 
	 * This method takes one argument of "PCAL6416A::ARDUINO_SHIELD" to set RESET and ADDRESS pins. 
	 * 
	 * If the devoce is used as it self, this method doesn't need to be called. 
	 *	
	 * @param env	This argument can be given as "PCAL6416A::NONE" ot "PCAL6416A::ARDUINO_SHIELD"
	 */
	virtual void	begin( board env = NONE );
	
	/** Output, single port
	 * 
	 *	Basic GPIO port access function for single port output
	 *
	 * @param port	Port number
	 * @param value	Value to be output
	 * @param mask	Bit mask. Value will not be changed in bit positions '1' in mask
	 */
	void		output( int port, uint8_t value, uint8_t mask = 0 );

	/** Output, all ports
	 * 
	 *	Basic GPIO port access function for all ports output
	 *
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		output( const uint8_t *vp );
	
	/** Input, single port
	 * 
	 *	Basic GPIO port access function for single port input
	 *
	 * @param port	Port number
	 * @return Port read value
	 */
	uint8_t		input( int port );

	/** Input, all ports
	 * 
	 *	Basic GPIO port access function for all ports input
	 *
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return Pointer to vp
	 */
	uint8_t*	input( uint8_t *vp );

	/** Config, single port
	 * 
	 *	Basic GPIO port access function for single port configuration
	 *
	 * @param port	Port number
	 * @param value	Value to be written into configuration register
	 * @param mask	Bit mask. Value will not be changed in bit positions '1' in mask
	 */
	void		config( int port, uint8_t config, uint8_t mask = 0 );

	/** Config, all ports
	 * 
	 *	Basic GPIO port access function for all port configuration
	 *
	 * @param vp	Pointer to array of configuration values. The array should have 'n_ports' length
	 */
	void		config( const uint8_t* vp );

	/** Write all port method
	 * 
	 *	All port register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		write_port( access_word w, const uint8_t* vp );

	/** Write all port method
	 * 
	 *	All port 16 bit register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		write_port16( access_word w, const uint16_t* vp );

	/** Read all port method
	 * 
	 *	All port register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return	Pointer to vp
	 */
	uint8_t*	read_port( access_word w, uint8_t* vp );

	/** Read all port method
	 * 
	 *	All port 16 bit register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return	Pointer to vp
	 */
	uint16_t*	read_port16( access_word w, uint16_t* vp );

	/** Write single port method
	 * 
	 *	Single port register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param value		Value to be written into a register
	 * @param port_num	Option, to specify port number
	 */
	void		write_port( access_word w, uint8_t value, int port_num = 0 );

	/** Write single port method
	 * 
	 *	Single port 16 bit register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param value		Value to be written into a register
	 * @param port_num	Option, to specify port number
	 */
	void		write_port16( access_word w, uint16_t value, int port_num = 0 );

	/** Read single port method
	 * 
	 *	Single port register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param port_num	Option, to specify port number
	 * @return Register read value
	 */
	uint8_t		read_port( access_word w, int port_num = 0 );

	/** Read single port method
	 * 
	 *	Single port 16 bit register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param port_num	Option, to specify port number
	 * @return Register read value
	 */
	uint16_t	read_port16( access_word w, int port_num = 0 );

	/** Ping the device
	 *
	 * @return true when ACK 
	 */
	bool ping( void );
		
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
#endif	//	DOXYGEN_ONLY
};

/** PCAL6524 class
 *	
 *  @class PCAL6524
 */
class PCAL6524 : public PCAL6xxx_base
{
public:
	/** Name of the PCAL6524 registers */
	enum reg_num {
		Input_Port_0, Input_Port_1, Input_Port_2, reserved0, 
		Output_Port_0, Output_Port_1, Output_Port_2, reserved1, 
		Polarity_Inversion_port_0, Polarity_Inversion_port_1, Polarity_Inversion_port_2, reserved2, 
		Configuration_port_0, Configuration_port_1, Configuration_port_2, 
		Output_drive_strength_register_port_0A=0x40, Output_drive_strength_register_port_0B, 
		Output_drive_strength_register_port_1A, Output_drive_strength_register_port_1B, 
		Output_drive_strength_register_port_2A, Output_drive_strength_register_port_2B, 
		reserved3, reserved4, 
		Input_latch_register_port_0, Input_latch_register_port_1, Input_latch_register_port_2, reserved5, 
		Pull_up_pull_down_enable_register_port_0, Pull_up_pull_down_enable_register_port_1, Pull_up_pull_down_enable_register_port_2, reserved6, 
		Pull_up_pull_down_selection_register_port_0, Pull_up_pull_down_selection_register_port_1, Pull_up_pull_down_selection_register_port_2, reserved7, 
		Interrupt_mask_register_port_0, Interrupt_mask_register_port_1, Interrupt_mask_register_port_2, reserved8, 
		Interrupt_status_register_port_0, Interrupt_status_register_port_1, Interrupt_status_register_port_2, reserved9, 
		Output_port_configuration_register, reserved10, reserved11, reserved12, 
		Interrupt_edge_register_port_0A, Interrupt_edge_register_port_0B, 
		Interrupt_edge_register_port_1A, Interrupt_edge_register_port_1B, 
		Interrupt_edge_register_port_2A, Interrupt_edge_register_port_2B, 
		reserved13, reserved14, 
		Interrupt_clear_register_port_0, Interrupt_clear_register_port_1, Interrupt_clear_register_port_2, reserved15, 
		Input_status_port_0, Input_status_port_1, Input_status_port_2, reserved16, 
		Individual_pin_output_port_0_configuration_register, Individual_pin_output_port_1_configuration_register, Individual_pin_output_port_2_configuration_register, reserved17, 
		Switch_debounce_enable_0, Switch_debounce_enable_1, Switch_debounce_count, 
	};
	
	/** Constractor
	 * 
	 * @param interface I2C instance
	 * @param i2c_address I2C target address
	 */
	PCAL6524( I2C& interface, uint8_t i2c_address = (0x44 >> 1) + 0 );

	/** Destractor */
	virtual ~PCAL6524();

	static constexpr uint8_t	access_ref[ NUM_access_word ]	= {
		Input_Port_0,									//	IN,
		Output_Port_0,									//	OUT
		Polarity_Inversion_port_0,						//	POLARITY
		Configuration_port_0,							//	CONFIG
		Output_drive_strength_register_port_0A,			//	DRIVE_STRENGTH
		Input_latch_register_port_0,					//	LATCHLATCH
		Pull_up_pull_down_enable_register_port_0,		//	PULL_UD_EN
		Pull_up_pull_down_selection_register_port_0,	//	PULL_UD_SEL
		Interrupt_mask_register_port_0,					//	INT_MASK
		Interrupt_status_register_port_0,				//	INT_STATUS
		Output_port_configuration_register,				//	OUTPUT_PORT_CONFIG
	};

#if DOXYGEN_ONLY
	/** Constants for begin() method */
	enum board {
		NONE,
		ARDUINO_SHIELD,
	};

	/** Number of IO bits */
	const int	n_bits;

	/** Number of IO ports */
	const int	n_ports;

	/** Device/board initialization
	 *
	 * This method is needed to initialize Arduino-shield type evaluation boards from NXP. 
	 * This method takes one argument of "PCAL6524::ARDUINO_SHIELD" to set RESET and ADDRESS pins. 
	 * 
	 * If the devoce is used as it self, this method doesn't need to be called. 
	 *	
	 * @param env	This argument can be given as "PCAL6524::NONE" ot "PCAL6524::ARDUINO_SHIELD"
	 */
	virtual void	begin( board env = NONE );
	
	/** Output, single port
	 * 
	 *	Basic GPIO port access function for single port output
	 *
	 * @param port	Port number
	 * @param value	Value to be output
	 * @param mask	Bit mask. Value will not be changed in bit positions '1' in mask
	 */
	void		output( int port, uint8_t value, uint8_t mask = 0 );

	/** Output, all ports
	 * 
	 *	Basic GPIO port access function for all ports output
	 *
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		output( const uint8_t *vp );
	
	/** Input, single port
	 * 
	 *	Basic GPIO port access function for single port input
	 *
	 * @param port	Port number
	 * @return Port read value
	 */
	uint8_t		input( int port );

	/** Input, all ports
	 * 
	 *	Basic GPIO port access function for all ports input
	 *
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return Pointer to vp
	 */
	uint8_t*	input( uint8_t *vp );

	/** Config, single port
	 * 
	 *	Basic GPIO port access function for single port configuration
	 *
	 * @param port	Port number
	 * @param value	Value to be written into configuration register
	 * @param mask	Bit mask. Value will not be changed in bit positions '1' in mask
	 */
	void		config( int port, uint8_t config, uint8_t mask = 0 );

	/** Config, all ports
	 * 
	 *	Basic GPIO port access function for all port configuration
	 *
	 * @param vp	Pointer to array of configuration values. The array should have 'n_ports' length
	 */
	void		config( const uint8_t* vp );

	/** Write all port method
	 * 
	 *	All port register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		write_port( access_word w, const uint8_t* vp );

	/** Write all port method
	 * 
	 *	All port 16 bit register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		write_port16( access_word w, const uint16_t* vp );

	/** Read all port method
	 * 
	 *	All port register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return	Pointer to vp
	 */
	uint8_t*	read_port( access_word w, uint8_t* vp );

	/** Read all port method
	 * 
	 *	All port 16 bit register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return	Pointer to vp
	 */
	uint16_t*	read_port16( access_word w, uint16_t* vp );

	/** Write single port method
	 * 
	 *	Single port register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param value		Value to be written into a register
	 * @param port_num	Option, to specify port number
	 */
	void		write_port( access_word w, uint8_t value, int port_num = 0 );

	/** Write single port method
	 * 
	 *	Single port 16 bit register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param value		Value to be written into a register
	 * @param port_num	Option, to specify port number
	 */
	void		write_port16( access_word w, uint16_t value, int port_num = 0 );

	/** Read single port method
	 * 
	 *	Single port register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param port_num	Option, to specify port number
	 * @return Register read value
	 */
	uint8_t		read_port( access_word w, int port_num = 0 );

	/** Read single port method
	 * 
	 *	Single port 16 bit register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param port_num	Option, to specify port number
	 * @return Register read value
	 */
	uint16_t	read_port16( access_word w, int port_num = 0 );

	/** Ping the device
	 *
	 * @return true when ACK 
	 */
	bool ping( void );
		
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
#endif	//	DOXYGEN_ONLY
};

/** PCAL6534 class
 *	
 *  @class PCAL6534
 */
class PCAL6534 : public PCAL6xxx_base
{
public:
	/** Name of the PCAL6534 registers */
	enum reg_num {
		Input_Port_0,  Input_Port_1,  Input_Port_2,  Input_Port_3,  Input_Port_4,
		Output_Port_0, Output_Port_1, Output_Port_2, Output_Port_3, Output_Port_4,
		Polarity_Inversion_port_0, Polarity_Inversion_port_1, Polarity_Inversion_port_2, Polarity_Inversion_port_3, Polarity_Inversion_port_4,
		Configuration_port_0, Configuration_port_1, Configuration_port_2, Configuration_port_3, Configuration_port_4,
		Output_drive_strength_register_port_0A	= 0x30, Output_drive_strength_register_port_0B,
		Output_drive_strength_register_port_1A, Output_drive_strength_register_port_1B,
		Output_drive_strength_register_port_2A, Output_drive_strength_register_port_2B,
		Output_drive_strength_register_port_3A, Output_drive_strength_register_port_3B,
		Output_drive_strength_register_port_4A, reserved0,
		Input_latch_register_port_0, Input_latch_register_port_1, Input_latch_register_port_2, Input_latch_register_port_3, Input_latch_register_port_4,
		Pull_up_pull_down_enable_register_port_0, Pull_up_pull_down_enable_register_port_1, Pull_up_pull_down_enable_register_port_2, Pull_up_pull_down_enable_register_port_3, Pull_up_pull_down_enable_register_port_4,
		Pull_up_pull_down_selection_register_port_0, Pull_up_pull_down_selection_register_port_1, Pull_up_pull_down_selection_register_port_2, Pull_up_pull_down_selection_register_port_3, Pull_up_pull_down_selection_register_port_4,
		Interrupt_mask_register_port_0, Interrupt_mask_register_port_1, Interrupt_mask_register_port_2, Interrupt_mask_register_port_3, Interrupt_mask_register_port_4,
		Interrupt_status_register_port_0, Interrupt_status_register_port_1, Interrupt_status_register_port_2, Interrupt_status_register_port_3, Interrupt_status_register_port_4,
		Output_port_configuration_register,
		Interrupt_edge_register_port_0A, Interrupt_edge_register_port_0B,
		Interrupt_edge_register_port_1A, Interrupt_edge_register_port_1B,
		Interrupt_edge_register_port_2A, Interrupt_edge_register_port_2B,
		Interrupt_edge_register_port_3A, Interrupt_edge_register_port_3B,
		Interrupt_edge_register_port_4A, reserved1,
		Interrupt_clear_register_port_0, Interrupt_clear_register_port_1, Interrupt_clear_register_port_2, Interrupt_clear_register_port_3, Interrupt_clear_register_port_4,
		Input_status_port_0, Input_status_port_1, Input_status_port_2, Input_status_port_3, Input_status_port_4,
		Individual_pin_output_port_0_configuration_register, Individual_pin_output_port_1_configuration_register, Individual_pin_output_port_2_configuration_register, Individual_pin_output_port_3_configuration_register, Individual_pin_output_port_4_configuration_register,
		Switch_debounce_enable_0, Switch_debounce_enable_1,
		Switch_debounce_count,
	};
	
	/** Constractor
	 * 
	 * @param interface I2C instance
	 * @param i2c_address I2C target address
	 */
	PCAL6534( I2C& interface, uint8_t i2c_address = (0x44 >> 1) + 0 );

	/** Destractor */
	virtual ~PCAL6534();

	static constexpr uint8_t	access_ref[ NUM_access_word ]	= {
		Input_Port_0,									//	IN,
		Output_Port_0,									//	OUT
		Polarity_Inversion_port_0,						//	POLARITY
		Configuration_port_0,							//	CONFIG
		Output_drive_strength_register_port_0A,			//	DRIVE_STRENGTH
		Input_latch_register_port_0,					//	LATCHLATCH
		Pull_up_pull_down_enable_register_port_0,		//	PULL_UD_EN
		Pull_up_pull_down_selection_register_port_0,	//	PULL_UD_SEL
		Interrupt_mask_register_port_0,					//	INT_MASK
		Interrupt_status_register_port_0,				//	INT_STATUS
		Output_port_configuration_register,				//	OUTPUT_PORT_CONFIG
	};

#if DOXYGEN_ONLY
	/** Constants for begin() method */
	enum board {
		NONE,
		ARDUINO_SHIELD,
	};

	/** Number of IO bits */
	const int	n_bits;

	/** Number of IO ports */
	const int	n_ports;

	/** Device/board initialization
	 *
	 * This method is needed to initialize Arduino-shield type evaluation boards from NXP. 
	 * This method takes one argument of "PCAL6534::ARDUINO_SHIELD" to set RESET and ADDRESS pins. 
	 * 
	 * If the devoce is used as it self, this method doesn't need to be called. 
	 *	
	 * @param env	This argument can be given as "PCAL6534::NONE" ot "PCAL6534::ARDUINO_SHIELD"
	 */
	virtual void	begin( board env = NONE );
	
	/** Output, single port
	 * 
	 *	Basic GPIO port access function for single port output
	 *
	 * @param port	Port number
	 * @param value	Value to be output
	 * @param mask	Bit mask. Value will not be changed in bit positions '1' in mask
	 */
	void		output( int port, uint8_t value, uint8_t mask = 0 );

	/** Output, all ports
	 * 
	 *	Basic GPIO port access function for all ports output
	 *
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		output( const uint8_t *vp );
	
	/** Input, single port
	 * 
	 *	Basic GPIO port access function for single port input
	 *
	 * @param port	Port number
	 * @return Port read value
	 */
	uint8_t		input( int port );

	/** Input, all ports
	 * 
	 *	Basic GPIO port access function for all ports input
	 *
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return Pointer to vp
	 */
	uint8_t*	input( uint8_t *vp );

	/** Config, single port
	 * 
	 *	Basic GPIO port access function for single port configuration
	 *
	 * @param port	Port number
	 * @param value	Value to be written into configuration register
	 * @param mask	Bit mask. Value will not be changed in bit positions '1' in mask
	 */
	void		config( int port, uint8_t config, uint8_t mask = 0 );

	/** Config, all ports
	 * 
	 *	Basic GPIO port access function for all port configuration
	 *
	 * @param vp	Pointer to array of configuration values. The array should have 'n_ports' length
	 */
	void		config( const uint8_t* vp );

	/** Write all port method
	 * 
	 *	All port register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		write_port( access_word w, const uint8_t* vp );

	/** Write all port method
	 * 
	 *	All port 16 bit register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 */
	void		write_port16( access_word w, const uint16_t* vp );

	/** Read all port method
	 * 
	 *	All port register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return	Pointer to vp
	 */
	uint8_t*	read_port( access_word w, uint8_t* vp );

	/** Read all port method
	 * 
	 *	All port 16 bit register access function using word of 'access_word'
	 *
	 * @param w		Accsess word. This should be choosen from access_word'
	 * @param vp	Pointer to an array of values. The array should have 'n_ports' length
	 * @return	Pointer to vp
	 */
	uint16_t*	read_port16( access_word w, uint16_t* vp );

	/** Write single port method
	 * 
	 *	Single port register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param value		Value to be written into a register
	 * @param port_num	Option, to specify port number
	 */
	void		write_port( access_word w, uint8_t value, int port_num = 0 );

	/** Write single port method
	 * 
	 *	Single port 16 bit register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param value		Value to be written into a register
	 * @param port_num	Option, to specify port number
	 */
	void		write_port16( access_word w, uint16_t value, int port_num = 0 );

	/** Read single port method
	 * 
	 *	Single port register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param port_num	Option, to specify port number
	 * @return Register read value
	 */
	uint8_t		read_port( access_word w, int port_num = 0 );

	/** Read single port method
	 * 
	 *	Single port 16 bit register access function using word of 'access_word'
	 *
	 * @param w			Accsess word. This should be choosen from access_word'
	 * @param port_num	Option, to specify port number
	 * @return Register read value
	 */
	uint16_t	read_port16( access_word w, int port_num = 0 );

	/** Ping the device
	 *
	 * @return true when ACK 
	 */
	bool ping( void );
		
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
#endif	//	DOXYGEN_ONLY
};



/** PCAL97xx_base class
 *	
 *  @class PCAL97xx_base
 *
 *	Yet another abstraction class for PCAL6xxx devices
 *	This class is just passing parameters to GPIO_base class in this version
 */
class PCAL97xx_base : public GPIO_base
{
public:
	PCAL97xx_base( SPI& interface, uint8_t dev_address, const int nbits, const uint8_t arp[], uint8_t ai );
	virtual ~PCAL97xx_base();
};

/** PCAL9722 class
 *	
 *  @class PCAL9722
 */
class PCAL9722 : public PCAL97xx_base
{
public:
	/** Name of the PCAL9722 registers */
	enum reg_num {
		Input_Port_0, Input_Port_1, Input_Port_2, reserved0, 
		Output_Port_0, Output_Port_1, Output_Port_2, reserved1, 
		Polarity_Inversion_port_0, Polarity_Inversion_port_1, Polarity_Inversion_port_2, reserved2, 
		Configuration_port_0, Configuration_port_1, Configuration_port_2, 
		Output_drive_strength_register_port_0A=0x40, Output_drive_strength_register_port_0B, 
		Output_drive_strength_register_port_1A, Output_drive_strength_register_port_1B, 
		Output_drive_strength_register_port_2A, Output_drive_strength_register_port_2B, 
		reserved3, reserved4, 
		Input_latch_register_port_0, Input_latch_register_port_1, Input_latch_register_port_2, reserved5, 
		Pull_up_pull_down_enable_register_port_0, Pull_up_pull_down_enable_register_port_1, Pull_up_pull_down_enable_register_port_2, reserved6, 
		Pull_up_pull_down_selection_register_port_0, Pull_up_pull_down_selection_register_port_1, Pull_up_pull_down_selection_register_port_2, reserved7, 
		Interrupt_mask_register_port_0, Interrupt_mask_register_port_1, Interrupt_mask_register_port_2, reserved8, 
		Interrupt_status_register_port_0, Interrupt_status_register_port_1, Interrupt_status_register_port_2, reserved9, 
		Output_port_configuration_register, reserved10, reserved11, reserved12, 
		Interrupt_edge_register_port_0A, Interrupt_edge_register_port_0B, 
		Interrupt_edge_register_port_1A, Interrupt_edge_register_port_1B, 
		Interrupt_edge_register_port_2A, Interrupt_edge_register_port_2B, 
		reserved13, reserved14, 
		Interrupt_clear_register_port_0, Interrupt_clear_register_port_1, Interrupt_clear_register_port_2, reserved15, 
		Input_status_port_0, Input_status_port_1, Input_status_port_2, reserved16, 
		Individual_pin_output_port_0_configuration_register, Individual_pin_output_port_1_configuration_register, Individual_pin_output_port_2_configuration_register, reserved17, 
		Switch_debounce_enable_0, Switch_debounce_enable_1, Switch_debounce_count, 
	};
	
	/** Constractor
	 * 
	 * @param interface I2C instance
	 * @param i2c_address I2C target address
	 */
	PCAL9722( SPI& interface, uint8_t dev_address = (0x40 >> 1) + 0 );

	/** Destractor */
	virtual ~PCAL9722();

	/** Device/board initialization
	 *
	 * This method is needed to initialize Arduino-shield type evaluation boards from NXP. 
	 * This method takes one argument of "PCAL6534::ARDUINO_SHIELD" to set RESET and ADDRESS pins. 
	 * 
	 * If the devoce is used as it self, this method doesn't need to be called. 
	 *	
	 * @param env	This argument can be given as "PCAL6534::NONE" ot "PCAL6534::ARDUINO_SHIELD"
	 */
	void	begin( board env = NONE );
	
	static constexpr uint8_t	access_ref[ NUM_access_word ]	= {
		Input_Port_0,									//	IN,
		Output_Port_0,									//	OUT
		Polarity_Inversion_port_0,						//	POLARITY
		Configuration_port_0,							//	CONFIG
		Output_drive_strength_register_port_0A,			//	DRIVE_STRENGTH
		Input_latch_register_port_0,					//	LATCHLATCH
		Pull_up_pull_down_enable_register_port_0,		//	PULL_UD_EN
		Pull_up_pull_down_selection_register_port_0,	//	PULL_UD_SEL
		Interrupt_mask_register_port_0,					//	INT_MASK
		Interrupt_status_register_port_0,				//	INT_STATUS
		Output_port_configuration_register,				//	OUTPUT_PORT_CONFIG
	};

private:
	static constexpr int RESET_PIN_PCAL9722	= D6;
};


#endif //	ARDUINO_GPIO_NXP_ARD_H
