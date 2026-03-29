/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef R01LIB_IO_H
#define R01LIB_IO_H

#include	"fsl_port.h"
#include	"pin_mux.h"
#include	"clock_config.h"
#include	"board.h"
#include	"obj.h"

#define	PIN_OUTPUT			kGPIO_DigitalOutput
#define	PIN_INPUT			kGPIO_DigitalInput

/** pin names  */
#ifdef	CPU_MCXN947VDF
enum { 	
	DISABLED_PIN, 
	P0_0, P0_1, P0_2, P0_3,
	P0_4,
	P0_5,
	P0_6,
	P0_7,
	P0_8,
	P0_9,
	P0_10,
	P0_11,
	P0_12,
	P0_13,
	P0_14,
	P0_15,
	P0_16,
	P0_17,
	P0_18,
	P0_19,
	P0_20,
	P0_21,
	P0_22,
	P0_23,
	P0_24,
	P0_25,
	P0_26,
	P0_27,
	P0_28,
	P0_29,
	P0_30,
	P0_31,
	P1_0,
	P1_1,
	P1_2,
	P1_3,
	P1_4,
	P1_5,
	P1_6,
	P1_7,
	P1_8,
	P1_9,
	P1_10,
	P1_11,
	P1_12,
	P1_13,
	P1_14,
	P1_15,
	P1_16,
	P1_17,
	P1_18,
	P1_19,
	P1_20,
	P1_21,
	P1_22,
	P1_23,
	P1_30,
	P1_31,
	P2_0,
	P2_1,
	P2_2,
	P2_3,
	P2_4,
	P2_5,
	P2_6,
	P2_7,
	P2_8,
	P2_9,
	P2_10,
	P2_11,
	P3_0,
	P3_1,
	P3_2,
	P3_3,
	P3_4,
	P3_5,
	P3_6,
	P3_7,
	P3_8,
	P3_9,
	P3_10,
	P3_11,
	P3_12,
	P3_13,
	P3_14,
	P3_15,
	P3_16,
	P3_17,
	P3_18,
	P3_19,
	P3_20,
	P3_21,
	P3_22,
	P3_23,
	P4_0,
	P4_1,
	P4_2,
	P4_3,
	P4_4,
	P4_5,
	P4_6,
	P4_7,
	P4_12,
	P4_13,
	P4_14,
	P4_15,
	P4_16,
	P4_17,
	P4_18,
	P4_19,
	P4_20,
	P4_21,
	P4_22,
	P4_23,
	P5_0,
	P5_1,
	P5_2,
	P5_3,
	P5_4,
	P5_5,
	P5_6,
	P5_7,
	P5_8,
	P5_9,
};

	#define	D0		P4_3
	#define	D1		P4_2
	#define	D2		P0_29
	#define	D3		P1_23
	#define	D4		P0_30
	#define	D5		P1_21
	#define	D6		P1_2
	#define	D7		P0_31
	#define	D8		P0_28
	#define	D9		P0_10
	#define	D10		P0_27
	#define	D11		P0_24
	#define	D12		P0_26
	#define	D13		P0_25
	#define	D18		P4_0
	#define	D19		P4_1
	#define	A0		DISABLED_PIN
	#define	A1		DISABLED_PIN
	#define	A2		P0_14
	#define	A3		P0_22
	#define	A4		P0_15
	#define	A5		P0_23
	#define	SW2		P0_23
	#define	SW3		P0_6
	#define	MB_AN	DISABLED_PIN
	#define	MB_RST	P1_3
	#define	MB_CS	P3_23
	#define	MB_SCK	P3_21
	#define	MB_MISO	P3_22
	#define	MB_MOSI	P3_20
	#define	MB_PWM	P3_19
	#define	MB_INT	P5_7
	#define	MB_RX	P1_16
	#define	MB_TX	P1_17
	#define	MB_SCL	P1_1
	#define	MB_SDA	P1_0
	#define	RED		D9
	#define	GREEN	D10
	#define	BLUE	D6

	#define	I3C_SDA		MB_RX
	#define	I3C_SCL		MB_TX
	#define	I2C_SDA		D18
	#define	I2C_SCL		D19
	#define	SPI_CS		D10
	#define	SPI_MOSI	D11
	#define	SPI_MISO	D12
	#define	SPI_SCLK	D13
	#define	ARD_CS		SPI_CS
	#define	ARD_MOSI	SPI_MOSI
	#define	ARD_MISO	SPI_MISO
	#define	ARD_SCK		SPI_SCLK

	#define	PIN_LED_OFF	true
	#define	PIN_LED_ON	false



