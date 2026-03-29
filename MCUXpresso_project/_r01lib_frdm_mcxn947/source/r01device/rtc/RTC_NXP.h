/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef ARDUINO_RTC_DRIVER_NXP_ARD_H
#define ARDUINO_RTC_DRIVER_NXP_ARD_H

#include	"r01lib.h"
#include	"I2C_device.h"
#include	<stdint.h>
#include	<time.h>
#include	<memory>

/** RTC_NXP class
 *	
 *  @class RTC_NXP
 *
 *	RTC_NXP class is a base class for all RTC drivers
 *	All actual device class will be derived from this class
 */

class RTC_NXP
{
public:
	/** Arduino shield board evaluation board selection descriptor */
	enum board {
		NONE,
		ARDUINO_SHIELD,
	};
	/** Alarm setting digit selection descriptor */
	enum alarm_setting {
		SECOND,
		MINUTE,
		HOUR,
		DAY,
		WEEKDAY,
	};

	/** Constructor */
	RTC_NXP();

	/** Destructor */
	virtual ~RTC_NXP();
	
	/** Initializer (pure virtual method) */
	virtual void begin( void )	= 0;
	
	/** time
	 * 
	 *	"time()" in "time.h" compatible method for RTC
	 *
	 * @param tp pointer to time_t variable
	 * @return time_t value of current time
	 */
	time_t time( time_t* tp );
	
	/** set (pure virtual method)
	 * 
	 * @param now_tm struct to set calendar and time in RTC
	 */
	virtual void set( struct tm* now_tm )	= 0;

	/** Detector for oscillation stop (pure virtual method)
	 * 
	 * @return true, if the OSF (Oscillator Stop Flag) is set
	 */
	virtual bool oscillator_stop( void )	= 0;
	
	/** Alarm setting (pure virtual method)
	 * 
	 * @param digit to specify which parameter to set: SECOND, MINUTE, HOUR, DAY, WEEKDAY in 'enum alarm_setting'
	 * @param val Setting value. Set 0x80 to disabling
	 */
	virtual void alarm( alarm_setting digit, int val )	= 0;

	/** Alarm clearing (pure virtual method)
	 */
	virtual void alarm_clear( void )	= 0;

	/** Alarm interrupt disable (pure virtual method)
	 */
	virtual void alarm_disable( void )	= 0;

	/** Interrupt clear (pure virtual method)
	 */
	virtual uint8_t int_clear( void )	= 0;

protected:
	/** rtc_time (pure virtual method)
	 * 
	 * @return time_t returns RTC time in time_t format
	 */
	virtual time_t rtc_time( void )	= 0;
		
	/** Class method for BCD to int conversion
	 * 
	 * @param v BCD value
	 * @return integer 
	 */
	static uint8_t	bcd2dec( uint8_t v );

	/** Class method for int to BCD conversion
	 * 
	 * @param v integer
	 * @return BCD value
	 */
	static uint8_t	dec2bcd( uint8_t v );
};



/** SPI_for_RTC class
 *	
 *	SPI interface operation for RTC devices
 *
 *  @class SPI_for_RTC
 */

class SPI_for_RTC : public Serial_device
{
public:
	SPI_for_RTC( SPI& interface );
	~SPI_for_RTC();

	/** Send data
	 * 
	 * @param data pointer to data buffer
	 * @param size data size
	 * @param stop option: generating STOP-condition after transaction. Defailt: true
	 * @return transferred data size
	 */
	void txrx( uint8_t *data, int size );

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

	/** Register read, 8 bit
	 *
	 * @param reg register index/address/pointer
	 * @return data value
	 */
	uint8_t read_r8( uint8_t reg );

	/** Register overwriting with bit-mask
	 *	
	 *	Register can be updated by bit level
	 *
	 * @param reg register index/address/pointer
	 * @param mask bit-mask to protect overwriting
	 * @param value value to overwrite
	 */
	void bit_op8(  uint8_t reg,  uint8_t mask,  uint8_t value );

private:
	SPI&		spi;
};

/** PCF2131 class
 *	
 *	A base class for PCF2131
 *	Implementing register operation with abstarcted interface
 *
 *  @class PCF2131
 */

