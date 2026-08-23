/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

/** I3C class
 *
 *  @class I3C
 *
 *	A master-mode I3C driver, derived from I2C. Supports native I3C-SDR
 *	transactions (dynamic addressing via DAA, CCC broadcast/direct
 *	commands, IBI) as well as running the same physical bus in legacy
 *	I2C mode (see mode()) for targets that don't speak I3C.
 */

#ifndef R01LIB_I3C_H
#define R01LIB_I3C_H

#include "r01lib.h"

#ifdef I3C_SUPPORTED

#include	"i2c.h"
#include	"fsl_i3c.h"

/** when defined, reg_write()/reg_read() use their own direct
 *  register-addressed transfer path (reg_xfer()) instead of composing a
 *  register-address byte into a plain write()/read() the way I2C's base
 *  implementation does.
 */
#define	CUSTOM_REGISTAR_XFER

/** constants for the I3C Common Command Codes (CCC) used by ccc_broadcast()/
 *  ccc_set()/ccc_get(); see the MIPI I3C specification for the full set and
 *  their semantics.
 */
enum CCC
{
	BROADCAST_ENEC		= 0x00,
	BROADCAST_RSTDAA	= 0x06,
	BROADCAST_ENTDAA	= 0x07,
	DIRECT_ENEC			= 0x80,
	DIRECT_DICEC		= 0x81,
	DIRECT_SETDASA		= 0x87,
	DIRECT_SETNEWDA		= 0x88,
	DIRECT_GETPID		= 0x8D,
	DIRECT_GETBCR		= 0x8E,
	DIRECT_GETDCR		= 0x8F,
	DIRECT_GETSTATUS	= 0x90,
	DIRECT_RSTACT		= 0x90
};

typedef void (*i3c_func_ptr)(void); 

class I3C : public I2C
{
public:
	/** constants for mode setting  */
	enum MODE
	{
		I3C_MODE	= kI3C_TypeI3CSdr,
		I2C_MODE	= kI3C_TypeI2C,
		I3CDDR_MODE	= kI3C_TypeI3CDdr
	};
	
	/** constants for SCL frequency settings  */
	enum FREQ
	{
		OD_FREQ					= 4000000UL,
		PP_FREQ					= 12500000UL,
		DEFAULT_FREQ_SETTING	= 0
	};
	
	/** constants for miscellaneous setting  */
	enum MISC
	{
		BROADCAST_ADDR	= 0x7E,
		PID_LENGTH		= 6
	};

	/** Create an I3C instance with specified pins
	 *
	 * @param sda         pin number to connect SDA
	 * @param scl         pin number to connect SCL
	 * @param i2c_freq    (optional) SCL frequency in Hz for I2C operation
	 * @param i3c_od_freq (optional) SCL frequency in Hz for I3C open-drain operation
	 * @param i3c_pp_freq (optional) SCL frequency in Hz for I3C push-pull operation
	 */
	I3C( int sda, int scl, uint32_t i2c_freq = I2C::FREQ, uint32_t i3c_od_freq = OD_FREQ, uint32_t i3c_pp_freq = PP_FREQ );

	/** Destructor to free I3C resource
	 */
	virtual ~I3C();

	using I2C::frequency;	// I3C's own frequency() overloads below have different
							// signatures from I2C::frequency(uint32_t), which would
							// otherwise hide it from lookup through an I3C object

	/** Frequency settings
	 *
	 * @param i2c_freq    (optional) SCL frequency in Hz for I2C operation
	 * @param i3c_od_freq (optional) SCL frequency in Hz for I3C open-drain operation
	 * @param i3c_pp_freq (optional) SCL frequency in Hz for I3C push-pull operation
	 *
	 *  @note use zero or DEFAULT_FREQ_SETTING to leave a given rate at its default
	 */
	virtual void	frequency( uint32_t i2c_freq, uint32_t i3c_od_freq, uint32_t i3c_pp_freq );

	/** All frequency settings (I2C, I3C open-drain, I3C push-pull) reverted to default
	 */
	virtual void	frequency( void );

	/** mode setting
	 *	I3C bus is configured to I3C-SDR, I3C-DDR or I2C
	 *
	 * @param mode I3C_MODE, I2C_MODE or I3CDDR_MODE
	 */
	virtual void 	mode( MODE mode );

	using I2C::write;	// I3C only overrides the buffer-write/-read overloads
	using I2C::read;	// below; I2C's single-byte write(uint8_t,uint8_t,bool)/
						// read(uint8_t,bool) would otherwise be hidden

	/** write transaction
	 *
	 * @param targ target address
	 * @param dp data to write
	 * @param length data length
	 * @param stop (option) generate STOP condition: "false" to make repeated-start in next transaction
	 * @return status_t
	 */
	virtual status_t	write( uint8_t targ, const uint8_t *dp, int length, bool stop = STOP );	

	/** read transaction
	 *
	 * @param targ target address
	 * @param dp data buffer for read
	 * @param length data length
	 * @param stop (option) generate STOP condition: "false" to make repeated-start in next transaction
	 * @return status_t
	 */
	virtual status_t	read( uint8_t targ, uint8_t *dp, int length, bool stop = STOP );
	
#ifdef	CUSTOM_REGISTAR_XFER
	using I2C::reg_write;	// see the frequency() using-declaration above -- same
	using I2C::reg_read;	// reasoning, I3C's own overloads below add a `stop` param

