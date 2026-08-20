/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef R01LIB_I2C_H
#define R01LIB_I2C_H

#include	<string.h>

#ifdef	CPU_MCXC444VLH
#include "fsl_i2c.h"
#else
#include "fsl_lpi2c.h"
#endif

#include	"obj.h"
#include	"io.h"

/** I2C class
 *
 *  @class I2C
 *
 *	A master-mode I2C driver (register read/write helpers, raw
 *	transactions, and a bus scan). I3C derives from this class to reuse
 *	the same API surface while running the physical bus in I3C mode.
 */

/** size (in bytes) of the internal scratch buffer reg_write() uses to
 *  prepend a register address to the data being written. Limits the
 *  maximum single reg_write() payload to REG_RW_BUFFER_SIZE - 1 bytes.
 */
#define	REG_RW_BUFFER_SIZE	10

class I2C: public Obj
{
public:

	/** defining pointer to NAK callback	*/
	typedef void (*err_cb_ptr)( status_t status, uint8_t address );

	/** constants for STOP-condition setting  */
	enum STOP_CONDITION
	{
		STOP	= true,
		NO_STOP	= false
	};

	/** constants for SCL frequency settings  */
	enum FREQ
	{
		FREQ	= 400000UL
	};
		
	/** Create an I2C instance with specified pins
	 *
	 * @param sda pin number to connect SDA
	 * @param scl pin number to connect SCL
	 * @param no_hw (option) flag for I3C. I3C constructor will set this flag to avoid set hardware for both I3C and I2C
	 */
	I2C( int sda, int scl, bool no_hw = false );

	/** Destractor to freeing I3C resource
	 */
	virtual ~I2C();
	
	/** set SCL frequency
	 *
	 * @param frequency integer value in "Hz"
	 */
	virtual void		frequency( uint32_t frequency );

	/** Pull-up enable
	 *
	 * @param enable bool for enabling internal pull-up resister
	 */
	virtual void		pullup( bool enable );

	/** Register write (multiple byte data)
	 *	provideds interface for register write
	 *	
	 * @param targ target address
	 * @param reg register address
	 * @param dp data to write
	 * @param length data length
	 * @return status_t
	 */
	virtual status_t	reg_write( uint8_t targ, uint8_t reg, const uint8_t *dp, int length );

	/** Register write (single byte data)
	 *	provideds interface for register write
	 *	
	 * @param targ target address
	 * @param reg register address
	 * @param data data to write
	 * @return status_t
	 */
	virtual status_t	reg_write( uint8_t targ, uint8_t reg, uint8_t data );
	
	/** Register read (multiple byte data)
	 *	provides interface for register read
	 *
	 * @param targ target address
	 * @param reg register address
	 * @param dp buffer to receive the read data
	 * @param length data length
	 * @return status_t
	 */
	virtual status_t	reg_read( uint8_t targ, uint8_t reg, uint8_t *dp, int length );

	/** Register read (single byte data)
	 *	provideds interface for register read
	 *	returns a single byte data
	 *	status can be checked by last_status variable
	 *	
	 * @param targ target address
	 * @param reg register address
	 * @return read data
	 */
	virtual uint8_t		reg_read( uint8_t targ, uint8_t reg );
	
	/** write transaction (multiple byte data)
	 *
	 * @param address target address
	 * @param dp data to write
	 * @param length data length
	 * @param stop (option) generate STOP condition: "false" to make repeated-start in next transaction
	 * @return status_t
	 */
	virtual status_t	write( uint8_t address, const uint8_t *dp, int length, bool stop = STOP );

	/** write transaction (single byte data)
	 *
	 * @param targ target address
	 * @param data data to write
	 * @param stop (option) generate STOP condition: "false" to make repeated-start in next transaction
	 * @return status_t
	 */
	virtual status_t	write( uint8_t targ, uint8_t data, bool stop = STOP );

	/** read transaction (multiple byte data)
	 *
	 * @param address target address
	 * @param dp buffer to receive the read data
	 * @param length data length
	 * @param stop (option) generate STOP condition: "false" to make repeated-start in next transaction
	 * @return status_t
	 */
	virtual status_t	read( uint8_t address, uint8_t *dp, int length, bool stop = STOP );