class PCF2131 : public RTC_NXP
{
public:
	/** Name of the PCF2131 registers */
	enum reg_num {
		Control_1, Control_2, Control_3, Control_4, Control_5,
		SR_Reset,
		_100th_Seconds, Seconds, Minutes,Hours, Days, Weekdays, Months, Years,
		Second_alarm, Minute_alarm, Hour_alarm, Day_alarm, Weekday_alarm,
		CLKOUT_ctl,
		Timestp_ctl1, Sec_timestp1, Min_timestp1, Hour_timestp1, Day_timestp1, Mon_timestp1, Year_timestp1,
		Timestp_ctl2, Sec_timestp2, Min_timestp2, Hour_timestp2, Day_timestp2, Mon_timestp2, Year_timestp2,
		Timestp_ctl3, Sec_timestp3, Min_timestp3, Hour_timestp3, Day_timestp3, Mon_timestp3, Year_timestp3,
		Timestp_ctl4, Sec_timestp4, Min_timestp4, Hour_timestp4, Day_timestp4, Mon_timestp4, Year_timestp4,
		Aging_offset,
		INT_A_MASK1, INT_A_MASK2, INT_B_MASK1, INT_B_MASK2,
		Watchdg_tim_ctl, Watchdg_tim_val
	};
	/** Periodic interrupt selection descriptor */
	enum periodic_int_select {
		DISABLE,
		EVERY_SECOND,
		EVERY_MINUTE,
	};
	/** Timestamp setting descriptor */
	enum timestanp_setting {
		LAST,
		FIRST,
	};
	/** Interrupt output selector */
	enum int_out_sel {
		INT_A,
		INT_B,
	};

	/** Create a PCF2131 instance connected to I2C with specified address
	 *
	 * @param interface I2C instance
	 * @param i2c_address I2C-bus address (default: (0xBC>>1))
	 */
	PCF2131( I2C& interface, uint8_t i2c_address = (0xA6 >> 1) );

	/** Create a PCF2131 instance connected to SPI
	 *
	 * @param interface SPI instance
	 */
	PCF2131( SPI& interface );

	/** Destructor */
	virtual ~PCF2131();
	
	/** Initializer 
	 * Clears penginf interrupt
	 */
	void begin( void );

	/** Detector for oscillation stop
	 * 
	 * @return true, if the OSF (Oscillator Stop Flag) is set
	 */
	bool oscillator_stop( void );

	/** time
	 * 
	 * @return time_t value of current time
	 */
	time_t rtc_time( void );

	/** set
	 * 
	 * @param now_tm struct to set calendar and time in RTC
	 */
	void set( struct tm* now_tm );
	
	
	/** Alarm setting
	 * 
	 * @param digit to specify which parameter to set: SECOND, MINUTE, HOUR, DAY, WEEKDAY in 'enum alarm_setting'
	 * @param val Setting value. Set 0x80 to disabling
	 */
	void alarm( alarm_setting digit, int val );

	/** Alarm setting
	 * 
	 * @param digit to specify which parameter to set: SECOND, MINUTE, HOUR, DAY, WEEKDAY in 'enum alarm_setting'
	 * @param val Setting value. Set 0x80 to disabling
	 * @param int_sel Interrupt output selector. ) for INT_A, 1 for INT_B
	 */
	void alarm( alarm_setting digit, int val, int int_sel );

	/** Alarm interrupt disable
	 */
	void alarm_clear( void );

	/** Interrupt clear
	 */
	void alarm_disable( void );
	
	/** Timestamp setting
	 * 
	 * @param num timestamp number: 1~4
	 * @param ts_setting event recording option. Choose LAST or FIRST in 'enum timestanp_setting'
	 * @param int_sel Interrupt output selector. ) for INT_A, 1 for INT_B
	 */
	void timestamp( int num, timestanp_setting ts_setting, int int_sel = 0 );

	/** Getting timestamp info
	 * 
	 * @param num timestamp number: 1~4
	 * @return time_t
	 */
	time_t timestamp( int num );
	
	/** Interrupt clear
	 */
	uint8_t int_clear( void );

	/** Interrupt clear
	 */
	uint8_t int_clear( uint8_t* state_p );

	/** Enabling every second/minute interrupt
	 * 
	 * @param sel choose DISABLE, EVERY_SECOND or EVERY_MINUTE in 'enum periodic_int_select'
	 * @param int_sel Interrupt output selector. ) for INT_A, 1 for INT_B
	 */
	void periodic_interrupt_enable( periodic_int_select sel, int int_sel = 0 );

private:
	const int int_mask_reg[ 2 ][ 2 ]	= {
		{ INT_A_MASK1, INT_A_MASK2, },
		{ INT_B_MASK1, INT_B_MASK2, },
	};
	std::unique_ptr<Serial_device>	intfp;
};