#elif	CPU_MCXN236VDF

enum { 	
	DISABLED_PIN, 
	P0_0,
	P0_1,
	P0_2,
	P0_3,
	P0_4,
	P0_5,
	P0_6,
	P0_7,
	P0_14,
	P0_15,
	P0_16,
	P0_17,
	P0_18,
	P0_19,
	P0_20,
	P0_21,
	P0_22,
	P0_23,
	P0_24,
	P0_25,
	P0_26,
	P0_27,
	P0_28,
	P0_29,
	P1_0,
	P1_1,
	P1_2,
	P1_3,
	P1_4,
	P1_5,
	P1_6,
	P1_7,
	P1_8,
	P1_9,
	P1_10,
	P1_11,
	P1_12,
	P1_13,
	P1_14,
	P1_15,
	P1_16,
	P1_17,
	P1_18,
	P1_19,
	P1_30,
	P1_31,
	P2_0,
	P2_1,
	P2_2,
	P2_3,
	P2_4,
	P2_5,
	P2_6,
	P2_7,
	P2_8,
	P2_9,
	P2_10,
	P2_11,
	P3_0,
	P3_1,
	P3_2,
	P3_6,
	P3_7,
	P3_8,
	P3_9,
	P3_10,
	P3_11,
	P3_12,
	P3_13,
	P3_14,
	P3_15,
	P3_16,
	P3_17,
	P3_18,
	P3_20,
	P3_21,
	P3_22,
	P3_23,
	P4_0,
	P4_1,
	P4_2,
	P4_3,
	P4_4,
	P4_5,
	P4_6,
	P4_7,
	P4_12,
	P4_13,
	P4_14,
	P4_15,
	P4_16,
	P4_17,
	P4_18,
	P4_19,
	P4_20,
	P4_21,
	P4_22,
	P4_23,
	P5_0,
	P5_1,
	P5_2,
	P5_3,
	P5_4,
	P5_5,
	P5_6,
	P5_7,
};

	#define	D0		P4_3
	#define	D1		P4_2
	#define	D2		P2_0
	#define	D3		P3_12
	#define	D4		P0_21
	#define	D5		P2_7
	#define	D6		P3_17
	#define	D7		P0_22
	#define	D8		P0_23
	#define	D9		P3_14
	#define	D10		P1_3
	#define	D11		P1_0
	#define	D12		P1_2
	#define	D13		P1_1
	#define	D18		P1_16
	#define	D19		P1_17
	#define	A0		P4_6
	#define	A1		P4_15
	#define	A2		P4_16
	#define	A3		P4_17
	#define	A4		P4_12
	#define	A5		P4_13
	#define	SW2		P0_20
	#define	SW3		P0_6
	#define	MB_AN	P5_3
	#define	MB_RST	P5_2
	#define	MB_CS	D18
	#define	MB_SCK	P1_1
	#define	MB_MISO	P1_2
	#define	MB_MOSI	P1_0
	#define	MB_PWM	P4_18
	#define	MB_INT	P5_6
	#define	MB_RX	P4_3
	#define	MB_TX	P4_2
	#define	MB_SCL	P4_1
	#define	MB_SDA	P4_0
	#define	RED		MB_PWM
	#define	GREEN	P4_19
	#define	BLUE	A3

	#define	I3C_SDA		D18
	#define	I3C_SCL		D19
	#define	I2C_SDA		A4
	#define	I2C_SCL		A5
	#define	SPI_CS		D10
	#define	SPI_MOSI	D11
	#define	SPI_MISO	D12
	#define	SPI_SCLK	D13
	#define	ARD_CS		SPI_CS
	#define	ARD_MOSI	SPI_MOSI
	#define	ARD_MISO	SPI_MISO
	#define	ARD_SCK		SPI_SCLK

	#define	PIN_LED_OFF	true
	#define	PIN_LED_ON	false


