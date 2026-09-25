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
#ifdef	CPU_MCXN947VDF
TwoWire	Wire2( MB_SDA,  MB_SCL );
#endif

/*
 *  Target (slave) mode, on the SDK's LPI2C_Slave* transfer API.
 *
 *  An LPI2C's master and slave halves run independently on the same pins,
 *  so a board can be a controller and a target at once, even addressing
 *  its own target. Four things the SDK leaves to the caller, each seen on
 *  hardware:
 *
 *  - FRDM-MCXA153's SDK (fsl_lpi2c 2.5.4) turns the master off when it
 *    arms the slave (LPI2C_SlaveTransferNonBlocking(): "Enable the slave
 *    function and disable the master function"); FRDM-MCXN947's (2.2.4)
 *    doesn't. The master is turned back on after arming, or every later
 *    controller transfer waits forever on a disabled master.
 *  - When the reply a controller reads runs out, the SDK sends nothing and
 *    the slave stretches SCL forever. 0xFF is sent from there on, as AVR
 *    does.
 *  - A write longer than the buffer: the byte that doesn't fit is NAKed
 *    (ACKSTALL, deciding each ACK in the TransmitAck event), and still
 *    read into a scratch byte, since an unread byte leaves the receive
 *    flag set and the interrupt firing.
 *  - Deleting the master (end(), or the reset after a timeout) gates the
 *    LPI2C's clock (A153) or de-initializes its FlexComm (N947), which
 *    takes the slave with it, so start() re-arms the target after
 *    re-creating the master.
 */
struct WireTarget
{
	lpi2c_slave_handle_t	handle;
	uint8_t		address;
	bool		active;
	bool		reading;	// the controller is reading, not writing
	bool		rx_given;	// rx[] handed to the SDK for this write
	bool		rx_full;	// ... and it filled up
	bool		tx_given;	// onRequest() already asked for this read
	bool		in_request;	// inside onRequest(): write() fills tx[]
	uint8_t		rx[ WIRE_BUFFER_SIZE ];		// the SDK receives into this
	uint8_t		received[ WIRE_BUFFER_SIZE ];	// read() reads a finished write here
	uint8_t		tx[ WIRE_BUFFER_SIZE ];
	size_t		tx_size;
	uint8_t		fill;		// what a controller gets past the reply
	uint8_t		sink;		// where a NAKed byte is read to
};

static void target_callback( LPI2C_Type *, lpi2c_slave_transfer_t *xfer, void *self )
{
	static_cast<TwoWire *>( self )->target_event( xfer );
}

TwoWire::TwoWire( int sda_pin, int scl_pin )
	: _sda( sda_pin ), _scl( scl_pin ), i2c( nullptr ),
	  tx_size( 0 ), rx_data( rx_buf ), rx_index( 0 ), rx_size( 0 ),
	  target( nullptr ), user_onReceive( nullptr ), user_onRequest( nullptr ),
	  timeout_us( 0 ), reset_on_timeout( false ), timed_out( false ){}

bool TwoWire::on_i3c_pins( void ) const
{
	return ( I3C_SDA == _sda ) && ( I3C_SCL == _scl );
}

void TwoWire::begin( void )
{
	target_stop();
	start( 100000 );
}

void TwoWire::begin( uint8_t address )
{
	if ( on_i3c_pins() )
		panic( "Wire1: target (slave) mode isn't supported on Wire1, which runs on the I3C peripheral. Use Wire" );
#ifdef	CPU_MCXN947VDF
	/*
	 *  LPI2C3 (FlexComm3), behind Wire2, never sees the bus as a target:
	 *  its slave status stays 0 through a transfer addressed to it, while
	 *  its own master side on the same pins works, and LPI2C2 set up
	 *  identically (Wire) works. Every slave register matches LPI2C2's.
	 *  Cause not found; checked with Wire and Wire2 jumpered together.
	 */
	if ( _sda == MB_SDA && _scl == MB_SCL )
		panic( "Wire2: target (slave) mode doesn't work on Wire2 (LPI2C3 on FRDM-MCXN947). Use Wire" );
#endif
	if ( address > 127 )
		panic( "Wire: begin(address) given an address over 127" );

	if ( !target )
		target	= new WireTarget();
	target->address	= address;
	target->active	= true;

	start( 100000 );	// a controller too, as on AVR; start() arms the target
}

void TwoWire::begin( int address )
{
	if ( address >= 0 && address <= 127 )
		begin( (uint8_t)address );
	else if ( address < 0 )
		panic( "Wire: begin() given a negative address" );
	else
	{
		target_stop();
		start( address );	// the pre-0.7.0 begin(frequency)
	}
}