	/** read transaction (single byte data)
	 *	provideds interface for register read
	 *	returns a single byte data
	 *	status can be checked by last_status variable
	 *
	 * @param targ target address
	 * @param stop (option) generate STOP condition: "false" to make repeated-start in next transaction
	 * @return data
	 */
	virtual uint8_t		read( uint8_t targ, bool stop = STOP );

	/** registering error handling method
	 *
	 * @param callback pointer to error handling method. use "nullptr" to suppress any actions
	 * @return the previously-registered callback
	 */
	virtual err_cb_ptr	err_callback( err_cb_ptr callback );

	/** default error handling callback method
	 * 		this method will be called when I2C process got an error
	 * 		the callback can be installed by err_callback method
	 *
	 * @param error error status code
	 * @param address target address
	 */
	static void 		err_handling( status_t error, uint8_t address );

	/** ping
	 *		check device returns ACK
	 *
	 * @param addr	target address
	 * @return true if ACKs
	 */
	virtual bool		ping( uint8_t addr );

	/** device scan
	 *		device scan result will be stored in *bool
	 *
	 * @param start	scan start address
	 * @param last	scan last address
	 * @param result buffer (indexed by address, 0..last) to receive true/false per address
	 */
	virtual void		scan( uint8_t start, uint8_t last, bool *result );

	/** device scan
	 * 		device scan result will be shown on the screen
	 * 		scan range can be specified by start and last parameters
	 *
	 * @param start	scan start address
	 * @param last	scan last address
	 */
	virtual void		scan( uint8_t start, uint8_t last );

	/** device scan
	 * 		device scan result will be shown on the screen
	 * 		scan range can be specified and last parameter
	 *
	 * @param last	scan last address
	 */
	virtual void		scan( uint8_t last = 124 );

	/** method for I3C class compatibility (no-op stub on the plain-I2C base class)
	 * Does nothing but return kStatus_Success. This lets device driver code
	 * written against CCC commands run unmodified on plain I2C -- I3C
	 * overrides this method to actually perform the CCC broadcast/direct
	 * write.
	 *
	 * @param ccc CCC command
	 * @param addr target address
	 * @param data single byte data
	 * @return kStatus_Success
	 */
	virtual status_t	ccc_set( uint8_t ccc, uint8_t addr, uint8_t data );

	/** method for I3C class compatibility (no-op stub on the plain-I2C base class)
	 * Does nothing but zero-fill the receive buffer and return kStatus_Success.
	 * This lets device driver code written against CCC commands run
	 * unmodified on plain I2C -- I3C overrides this method to actually
	 * perform the CCC broadcast/direct read.
	 *
	 * @param ccc CCC command
	 * @param addr target address
	 * @param dp buffer to receive data
	 * @param length data length
	 * @return status_t
	 */
	virtual status_t	ccc_get( uint8_t ccc, uint8_t addr, uint8_t *dp, uint8_t length );

	/** variable for reporting last state */
	status_t				last_status;

protected:
	/** low-level write transaction, without err_callback dispatch on failure.
	 *  write()'s implementation, and I3C's I2C_MODE override point.
	 *
	 * @param address target address
	 * @param dp data to write
	 * @param length data length
	 * @param stop (option) generate STOP condition: "false" to make repeated-start in next transaction
	 * @return status_t
	 */
	virtual status_t	write_core( uint8_t address, const uint8_t *dp, int length, bool stop = STOP );

	/** low-level read transaction, without err_callback dispatch on failure.
	 *  read()'s implementation, and I3C's I2C_MODE override point.
	 *
	 * @param address target address
	 * @param dp buffer to receive the read data
	 * @param length data length
	 * @param stop (option) generate STOP condition: "false" to make repeated-start in next transaction
	 * @return status_t
	 */
	virtual status_t	read_core( uint8_t address, uint8_t *dp, int length, bool stop = STOP );

private:
#if	CPU_MCXC444VLH
	i2c_master_config_t		masterConfig;
	I2C_Type				*unit_base;
	bool					repeated_start_required_flag;
#else
	lpi2c_master_config_t	masterConfig;
	LPI2C_Type				*unit_base;
#endif
	DigitalInOut			_sda;
	DigitalInOut			_scl;
	err_cb_ptr				err_cb;
	bool					_no_hw;
};

#endif // R01LIB_I2C_H