#elif	CPU_MCXA156VLL

enum { 	
	DISABLED_PIN, 
	P0_0,
	P0_1,
	P0_2,
	P0_3,
	P0_6,
	P0_16,
	P0_17,
	P0_18,
	P0_19,
	P0_20,
	P0_21,
	P0_22,
	P0_23,
	P1_0,
	P1_1,
	P1_2,
	P1_3,
	P1_4,
	P1_5,
	P1_6,
	P1_7,
	P1_8,
	P1_9,
	P1_10,
	P1_11,
	P1_12,
	P1_13,
	P1_14,
	P1_15,
	P1_29,
	P1_30,
	P1_31,
	P2_0,
	P2_1,
	P2_2,
	P2_3,
	P2_4,
	P2_5,
	P2_6,
	P2_7,
	P2_10,
	P2_11,
	P2_12,
	P2_13,
	P2_15,
	P2_16,
	P2_17,
	P2_19,
	P2_20,
	P2_21,
	P2_23,
	P3_0,
	P3_1,
	P3_6,
	P3_7,
	P3_8,
	P3_9,
	P3_10,
	P3_11,
	P3_12,
	P3_13,
	P3_14,
	P3_15,
	P3_16,
	P3_17,
	P3_18,
	P3_19,
	P3_20,
	P3_21,
	P3_22,
	P3_27,
	P3_28,
	P3_29,
	P3_30,
	P3_31,
	P4_2,
	P4_3,
	P4_4,
	P4_5,
	P4_6,
	P4_7,
};

	#define	D0		P2_11
	#define	D1		P2_10
	#define	D2		P3_1
	#define	D3		P3_12
	#define	D4		P3_31
	#define	D5		P3_14
	#define	D6		P3_16
	#define	D7		P1_14
	#define	D8		P1_15
	#define	D9		P3_17
	#define	D10		P3_13
	#define	D11		P3_15
	#define	D12		P2_16
	#define	D13		P2_12
	#define	D18		P0_16
	#define	D19		P0_17
	#define	A0		P1_10
	#define	A1		P2_5
	#define	A2		P2_3
	#define	A3		P2_4
	#define	A4		P1_12
	#define	A5		P1_13
	#define	SW2		P1_7
	#define	SW3		P0_6
	#define	MB_AN	P3_30
	#define	MB_RST	P3_29
	#define	MB_CS	P1_3
	#define	MB_SCK	P1_1
	#define	MB_MISO	P1_2
	#define	MB_MOSI	P1_0
	#define	MB_PWM	P3_18
	#define	MB_INT	P3_19
	#define	MB_RX	P3_20
	#define	MB_TX	P3_21
	#define	MB_SCL	P3_27
	#define	MB_SDA	P3_28
	#define	RED		D3
	#define	GREEN	D10
	#define	BLUE	P3_0

	#define	I3C_SDA		D18
	#define	I3C_SCL		D19
	#define	I2C_SDA		D18
	#define	I2C_SCL		D19
	#define	SPI_CS		MB_CS
	#define	SPI_MOSI	MB_MOSI
	#define	SPI_MISO	MB_MISO
	#define	SPI_SCLK	MB_SCK
	#define	ARD_CS		P2_6
	#define	ARD_MOSI	P2_13
	#define	ARD_MISO	D12
	#define	ARD_SCK		D13

	#define	PIN_LED_OFF	true
	#define	PIN_LED_ON	false