void TwoWire::onReceive( void (*handler)( int ) )
{
	user_onReceive	= handler;
}

void TwoWire::onRequest( void (*handler)( void ) )
{
	user_onRequest	= handler;
}

void TwoWire::target_arm( void )
{
	LPI2C_Type	*base	= i2c->lpi2c_base();
	WireTarget	*t		= target;

	t->reading		= false;
	t->rx_given		= false;
	t->rx_full		= false;
	t->tx_given		= false;
	t->in_request	= false;
	t->fill			= 0xFF;

	lpi2c_slave_config_t	cfg;
	LPI2C_SlaveGetDefaultConfig( &cfg );
	cfg.address0			= t->address;
	cfg.sclStall.enableAck	= true;		// ACKSTALL, so a full buffer can NAK

	LPI2C_SlaveInit( base, &cfg, i2c->lpi2c_clock() );
	LPI2C_SlaveTransferCreateHandle( base, &t->handle, target_callback, this );
	LPI2C_SlaveTransferNonBlocking( base, &t->handle,
		kLPI2C_SlaveAddressMatchEvent | kLPI2C_SlaveTransmitAckEvent |
		kLPI2C_SlaveRepeatedStartEvent | kLPI2C_SlaveCompletionEvent );

	LPI2C_MasterEnable( base, true );	// A153's SDK turned it off (see top)
	i2c->wait_bus_idle();
}

void TwoWire::target_stop( void )
{
	if ( !target || !target->active )
		return;

	target->active	= false;

	if ( i2c )
	{
		LPI2C_Type	*base	= i2c->lpi2c_base();

		LPI2C_SlaveDisableInterrupts( base, (uint32_t)kLPI2C_SlaveIrqFlags );
		LPI2C_SlaveEnable( base, false );
	}
}

void TwoWire::target_event( void *p )
{
	lpi2c_slave_transfer_t	*x	= static_cast<lpi2c_slave_transfer_t *>( p );
	WireTarget				*t	= target;

	switch ( x->event )
	{
		case kLPI2C_SlaveAddressMatchEvent:
			t->reading	= x->receivedAddress & 1u;
			t->rx_given	= false;
			t->rx_full	= false;
			t->tx_given	= false;
			x->data		= nullptr;
			x->dataSize	= 0;
			break;

		case kLPI2C_SlaveTransmitAckEvent:
		{
			// Before the byte is stored: ACK it only if it will fit
			size_t	stored	= t->rx_given ? sizeof( t->rx ) - x->dataSize : 0;
			LPI2C_SlaveTransmitAck( i2c->lpi2c_base(), !t->rx_full && stored < sizeof( t->rx ) );
			break;
		}

		case kLPI2C_SlaveReceiveEvent:
			if ( !t->rx_given )
			{
				x->data		= t->rx;
				x->dataSize	= sizeof( t->rx );
				t->rx_given	= true;
			}
			else
			{
				t->rx_full	= true;		// NAKed above; read it and drop it
				x->data		= &t->sink;
				x->dataSize	= 1;
			}
			break;

		case kLPI2C_SlaveTransmitEvent:
			if ( !t->tx_given )
			{
				t->tx_given	= true;
				t->tx_size	= 0;
				if ( user_onRequest )
				{
					t->in_request	= true;
					user_onRequest();
					t->in_request	= false;
				}
				if ( t->tx_size )
				{
					x->data		= t->tx;
					x->dataSize	= t->tx_size;
					break;
				}
			}
			x->data		= &t->fill;		// past the reply: 0xFF, as on AVR
			x->dataSize	= 1;
			break;

		case kLPI2C_SlaveRepeatedStartEvent:
		case kLPI2C_SlaveCompletionEvent:
			// A controller's write just ended: hand it to onReceive(), as
			// AVR does even for a write of no bytes
			if ( !t->reading && x->completionStatus == kStatus_Success )
			{
				size_t	n	= t->rx_full ? sizeof( t->rx ) : t->rx_given ? x->transferredCount : 0;

				// Its own buffer, not rx_buf: this can land in the middle of
				// a requestFrom() of our own, which reads into rx_buf
				memcpy( t->received, t->rx, n );
				rx_data		= t->received;
				rx_index	= 0;
				rx_size		= n;

				if ( user_onReceive )
					user_onReceive( (int)n );
			}
			t->reading	= false;
			t->rx_given	= false;
			t->rx_full	= false;
			break;

		default:
			break;
	}
}