/** PCF85063A class
 *	
 *	PCF85063A class driver
 *
 *  @class PCF85063A
 */

class PCF85063_base : public RTC_NXP
{
public:
	/** Name of the PCF85063 registers */
	enum reg_num {
		Control_1, Control_2,
		Offset,
		RAM_byte,
		Seconds, Minutes, Hours, Days, Weekdays, Months, Years,
		Second_alarm, Minute_alarm, Hour_alarm, Day_alarm, Weekday_alarm,
		Timer_value, Timer_mode
	};
	
	/** Constructor */
	PCF85063_base();

	/** Destructor */
	virtual ~PCF85063_base();
	
	/** Initializer but nothing done with this menthod in this version. 
	 * Don't need to call
	 */
	void begin( void );
	
	/** set
	 * 
	 * @param now_tm struct to set calendar and time in RTC
	 */
	void set( struct tm* now_tm );

	/** Detector for oscillation stop
	 * 
	 * @return true, if the OSF (Oscillator Stop Flag) is set
	 */
	bool oscillator_stop( void );
	
	/** Alarm setting
	 * 
	 * @param digit to specify which parameter to set: SECOND, MINUTE, HOUR, DAY, WEEKDAY in 'enum alarm_setting'
	 * @param val Setting value. Set 0x80 to disabling
	 */
	void alarm( alarm_setting digit, int val );

	/** Alarm clearing
	 */
	void alarm_clear( void );

	/** Alarm interrupt disable
	 */
	void alarm_disable( void );

	/** Interrupt clear
	 */
	uint8_t int_clear( void );

	
	/** Timer setting
	 * 
	 * @param period timer interval in second
	 * @return actual timer set value in second
	 */
	float timer( float period );

protected:
	/** rtc_time
	 * 
	 * @return time_t returns RTC time in time_t format
	 */
	time_t rtc_time( void );

	/** Proxy method for interface  (pure virtual method) */
	virtual void _reg_w( uint8_t reg, uint8_t *vp, int len )	= 0;

	/** Proxy method for interface  (pure virtual method) */
	virtual void _reg_r( uint8_t reg, uint8_t *vp, int len )	= 0;

	/** Proxy method for interface  (pure virtual method) */
	virtual void _reg_w( uint8_t reg, uint8_t val )	= 0;

	/** Proxy method for interface  (pure virtual method) */
	virtual uint8_t _reg_r( uint8_t reg )	= 0;

	/** Proxy method for interface  (pure virtual method) */
	virtual void _bit_op8( uint8_t reg, uint8_t mask, uint8_t val )	= 0;
};

class PCF85063A : public PCF85063_base, public I2C_device
{
public:
	/** Create a PCF85063A instance connected to specified I2C pins with specified address
	 *
	 * @param wire TwoWire instance
	 * @param i2c_address I2C-bus address (default: (0xBC>>1))
	 */
	PCF85063A( I2C& interface, uint8_t i2c_address = (0xA2 >> 1) );

	/** Destructor */
	virtual ~PCF85063A();

#if DOXYGEN_ONLY
	/** Name of the PCF85063A registers */
	enum reg_num {
		Control_1, Control_2,
		Offset,
		RAM_byte,
		Seconds, Minutes, Hours, Days, Weekdays, Months, Years,
		Second_alarm, Minute_alarm, Hour_alarm, Day_alarm, Weekday_alarm,
		Timer_value, Timer_mode
	};

	/** time
	 * 
	 *	"time()" in "time.h" compatible method for RTC
	 *
	 * @param tp pointer to time_t variable
	 * @return time_t value of current time
	 */
	time_t time( time_t* tp );

	/** Initializer but nothing done with this menthod in this version. 
	 * Don't need to call
	 */
	void begin( void );
	
	/** set
	 * 
	 * @param now_tm struct to set calendar and time in RTC
	 */
	void set( struct tm* now_tm );

	/** Detector for oscillation stop
	 * 
	 * @return true, if the OSF (Oscillator Stop Flag) is set
	 */
	bool oscillator_stop( void );
	
	/** Alarm setting
	 * 
	 * @param digit to specify which parameter to set: SECOND, MINUTE, HOUR, DAY, WEEKDAY in 'enum alarm_setting'
	 * @param val Setting value. Set 0x80 to disabling
	 */
	void alarm( alarm_setting digit, int val );