	/** Register write (multiple byte data)
	 *	provides interface for register write
	 *
	 * @param targ target address
	 * @param reg register address
	 * @param dp data to write
	 * @param length data length
	 * @param stop currently ignored -- reg_xfer() always uses its own default (STOP); kept for API-shape parity with I2C::reg_write()
	 * @return status_t
	 */
	virtual status_t	reg_write( uint8_t targ, uint8_t reg, const uint8_t *dp, int length, bool stop = STOP );

	/** Register read (multiple byte data)
	 *	provides interface for register read
	 *
	 * @param targ target address
	 * @param reg register address
	 * @param dp buffer to receive the read data
	 * @param length data length
	 * @param stop currently ignored -- reg_xfer() always uses its own default (STOP); kept for API-shape parity with I2C::reg_read()
	 * @return status_t
	 */
	virtual status_t	reg_read( uint8_t targ, uint8_t reg, uint8_t *dp, int length, bool stop = STOP );
#endif	// CUSTOM_REGISTAR_XFER
	
	/** check IBI status
	 *  	Non-blocking poll of whether an In-Band Interrupt has occurred
	 *  	since the last call. Clears the pending flag on read.
	 *
	 * @return target address of IBI initiated device or zero if no event happened
	 */
	virtual uint8_t		check_IBI( void );

	/** set IBI callback function
	 *  	Registered function is invoked from master_ibi_callback() (the
	 *  	SDK-facing IBI handler) whenever an IBI is serviced.
	 *
	 * @param fp pointer to the function to call, or nullptr to disable
	 */
	virtual void		set_IBI_callback( i3c_func_ptr fp );

	/** CCC broadcast
	 *  	Sends a Common Command Code to the broadcast address (0x7E),
	 *  	optionally followed by data bytes.
	 *
	 * @param ccc CCC command
	 * @param dp data to send along with the command (may be nullptr if length is 0)
	 * @param length data length
	 * @param first_time (option) force the one-time open-drain-frequency priming step ccc_broadcast() otherwise only does automatically on this instance's first broadcast
	 * @return status_t
	 */
	virtual status_t	ccc_broadcast( uint8_t ccc, const uint8_t *dp, uint8_t length, bool first_time = false );

	/** CCC direct set
	 *  	Broadcasts the CCC command, then writes a single data byte
	 *  	directly to addr.
	 *
	 * @param ccc CCC command
	 * @param addr target address
	 * @param data single byte data to write
	 * @return status_t
	 */
	virtual status_t	ccc_set( uint8_t ccc, uint8_t addr, uint8_t data );

	/** CCC direct get
	 *  	Broadcasts the CCC command, then reads data directly from addr.
	 *
	 * @param ccc CCC command
	 * @param addr target address
	 * @param dp buffer to receive data
	 * @param length data length
	 * @return status_t
	 */
	virtual status_t	ccc_get( uint8_t ccc, uint8_t addr, uint8_t *dp, uint8_t length );

	/** perform DAA procedure
	 *  	Runs Dynamic Address Assignment, handing out addresses from
	 *  	address_list to any devices on the bus that don't have one yet.
	 *
	 * @param address_list candidate address list to assign from
	 * @param list_length address list length
	 * @param device_list out-parameter: set to the SDK's own static array of assigned-device info
	 * @return int for number of devices which has newly assigned addresses (max 10)
	 */
	virtual int			DAA( const uint8_t *address_list, uint8_t list_length, i3c_device_info_t** device_list );

	/** SDK IBI (In-Band Interrupt) event callback, registered with the
	 *  MCUXpresso SDK I3C driver at construction time. Records the
	 *  interrupting device's address for check_IBI(), buffers any IBI
	 *  payload data, and invokes the user callback set via
	 *  set_IBI_callback(), if any. Not intended to be called directly.
	 *
	 * @param base I3C peripheral base address (SDK callback signature)
	 * @param handle SDK master transfer handle
	 * @param ibiType kind of IBI event
	 * @param ibiState IBI handler state (e.g. whether a data buffer is needed)
	 */
	static void		master_ibi_callback( I3C_Type *base, i3c_master_handle_t *handle, i3c_ibi_type_t ibiType, i3c_ibi_state_t ibiState );

	/** SDK transfer-complete callback, registered with the MCUXpresso SDK
	 *  I3C driver at construction time. Records the outcome of an
	 *  asynchronous transfer. Not intended to be called directly.
	 *
	 * @param base I3C peripheral base address (SDK callback signature)
	 * @param handle SDK master transfer handle
	 * @param status transfer completion status
	 * @param userData user data pointer (unused)
	 */
	static void		master_callback( I3C_Type *base, i3c_master_handle_t *handle, status_t status, void *userData );

protected:
	using	I2C::ping;
	using	I2C::scan;

private:
	status_t	xfer( i3c_direction_t dir, i3c_bus_type_t type, uint8_t targ, uint8_t *dp, int length, bool stop = STOP );

#ifdef	CUSTOM_REGISTAR_XFER
	status_t 	reg_xfer( i3c_direction_t dir, i3c_bus_type_t type, uint8_t targ, uint8_t reg, uint8_t reg_length, uint8_t *dp, int length, bool stop = STOP );
#endif	// CUSTOM_REGISTAR_XFER

	i3c_bus_type_t								bus_type;
	static const i3c_master_transfer_callback_t	masterCallback;
	i3c_master_config_t							masterConfig;
	bool										first_broadcast;
};
#else	// I3C_SUPPORTED
#endif	// I3C_SUPPORTED

#endif	// R01LIB_I3C_H