#elif	CPU_MCXA153VLH


enum { 	
	DISABLED_PIN, 
	P0_0,
	P0_1,
	P0_2,
	P0_3,
	P0_6,
	P0_16,
	P0_17,
	P1_0,
	P1_1,
	P1_2,
	P1_3,
	P1_4,
	P1_5,
	P1_6,
	P1_7,
	P1_8,
	P1_9,
	P1_10,
	P1_11,
	P1_12,
	P1_13,
	P1_29,
	P1_30,
	P1_31,
	P2_0,
	P2_1,
	P2_2,
	P2_3,
	P2_4,
	P2_5,
	P2_6,
	P2_7,
	P2_12,
	P2_13,
	P2_16,
	P3_0,
	P3_1,
	P3_6,
	P3_7,
	P3_8,
	P3_9,
	P3_10,
	P3_11,
	P3_12,
	P3_13,
	P3_14,
	P3_15,
	P3_27,
	P3_28,
	P3_29,
	P3_30,
	P3_31,
};

	#define	D0		P1_4
	#define	D1		P1_5
	#define	D2		P2_4
	#define	D3		P3_0
	#define	D4		P2_5
	#define	D5		P3_12
	#define	D6		P3_13
	#define	D7		P3_1
	#define	D8		P3_15
	#define	D9		P3_14
	#define	D10		P2_6
	#define	D11		P2_13
	#define	D12		P2_16
	#define	D13		P2_12
	#define	D18		P1_8
	#define	D19		P1_9
	#define	A0		P1_10
	#define	A1		P1_12
	#define	A2		P1_13
	#define	A3		P2_0
	#define	A4		P3_31
	#define	A5		P3_30
	#define	SW2		P3_29
	#define	SW3		P1_7
	#define	MB_AN	P3_30
	#define	MB_RST	P3_1
	#define	MB_CS	P1_3
	#define	MB_SCK	P1_1
	#define	MB_MISO	P1_2
	#define	MB_MOSI	P1_0
	#define	MB_PWM	P3_12
	#define	MB_INT	P2_5
	#define	MB_RX	P3_14
	#define	MB_TX	P3_15
	#define	MB_SCL	P3_27
	#define	MB_SDA	P3_28
	#define	RED		D5
	#define	GREEN	D6
	#define	BLUE	D3

	#define	I3C_SDA		P0_16
	#define	I3C_SCL		P0_17
	#define	I2C_SDA		D18
	#define	I2C_SCL		D19
	#define	SPI_CS		D10
	#define	SPI_MOSI	D11
	#define	SPI_MISO	D12
	#define	SPI_SCLK	D13
	#define	ARD_CS		SPI_CS
	#define	ARD_MOSI	SPI_MOSI
	#define	ARD_MISO	SPI_MISO
	#define	ARD_SCK		SPI_SCLK

	#define	PIN_LED_OFF	true
	#define	PIN_LED_ON	false


#else
	#error Target CPU is not supported
#endif // CPU_MCXN947VDF


#define	ARD_D0	D0
#define	ARD_D1	D1
#define	ARD_D2	D2
#define	ARD_D3	D3
#define	ARD_D4	D4
#define	ARD_D5	D5
#define	ARD_D6	D6
#define	ARD_D7	D7
#define	ARD_D8	D8
#define	ARD_D9	D9
#define	ARD_D10	D10
#define	ARD_D11	D11
#define	ARD_D12	D12
#define	ARD_D13	D13
#define	ARD_D18	D18
#define	ARD_D19	D19
#define	ARD_A0	A0
#define	ARD_A1	A1
#define	ARD_A2	A2
#define	ARD_A3	A3
#define	ARD_A4	A4
#define	ARD_A5	A5

/** DigitalInOut class
 *	
 *  @class DigitalInOut
 *
 *	A class for operating GPIO easy
 */