	/** Alarm clearing
	 */
	void alarm_clear( void );

	/** Alarm interrupt disable
	 */
	void alarm_disable( void );

	/** Interrupt clear
	 */
	uint8_t int_clear( void );

	
	/** Timer setting
	 * 
	 * @param period timer interval in second
	 * @return actual timer set value in second
	 */
	float timer( float period );
	
	/** Multiple register write
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_w( uint8_t reg_adr, uint8_t *data, int size );

	/** Single register write
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_w( uint8_t reg_adr, uint8_t data );

	/** Multiple register read
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_r( uint8_t reg_adr, uint8_t *data, int size );

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

	/** Register read, 8 bit
	 *
	 * @param reg register index/address/pointer
	 * @return data value
	 */
	uint8_t read_r8( uint8_t reg );

	/** Register overwriting with bit-mask
	 *	
	 *	Register can be updated by bit level
	 *
	 * @param reg register index/address/pointer
	 * @param mask bit-mask to protect overwriting
	 * @param value value to overwrite
	 */
	void bit_op8(  uint8_t reg,  uint8_t mask,  uint8_t value );

#endif	//	DOXYGEN_ONLY

private:
	/** Proxy method for interface */
	void _reg_w( uint8_t reg, uint8_t *vp, int len );

	/** Proxy method for interface */
	void _reg_r( uint8_t reg, uint8_t *vp, int len );

	/** Proxy method for interface */
	void _reg_w( uint8_t reg, uint8_t val );

	/** Proxy method for interface */
	uint8_t _reg_r( uint8_t reg );

	/** Proxy method for interface */
	void _bit_op8( uint8_t reg, uint8_t mask, uint8_t val );
};

class PCF85063TP : public PCF85063A
{
public:
	/** Create a PCF85063TP instance connected to specified I2C pins with specified address
	 *
	 * @param wire TwoWire instance
	 * @param i2c_address I2C-bus address (default: (0xBC>>1))
	 */
	PCF85063TP( I2C& interface, uint8_t i2c_address = (0xA2 >> 1) );

	/** Destructor */
	virtual ~PCF85063TP();
	
#if DOXYGEN_ONLY
	/** Name of the PCF85063TP registers */
	enum reg_num {
		Control_1, Control_2,
		Offset,
		RAM_byte,
		Seconds, Minutes, Hours, Days, Weekdays, Months, Years,
	};
	/** time
	 * 
	 *	"time()" in "time.h" compatible method for RTC
	 *
	 * @param tp pointer to time_t variable
	 * @return time_t value of current time
	 */
	time_t time( time_t* tp );
	
	/** Initializer but nothing done with this menthod in this version. 
	 * Don't need to call
	 */
	void begin( void );
	
	/** set
	 * 
	 * @param now_tm struct to set calendar and time in RTC
	 */
	void set( struct tm* now_tm );

	/** Detector for oscillation stop
	 * 
	 * @return true, if the OSF (Oscillator Stop Flag) is set
	 */
	bool oscillator_stop( void );
	
	/** Multiple register write
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_w( uint8_t reg_adr, uint8_t *data, int size );

	/** Single register write
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_w( uint8_t reg_adr, uint8_t data );

	/** Multiple register read
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_r( uint8_t reg_adr, uint8_t *data, int size );

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

	/** Register read, 8 bit
	 *
	 * @param reg register index/address/pointer
	 * @return data value
	 */
	uint8_t read_r8( uint8_t reg );

	/** Register overwriting with bit-mask
	 *	
	 *	Register can be updated by bit level
	 *
	 * @param reg register index/address/pointer
	 * @param mask bit-mask to protect overwriting
	 * @param value value to overwrite
	 */
	void bit_op8(  uint8_t reg,  uint8_t mask,  uint8_t value );

#endif	//	DOXYGEN_ONLY

};