void TwoWire::start( int baud )
{
	baudrate	= baud;

	bool	is_i3c	= on_i3c_pins();

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

			//	Internal pull-ups on, as AVR's twi_init() and UNO R4's
			//	begin() do: FRDM-MCXA153 has no pull-up resistors on D18/D19,
			//	so without them a bus with nothing else on it floats, reads
			//	low, and the first transfer hangs or finds the bus busy.
			//	Harmless next to external pull-ups, which set the rise time.
			i2c->pullup( true );
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
	{
		i2c->pin_low_timeout( timeout_us );
		i2c->frequency( baudrate );
	}

	i2c->err_callback( nullptr );

	if ( target && target->active )
		target_arm();
}

void TwoWire::end( void )
{
	target_stop();

	if ( i2c && !on_i3c_pins() )
		i2c->pullup( false );	// as AVR's twi_disable()

	delete i2c;
	i2c	= nullptr;
}

void TwoWire::setClock( uint32_t freq )
{
	if ( !i2c )
		return;	// no-op before begin() -- nothing to reconfigure yet

	baudrate	= (int)freq;

	if ( on_i3c_pins() )
		static_cast<I3C *>( i2c )->frequency( baudrate, 0, 0 );
	else
		i2c->frequency( baudrate );
}

void TwoWire::beginTransmission( const uint8_t address )
{
	targ_addr	= address;
	tx_size		= 0;
}

size_t TwoWire::write( uint8_t data )
{
	if ( target && target->in_request )	// onRequest(): the reply
	{
		if ( target->tx_size >= sizeof( target->tx ) )
		{
			setWriteError();
			return	0;
		}
		target->tx[ target->tx_size++ ]	= data;
		return	1;
	}

	if ( tx_size >= sizeof( tx_buf ) )
	{
		setWriteError();
		return	0;
	}

	tx_buf[ tx_size++ ]	= data;

	return	1;
}

size_t TwoWire::write( const uint8_t *data, size_t length )
{
	size_t	n	= 0;

	while ( n < length && write( data[ n ] ) )
		n++;

	return	n;
}

uint8_t TwoWire::endTransmission( bool stop )
{
	status_t	r	= i2c->write( targ_addr, tx_buf, tx_size, stop );

	check_timeout( r );
	return	r;
}

uint8_t	TwoWire::requestFrom( const uint8_t address, const size_t length, bool stop )
{
	size_t	n	= ( length < sizeof( rx_buf ) ) ? length : sizeof( rx_buf );

	rx_data		= rx_buf;
	rx_index	= 0;
	rx_size		= 0;

	status_t	r	= i2c->read( address, rx_buf, n, stop );

	check_timeout( r );
	if ( r )
		return	0;

	//	Set again after the read: this board's own target can take a write
	//	(and hand it to onReceive(), moving these) partway through it
	rx_data		= rx_buf;
	rx_index	= 0;
	rx_size		= n;
	return	n;
}

uint8_t	TwoWire::requestFrom( uint8_t address, uint8_t quantity, uint32_t iaddress, uint8_t isize, uint8_t sendStop )
{
	if ( isize > 0 )
	{
		if ( isize > 3 )
			isize	= 3;

		beginTransmission( address );
		while ( isize-- > 0 )
			write( (uint8_t)( iaddress >> ( isize * 8 ) ) );

		if ( endTransmission( false ) )
		{
			rx_data		= rx_buf;
			rx_index	= 0;
			rx_size		= 0;
			return	0;
		}
	}

	return	requestFrom( address, (size_t)quantity, (bool)sendStop );
}

void TwoWire::setWireTimeout( uint32_t timeout, bool reset_with_timeout )
{
	timeout_us			= timeout;
	reset_on_timeout	= reset_with_timeout;
	timed_out			= false;

	if ( i2c && !on_i3c_pins() )
		i2c->pin_low_timeout( timeout_us );
}

bool TwoWire::getWireTimeoutFlag( void )
{
	return	timed_out;
}

void TwoWire::clearWireTimeoutFlag( void )
{
	timed_out	= false;
}

void TwoWire::check_timeout( int status )
{
	if ( status != kStatus_LPI2C_PinLowTimeout )
		return;

	timed_out	= true;

	if ( reset_on_timeout )
	{
		//	A fresh I2C object runs LPI2C_MasterInit(), whose software reset
		//	drops whatever half-finished transfer the module was stuck in.
		delete i2c;
		i2c	= nullptr;
		start( baudrate );
	}
}

int TwoWire::available( void )
{
	return (int)( rx_size - rx_index );
}

int TwoWire::read( void )
{
	if ( rx_index >= rx_size )
		return -1;
	return	rx_data[ rx_index++ ];
}

int TwoWire::peek( void )
{
	if ( rx_index >= rx_size )
		return -1;
	return	rx_data[ rx_index ];
}