class DigitalInOut: public Obj
{
public:
	enum	direction {
		INPUT	= kGPIO_DigitalInput, 
		OUTPUT	= kGPIO_DigitalOutput
	};
	
	enum	PinMode {
		PullNone	= 0x0,
		PullUp		= 0x1,
		PullDown	= 0x2,
		OpenDrain	= 0x8
	};

	/** Create a DigitalInOut instance with specified pins
	 *
	 * @param pin_num pin number
	 * @param direction (option) direction setting
	 * @param value (option) default value for output
	 * @param pin_mode (option) PullUp, PullDown, PullNone, OpenDrain
	 */
	DigitalInOut( uint8_t pin_num, bool direction = kGPIO_DigitalInput, bool value = 0, int pin_mode = PullNone );

	/** Destractor
	 */
	virtual ~DigitalInOut();
	
	/** Pin output seting
	 *
	 * @param value setting output
	 */
	void	value( bool value );

	/** Pin input state read
	 *
	 * @return pin state
	 */
	bool	value( void );
	
	/** Pin direction to set as output
	 */
	void	output( void );

	/** Pin direction to set as in
	 */
	void	input( void );

	/** Pin mux setting
	 * This interface is provided to other class drivers to change pin config dynamically
	 */
	void	pin_mux( int mux );
		
	/** Pin mode setting
	 * @param pin_mode PullUp, PullDown, PullNone, OpenDrain
	 */
	void	mode( int pin_mode );

	uint32_t mode( void );

	/** A short hand for setting pins
	 */
	DigitalInOut&	operator=( bool v );
	DigitalInOut&	operator=( DigitalInOut& rhs );

	/** A short hand for reading pins
	 */
	operator	bool();

protected:
	void direction( bool dir );
	
	uint8_t		_pn; 
	GPIO_Type	*gpio_n;
	PORT_Type	*port_n;
	uint8_t		gpio_pin; 
	
private:
	bool 	_dir; 
	bool 	_value;
};

/** DigitalOut class
 *	
 *  @class DigitalOut
 *
 *	A class for operating GPIO easy
 */

class DigitalOut : public DigitalInOut
{
public:
	using DigitalInOut::operator=;

	/** Create a DigitalOut instance with specified pins
	 *
	 * @param pin_num pin number
	 * @param value (option) default value for output
	 * @param pin_mode (option) PullUp, PullDown, PullNone, OpenDrain
	 */
	DigitalOut( uint8_t pin_num, bool value = 0, int pin_mode = PullNone );
	virtual ~DigitalOut();
};

/** DigitalIn class
 *	
 *  @class DigitalIn
 *
 *	A class for operating GPIO easy
 */

class DigitalIn : public DigitalInOut
{
public:
	using DigitalInOut::operator=;

	/** Create a DigitalIn instance with specified pins
	 *
	 * @param pin_num pin number
	 * @param pin_mode (option) PullUp, PullDown, PullNone, OpenDrain
	 */
	DigitalIn( uint8_t pin_num, int pin_mode = PullNone );
	virtual ~DigitalIn();
};


static inline void PORT_SetPinPullUpDown( PORT_Type *base, uint32_t pin, int enable, int logic )
{
	base->PCR[pin] = (base->PCR[pin] & ~PORT_PCR_PS_MASK) | PORT_PCR_PS( enable );
	base->PCR[pin] = (base->PCR[pin] & ~PORT_PCR_PE_MASK) | PORT_PCR_PE( logic );
}

static inline void PORT_SetPinOpenDrain( PORT_Type *base, uint32_t pin, int enable )
{
	base->PCR[pin] = (base->PCR[pin] & ~PORT_PCR_ODE_MASK) | PORT_PCR_ODE( enable );
}

static inline uint32_t PORT_GetPinMode( PORT_Type *base, uint32_t pin )
{
	return base->PCR[pin];
}



#endif // R01LIB_IO_H