class PCF85263A : public RTC_NXP, public I2C_device
{
public:
	/** Name of the PCF85263A registers */
	enum reg_num {
		_100th_seconds, Seconds, Minutes, Hours, Days, Weekdays, Months, Years,
		Second_alarm1, Minute_alarm1, Hour_alarm1, Day_alarm1, Month_alarm1,
		Minute_alarm2, Hour_alarm2, Weekday_alarm2,
		Alarm_enables,
		TSR1_seconds, TSR1_minutes, TSR1_hours, TSR1_days, TSR1_months, TSR1_years,
		TSR2_seconds, TSR2_minutes, TSR2_hours, TSR2_days, TSR2_months, TSR2_years,
		TSR3_seconds, TSR3_minutes, TSR3_hours, TSR3_days, TSR3_months, TSR3_years,
		TSR_mode,
		Offset, Oscillator, Battery_switch, Pin_IO, Function,
		INTA_enable, INTB_enable, Flags,
		RAM_byte, WatchDog, Stop_enable, Resets,

		Hours_xx_xx_00 = 0x03, Hours_xx_00_xx, Hours_00_xx_xx, Reserved0, Reserved1, 
		Second_alm1, Minute_alm1, Hr_xx_xx_00_alm1, Hr_xx_00_xx_alm1, Hr_00_xx_xx_alm1, 
		Minute_alm2, Hr_xx_00_alm2, Hr_00_xx_alm2, 
		TSR1_hr_xx_xx_00 = 0x13, TSR1_hr_xx_00_xx, TSR1_hr_00_xx_xx, Reserved2, 
		TSR2_hr_xx_xx_00 = 0x19, TSR2_hr_xx_00_xx, TSR2_hr_00_xx_xx, Reserved3, 
		TSR3_hr_xx_xx_00 = 0x1F, TSR3_hr_xx_00_xx, TSR3_hr_00_xx_xx, Reserved4, 	
	};
	
	/** Alarm setting digit selection descriptor */
	enum alarm_setting_85263A {
		SECOND,
		MINUTE,
		HOUR,
		DAY,
		MONTH,
		MINUTE2,
		HOUR2,
		WEEKDAY2,
	};
	
	/** Periodic interrupt selection descriptor */
	enum periodic_int_select {
		DISABLE,
		EVERY_SECOND,
		EVERY_MINUTE,
	};

	/** INT_A pin setting descriptor */
	enum inta {
		INTA_CLKOUT,
		INTA_BATTERY_MODE_INDICATION,
		INTA_INTTERRUPT,
		INTA_HIGH_Z,
	};

	/** INT_B pin setting descriptor */
	enum intb {
		INTB_DISABLE,
		INTB_INTTERRUPT,
		INTB_CLKOUT,
		INTB_INPUT_MODE,
	};

	/** Timestamp pin input setting descriptor */
	enum ts_in {
		TS_PULLUP_80K	= 0x00,
		TS_PULLUP_40K	= 0x40,
		TSL_ACTIVE_HIGH	= 0x00,
		TSL_ACTIVE_LOW	= 0x20,
		TSIM_CMOS		= 0x00,
		TSIM_MECHANICAL	= 0x10,
	};

	/** Interrupt output selector */
	enum int_out_sel {
		INT_A,
		INT_B,
	};
	/** Create a PCF85263A instance connected to specified I2C pins with specified address
	 *
	 * @param wire TwoWire instance
	 * @param i2c_address I2C-bus address (default: (0xBC>>1))
	 */
	PCF85263A( I2C& interface, uint8_t i2c_address = (0xA2 >> 1) );

	/** Destructor */
	virtual ~PCF85263A();
	
	/** Initializer but nothing done with this menthod in this version. 
	 * Don't need to call
	 */
	void begin( void );
	
	/** set
	 * 
	 * @param now_tm struct to set calendar and time in RTC
	 */
	void set( struct tm* now_tm );

	/** Detector for oscillation stop
	 * 
	 * @return true, if the OSF (Oscillator Stop Flag) is set
	 */
	bool oscillator_stop( void );
	
	/** Alarm setting
	 * 
	 * @param digit to specify which parameter to set: SECOND, MINUTE, HOUR, DAY, WEEKDAY in 'enum alarm_setting'
	 * @param val Setting value. Set 0x80 to disabling
	 */
	void alarm( alarm_setting digit, int val );

	/** Alarm setting
	 * 
	 * @param digit to specify which parameter to set: SECOND, MINUTE, HOUR, DAY, WEEKDAY in 'enum alarm_setting'
	 * @param val Setting value. Set 0x80 to disabling
	 * @param int_sel Interrupt output selector. ) for INT_A, 1 for INT_B
	 */
	void alarm( alarm_setting digit, int val, int int_sel );

