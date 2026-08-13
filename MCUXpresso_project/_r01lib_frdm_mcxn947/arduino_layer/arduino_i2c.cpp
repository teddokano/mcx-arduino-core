/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"
#include	"i3c.h"
#include	"arduino_i2c.h"

TwoWire	Wire(  I2C_SDA, I2C_SCL );
TwoWire	Wire1( I3C_SDA, I3C_SCL );

TwoWire::TwoWire( int sda_pin, int scl_pin ) : _sda( sda_pin ), _scl( scl_pin ), i2c( nullptr ){}

void TwoWire::begin( int baud )
{
	baudrate	= baud;

	bool	is_i3c	= ( I3C_SDA == _sda ) && ( I3C_SCL == _scl );

	if ( !i2c )
	{
		if ( is_i3c )
		{
			I3C	*i3c;
			i3c	= new I3C( _sda, _scl );

#ifdef	CPU_MCXN947VDF
			/*
			 *  At least one native-I3C-mode bus operation needs to happen on
			 *  this bus before switching it down to legacy I2C_MODE -- on
			 *  FRDM-MCXN947, without this, I2C-mode reads on this bus
			 *  reliably NAK on the address phase (writes still succeed).
			 *  Confirmed via a controlled A/B test on real hardware (GDB,
			 *  then reflashed with/without this line): consistently fails
			 *  without it, consistently works with it. Root cause not fully
			 *  understood -- per the I3C spec the master's pull-up assist
			 *  should already be active during bus-free/START/address-header
			 *  regardless of prior traffic, so this looks like an SDK/
			 *  silicon init-ordering quirk (I3C_MasterInit() alone may not
			 *  be enough to arm the pull-up-assist state machine; one real
			 *  START/STOP cycle might be needed first) rather than a spec
			 *  violation we're intentionally working around. This priming
			 *  broadcast is expected to itself fail (kStatus_I3C_WriteAbort,
			 *  nothing has a dynamic address yet) -- only the bus activity
			 *  matters. Matches the flow NXP's own P3T1755_FRDM_MCXN947_demo_DAA
			 *  reference example always does (RSTDAA before any I2C-mode
			 *  access).
			 *
			 *  Confirmed NOT needed on A153 (its I3C_SDA/I3C_SCL are the
			 *  same physical pins as the general-purpose Arduino I2C
			 *  connector, which has real populated pull-up resistors --
			 *  unlike N947's dedicated onboard-sensor-only I3C bus, whose
			 *  fixed pull-ups are DNP on the schematic). Scoped to just
			 *  this chip rather than done unconditionally, so boards that
			 *  don't need it don't get a mystery extra I2C-bus transaction
			 *  on every Wire.begin().
			 */
			i3c->ccc_broadcast( CCC::BROADCAST_RSTDAA, nullptr, 0 );
#endif	// CPU_MCXN947VDF

			i3c->mode( I3C::MODE::I2C_MODE );
			i2c	= i3c;
		}
		else
		{
			i2c	= new I2C( _sda, _scl );
		}
	}

	/*
	 *  I2C(sda, scl, no_hw=true) — the base-class constructor I3C delegates
	 *  to — skips hardware init entirely (`if (no_hw) return;`), leaving
	 *  I2C::unit_base uninitialized. I3C's own
	 *  frequency(uint32_t,uint32_t,uint32_t)/frequency(void) only hide, not
	 *  override, I2C::frequency(uint32_t) (different signature), so calling
	 *  the generic i2c->frequency(baudrate) for an I3C instance would
	 *  dispatch to I2C::frequency() and dereference that uninitialized
	 *  unit_base -> BusFault. Route to I3C's own overload instead.
	 */
	if ( is_i3c )
		static_cast<I3C *>( i2c )->frequency( baudrate, 0, 0 );
	else
		i2c->frequency( baudrate );

	i2c->err_callback( nullptr );
}

void TwoWire::end( void )
{
	delete i2c;
	i2c	= nullptr;
}

void TwoWire::setClock( uint32_t freq )
{
	if ( !i2c )
		return;	// no-op before begin() -- nothing to reconfigure yet

	baudrate	= (int)freq;

	bool	is_i3c	= ( I3C_SDA == _sda ) && ( I3C_SCL == _scl );

	if ( is_i3c )
		static_cast<I3C *>( i2c )->frequency( baudrate, 0, 0 );
	else
		i2c->frequency( baudrate );
}

void TwoWire::beginTransmission( const uint8_t address )
{
	targ_addr		= address;
	data_buf_index	= 0;
}

size_t TwoWire::write( uint8_t data )
{
	data_buf[ data_buf_index++ ]	= data;

	return	data_buf_index;
}

size_t TwoWire::write( const uint8_t *data, size_t length )
{
    memcpy( &data_buf[ data_buf_index ], data, length );
	data_buf_index += length;

	return	data_buf_index;
}

uint8_t TwoWire::endTransmission( bool stop )
{
	return	i2c->write( targ_addr, data_buf, data_buf_index, stop );
}

uint8_t	TwoWire::requestFrom( const uint8_t address, const size_t length, bool stop )
{
	data_buf_index	= 0;
	read_size		= length;

	return	(i2c->read( address, data_buf, length )) ? 0 : length;
}

int TwoWire::available( void )
{
	return (int)(read_size - data_buf_index);
}

int TwoWire::read( void )
{
	if ( data_buf_index >= read_size )
		return -1;
	return	data_buf[ data_buf_index++ ];
}