	/** Alarm setting
	 * 
	 * @param digit to specify which parameter to set: SECOND, MINUTE, HOUR, DAY, WEEKDAY in 'enum alarm_setting'
	 * @param val Setting value. Set 0x80 to disabling
	 */
	void alarm( alarm_setting_85263A digit, int val, int int_sel = 0 );

	/** Alarm clearing
	 */
	void alarm_clear( void );

	/** Alarm interrupt disable
	 */
	void alarm_disable( void );

	/** Interrupt clear
	 */
	uint8_t int_clear( void );

	/** rtc_time
	 * 
	 * @return time_t returns RTC time in time_t format
	 */
	time_t rtc_time( void );

	/** Enabling every second/minute interrupt
	 * 
	 * @param sel choose DISABLE, EVERY_SECOND or EVERY_MINUTE in 'enum periodic_int_select'
	 */
	void periodic_interrupt_enable( periodic_int_select sel, int int_sel = 0 );

	/** Pin configuration
	 * 
	 * @param cfg_a To choose INT_A pin congiguration: Use CLKOUT, BATTERY_MODE_INDICATION, INTTERRUPT or HIGH_Z
	 * @param cfg_b To choose INT_A pin congiguration: Use DISABLE, INTTERRUPT, CLKOUT or INPUTMODE
	 */
	void pin_congfig(inta cfg_a, intb cfg_b);
	
	/** Timestamp pin configuration
	 * 
	 * @param setting To choose options: Compose value by ORing next constants: TS_PULLUP_80K, TS_PULLUP_40K, TSL_ACTIVE_HIGH, TSL_ACTIVE_LOW, TSIM_CMOS and TSIM_MECHANICAL
	 */
	void ts_congfig(int setting);
	
	/** Timestamp register read
	 * 
	 * @param num timestamp index
	 */
	time_t timestamp( int num );

#if DOXYGEN_ONLY
	/** time
	 * 
	 *	"time()" in "time.h" compatible method for RTC
	 *
	 * @param tp pointer to time_t variable
	 * @return time_t value of current time
	 */
	time_t time( time_t* tp );
	
	/** Multiple register write
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_w( uint8_t reg_adr, uint8_t *data, int size );

	/** Single register write
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_w( uint8_t reg_adr, uint8_t data );

	/** Multiple register read
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_r( uint8_t reg_adr, uint8_t *data, int size );

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

	/** Register read, 8 bit
	 *
	 * @param reg register index/address/pointer
	 * @return data value
	 */
	uint8_t read_r8( uint8_t reg );

	/** Register overwriting with bit-mask
	 *	
	 *	Register can be updated by bit level
	 *
	 * @param reg register index/address/pointer
	 * @param mask bit-mask to protect overwriting
	 * @param value value to overwrite
	 */
	void bit_op8(  uint8_t reg,  uint8_t mask,  uint8_t value );

#endif	//	DOXYGEN_ONLY

private:
	void set_alarm( int digit, int val, int int_sel );
};

class PCF85053A : public RTC_NXP, public I2C_device
{
public:
	/** Name of the PCF85053A registers */
	enum reg_num {
		Seconds, Seconds_alarm, Minutes, Minutes_alarm, Hours, Hours_alarm, 
		Day_of_the_Week, Day_of_the_Month, Month, Year,
		Control_Register, Status_Register,
		CLKOUT_Control,
		_2nd_Control_Register,
		Scratchpad, Version_Register, 
		Vendor_ID_Register, Model_Register,
		Offset, Oscillator, 
		Access_config, 
		Sec_timestp, Min_timestp, Hour_timestp, DayWk_timestp, DayMon_timestp, Mon_timestp, Year_timestp, 
		R_code1, R_code2	
	};
	
	/** Create a PCF85053A instance connected to specified I2C pins with specified address
	 *
	 * @param wire TwoWire instance
	 * @param i2c_address I2C-bus address (default: (0xBC>>1))
	 */
	PCF85053A( I2C& interface, uint8_t i2c_address = (0xDE >> 1) );

	/** Destructor */
	virtual ~PCF85053A();
	
	/** Initializer but nothing done with this menthod in this version. 
	 * Don't need to call
	 */
	void begin( void );
	
	/** set
	 * 
	 * @param now_tm struct to set calendar and time in RTC
	 */
	void set( struct tm* now_tm );

	/** Detector for oscillation stop
	 * 
	 * @return true, if the OSF (Oscillator Stop Flag) is set
	 */
	bool oscillator_stop( void );
	
	/** Alarm setting
	 * 
	 * @param digit to specify which parameter to set: SECOND, MINUTE, HOUR in 'enum alarm_setting_85053A'
	 * @param val Setting value. Set 0x80 to disabling
	 */
	void alarm( alarm_setting digit, int val );

	/** Alarm clearing
	 */
	void alarm_clear( void );

	/** Alarm interrupt disable
	 */
	void alarm_disable( void );

	/** Interrupt clear
	 */
	uint8_t int_clear( void );

	/** rtc_time
	 * 
	 * @return time_t returns RTC time in time_t format
	 */
	time_t rtc_time( void );

#if DOXYGEN_ONLY
	/** time
	 * 
	 *	"time()" in "time.h" compatible method for RTC
	 *
	 * @param tp pointer to time_t variable
	 * @return time_t value of current time
	 */
	time_t time( time_t* tp );
	
	/** Multiple register write
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_w( uint8_t reg_adr, uint8_t *data, int size );

	/** Single register write
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_w( uint8_t reg_adr, uint8_t data );

	/** Multiple register read
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_r( uint8_t reg_adr, uint8_t *data, int size );

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

	/** Register read, 8 bit
	 *
	 * @param reg register index/address/pointer
	 * @return data value
	 */
	uint8_t read_r8( uint8_t reg );

	/** Register overwriting with bit-mask
	 *	
	 *	Register can be updated by bit level
	 *
	 * @param reg register index/address/pointer
	 * @param mask bit-mask to protect overwriting
	 * @param value value to overwrite
	 */
	void bit_op8(  uint8_t reg,  uint8_t mask,  uint8_t value );

#endif	//	DOXYGEN_ONLY
	
private:
	//static inline constexpr uint8_t alarm_offsets[]	= { 1, 3, 5 };
	static int alarm_offsets[ 3 ];
};

class ForFutureExtention : public RTC_NXP, public I2C_device
{
public:
	/** Constructor */
	ForFutureExtention();

	/** Destructor */
	virtual ~ForFutureExtention();
	
	/** Initializer but nothing done with this menthod in this version. 
	 * Don't need to call
	 */
	void begin( void );
	
	/** set
	 * 
	 * @param now_tm struct to set calendar and time in RTC
	 */
	void set( struct tm* now_tm );

	/** Detector for oscillation stop
	 * 
	 * @return true, if the OSF (Oscillator Stop Flag) is set
	 */
	bool oscillator_stop( void );
	
	/** Alarm setting
	 * 
	 * @param digit to specify which parameter to set: SECOND, MINUTE, HOUR, DAY, WEEKDAY in 'enum alarm_setting'. Set 0x80 to disabling
	 */
	void alarm( alarm_setting digit, int val );

	/** Alarm clearing
	 */
	void alarm_clear( void );

	/** Alarm interrupt disable
	 */
	void alarm_disable( void );

	/** Interrupt clear
	 */
	uint8_t int_clear( void );

	
#if DOXYGEN_ONLY
	/** time
	 * 
	 *	"time()" in "time.h" compatible method for RTC
	 *
	 * @param tp pointer to time_t variable
	 * @return time_t value of current time
	 */
	time_t time( time_t* tp );
	
	/** Multiple register write
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_w( uint8_t reg_adr, uint8_t *data, int size );

	/** Single register write
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_w( uint8_t reg_adr, uint8_t data );

	/** Multiple register read
	 * 
	 * @param reg register index/address/pointer
	 * @param data pointer to data buffer
	 * @param size data size
	 * @return transferred data size
	 */
	void reg_r( uint8_t reg_adr, uint8_t *data, int size );

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

	/** Register read, 8 bit
	 *
	 * @param reg register index/address/pointer
	 * @return data value
	 */
	uint8_t read_r8( uint8_t reg );

	/** Register overwriting with bit-mask
	 *	
	 *	Register can be updated by bit level
	 *
	 * @param reg register index/address/pointer
	 * @param mask bit-mask to protect overwriting
	 * @param value value to overwrite
	 */
	void bit_op8(  uint8_t reg,  uint8_t mask,  uint8_t value );

#endif	//	DOXYGEN_ONLY

protected:
	/** rtc_time
	 * 
	 * @return time_t returns RTC time in time_t format
	 */
	time_t rtc_time( void );
};


#endif //	ARDUINO_RTC_DRIVER_NXP_ARD_H
