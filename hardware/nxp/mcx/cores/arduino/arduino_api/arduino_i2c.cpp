/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"r01lib.h"
#include	"i3c.h"
#include	"arduino_i2c.h"

TwoWire	Wire(  I2C_SDA, I2C_SCL );
#if defined( CPU_MCXA153VLH )
TwoWire	Wire1( I3C_SDA, I3C_SCL );
#elif defined( CPU_MCXN947VDF )
TwoWire	Wire1( I3C_SDA, I3C_SCL );
TwoWire	Wire2( MB_SDA,  MB_SCL );
#elif defined( CPU_MCXA156VLL )
// The on-board sensor is on Wire's own D18/D19 here (I3C0 reaches it only
// through those pins), so Wire1 is the MikroBus I2C, on LPI2C3.
TwoWire	Wire1( MB_SDA,  MB_SCL );
#elif defined( CPU_MCXN236VDF )
// No I3C sensor on this board. Wire1 is LPI2C2 (P4_0/P4_1), the bus of the
// on-board accelerometer and of the MikroBus I2C.
TwoWire	Wire1( MB_SDA,  MB_SCL );
#elif defined( CPU_MCXC444VLH )
// The on-board accelerometer's bus, I2C0, which no header pin reaches. A4/A5
// are Wire's I2C1, so they can't be a second bus here.
TwoWire	Wire1( PTE25,  PTE24 );
#else
#error "arduino_i2c.cpp: say which pins this board's Wire1 is on"
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
#if !defined( CPU_MCXC444VLH )	// LPI2C and I3C; FRDM-MCXC444 has the Kinetis I2C
	lpi2c_slave_handle_t	handle;
#endif
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
#if defined( CPU_MCXC444VLH )
	bool		in_write;	// a controller is writing to this target
	size_t		rx_count;	// bytes of that write in rx[]
	size_t		tx_index;	// next byte of tx[] to send
#endif
};

#if !defined( CPU_MCXC444VLH )	// LPI2C and I3C; FRDM-MCXC444 has the Kinetis I2C
static void target_callback( LPI2C_Type *, lpi2c_slave_transfer_t *xfer, void *self )
{
	static_cast<TwoWire *>( self )->target_event( xfer );
}
#endif

TwoWire::TwoWire( int sda_pin, int scl_pin )
	: _sda( sda_pin ), _scl( scl_pin ), i2c( nullptr ),
	  tx_size( 0 ), rx_data( rx_buf ), rx_index( 0 ), rx_size( 0 ),
	  target( nullptr ), user_onReceive( nullptr ), user_onRequest( nullptr ),
	  timeout_us( 0 ), reset_on_timeout( false ), timed_out( false ), last_status( 0 ){}

bool TwoWire::on_i3c_pins( void ) const
{
#if defined( CPU_MCXA153VLH ) || defined( CPU_MCXN947VDF )
	return ( I3C_SDA == _sda ) && ( I3C_SCL == _scl );
#elif defined( CPU_MCXA156VLL ) || defined( CPU_MCXN236VDF ) || defined( CPU_MCXC444VLH )
	// No TwoWire runs on I3C here (FRDM-MCXC444 has no I3C): I3C_SDA/I3C_SCL are Wire's D18/D19,
	// which Wire drives through LPI2C0 (LPI2C5 on FRDM-MCXN236).
	return false;
#else
#error "arduino_i2c.cpp: say whether any TwoWire here runs on I3C"
#endif
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

#if !defined( CPU_MCXC444VLH )	// LPI2C and I3C; FRDM-MCXC444 has the Kinetis I2C
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
#if defined( CPU_MCXN236VDF )
	flexcomm_keep_shared( LPI2C_GetInstance( base ) );	// Serial1's FlexComm too
#endif
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
#else
/*
 *  Target (slave) mode on FRDM-MCXC444's Kinetis I2C, driven from the I2C
 *  interrupt here: the SDK's I2C_SlaveInit() would rewrite the master's
 *  settings on the same module too.
 *
 *  One Kinetis I2C is master or slave at a time, never both at once, so
 *  unlike an LPI2C it can't address its own target. r01lib's controller
 *  transfers turn this interrupt and FLT[SSIE] off while they run (they
 *  poll the same IICIF, which SSIE would set for their own START), and a
 *  controller transfer can't start while another controller is talking to
 *  this target (the bus is busy).
 *
 *  The STOP and START detection interrupt (FLT[SSIE]) ends a write. Flags
 *  are cleared in the order the reference manual asks: STOPF/STARTF first,
 *  then IICIF.
 */
namespace {

TwoWire	*s_target_of[ 2 ]	= {};	// the TwoWire armed on I2C0, I2C1

constexpr uint8_t	FLT_W1C	= I2C_FLT_STOPF_MASK | I2C_FLT_STARTF_MASK;

int instance_of( I2C_Type *base )
{
	return ( base == I2C0 ) ? 0 : 1;
}

void target_irq( int n )
{
	if ( s_target_of[ n ] )
		s_target_of[ n ]->target_event( nullptr );
	else
	{
		I2C_Type	*base	= n ? I2C1 : I2C0;

		base->FLT	= base->FLT;			// clears STOPF/STARTF that are set
		base->S		= I2C_S_IICIF_MASK;
	}
}

}	// namespace

extern "C" void I2C0_IRQHandler( void )
{
	target_irq( 0 );
	SDK_ISR_EXIT_BARRIER;
}

extern "C" void I2C1_IRQHandler( void )
{
	target_irq( 1 );
	SDK_ISR_EXIT_BARRIER;
}

void TwoWire::target_arm( void )
{
	I2C_Type	*base	= i2c->kinetis_base();
	WireTarget	*t		= target;
	int			n		= instance_of( base );

	t->in_write		= false;
	t->rx_count		= 0;
	t->rx_full		= false;
	t->tx_size		= 0;
	t->tx_index		= 0;
	t->in_request	= false;
	t->fill			= 0xFF;

	s_target_of[ n ]	= this;

	// The address register is named A1, as is the Arduino pin (io.h)
#pragma push_macro( "A1" )
#undef A1
	base->A1	= (uint8_t)( t->address << 1 );
#pragma pop_macro( "A1" )
	base->FLT	= ( base->FLT & (uint8_t)~FLT_W1C ) | I2C_FLT_SSIE_MASK;
	base->FLT	= base->FLT;			// clear stale STOPF/STARTF
	base->S		= I2C_S_IICIF_MASK | I2C_S_ARBL_MASK;
	base->C1	= ( base->C1 & (uint8_t)~( I2C_C1_TX_MASK | I2C_C1_TXAK_MASK ) ) | I2C_C1_IICIE_MASK;

	EnableIRQ( n ? I2C1_IRQn : I2C0_IRQn );
}

void TwoWire::target_stop( void )
{
	if ( !target || !target->active )
		return;

	target->active	= false;

	if ( i2c )
	{
		I2C_Type	*base	= i2c->kinetis_base();

		base->C1	&= (uint8_t)~I2C_C1_IICIE_MASK;
		base->FLT	= base->FLT & (uint8_t)~( FLT_W1C | I2C_FLT_SSIE_MASK );
#pragma push_macro( "A1" )
#undef A1
		base->A1	= 0;
#pragma pop_macro( "A1" )
		s_target_of[ instance_of( base ) ]	= nullptr;
	}
}

void TwoWire::target_event( void *p )
{
	(void)p;

	I2C_Type	*base	= i2c->kinetis_base();
	WireTarget	*t		= target;
	uint8_t		flt		= base->FLT;

	if ( flt & FLT_W1C )
	{
		base->FLT	= flt;					// STOPF/STARTF first ...
		base->S		= I2C_S_IICIF_MASK;		// ... then IICIF
		base->C1	&= (uint8_t)~I2C_C1_TXAK_MASK;

		// A controller's write just ended (STOP or repeated START): hand it
		// to onReceive(), as AVR does even for a write of no bytes
		if ( t->in_write )
		{
			size_t	count	= t->rx_count;

			// Its own buffer, not rx_buf: this can land in the middle of a
			// requestFrom() of our own, which reads into rx_buf
			memcpy( t->received, t->rx, count );
			rx_data		= t->received;
			rx_index	= 0;
			rx_size		= count;
			t->in_write	= false;

			if ( user_onReceive )
				user_onReceive( (int)count );
		}
		return;
	}

	uint8_t	s	= base->S;

	if ( !( s & I2C_S_IICIF_MASK ) )
		return;

	base->S	= I2C_S_IICIF_MASK;

	if ( base->C1 & I2C_C1_MST_MASK )		// a controller transfer of our own
		return;

	if ( s & I2C_S_ARBL_MASK )
	{
		base->S	= I2C_S_ARBL_MASK;
		if ( !( s & I2C_S_IAAS_MASK ) )
			return;
	}

	if ( s & I2C_S_IAAS_MASK )
	{
		if ( s & I2C_S_SRW_MASK )			// the controller reads
		{
			base->C1		|= I2C_C1_TX_MASK;
			t->tx_size		= 0;
			t->tx_index		= 0;
			if ( user_onRequest )
			{
				t->in_request	= true;
				user_onRequest();
				t->in_request	= false;
			}
			base->D	= ( t->tx_index < t->tx_size ) ? t->tx[ t->tx_index++ ] : t->fill;
		}
		else								// the controller writes
		{
			base->C1	&= (uint8_t)~( I2C_C1_TX_MASK | I2C_C1_TXAK_MASK );
			t->in_write	= true;
			t->rx_count	= 0;
			t->rx_full	= false;
			(void)base->D;					// lets go of SCL
		}
		return;
	}

	if ( base->C1 & I2C_C1_TX_MASK )		// sending to the controller
	{
		if ( s & I2C_S_RXAK_MASK )			// it NAKed: the read is over
		{
			base->C1	&= (uint8_t)~I2C_C1_TX_MASK;
			(void)base->D;
		}
		else								// past the reply: 0xFF, as on AVR
			base->D	= ( t->tx_index < t->tx_size ) ? t->tx[ t->tx_index++ ] : t->fill;
		return;
	}

	// Receiving. TXAK is the ACK of the byte after this one: NAK it once
	// the buffer is full (a NAKed byte still arrives here, and is dropped)
	uint8_t	b	= base->D;

	if ( t->rx_count < sizeof( t->rx ) )
		t->rx[ t->rx_count++ ]	= b;
	else
		t->rx_full	= true;

	if ( t->rx_count >= sizeof( t->rx ) )
		base->C1	|= I2C_C1_TXAK_MASK;
}
#endif

void TwoWire::start( int baud )
{
	baudrate	= baud;

	bool	is_i3c	= on_i3c_pins();

	if ( !i2c )
	{
#if !defined( CPU_MCXC444VLH )	// LPI2C and I3C; FRDM-MCXC444 has the Kinetis I2C
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
#endif
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
#if !defined( CPU_MCXC444VLH )	// LPI2C and I3C; FRDM-MCXC444 has the Kinetis I2C
	if ( is_i3c )
		static_cast<I3C *>( i2c )->frequency( baudrate, 0, 0 );
	else
	{
		i2c->pin_low_timeout( timeout_us );
		i2c->frequency( baudrate );
	}
#else
	(void)is_i3c;
	i2c->pin_low_timeout( timeout_us );	// the Kinetis I2C's SCL low timeout
	i2c->frequency( baudrate );
#endif

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

#if !defined( CPU_MCXC444VLH )	// LPI2C and I3C; FRDM-MCXC444 has the Kinetis I2C
	if ( on_i3c_pins() )
		static_cast<I3C *>( i2c )->frequency( baudrate, 0, 0 );
	else
#endif
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

//	The driver's status as AVR's endTransmission() codes: 0 success, 2 address
//	NAK, 3 data NAK, 4 other, 5 timeout
static uint8_t avr_code( status_t r )
{
	switch ( r )
	{
		case kStatus_Success:					return 0;
#if !defined( CPU_MCXC444VLH )
		case kStatus_LPI2C_Nak:					return 2;	// address or data: the LPI2C doesn't say which
		case kStatus_LPI2C_PinLowTimeout:
		case kStatus_LPI2C_Timeout:				return 5;
#else
		case kStatus_I2C_Addr_Nak:				return 2;
		case kStatus_I2C_Nak:					return 3;
		case kStatus_I2C_Timeout:				return 5;
#endif
#ifdef I3C_SUPPORTED
		case kStatus_I3C_Nak:					return 2;	// the I3C reports an address's NAK only
		case kStatus_I3C_Timeout:				return 5;
#endif
		default:								return 4;
	}
}

uint8_t TwoWire::endTransmission( bool stop )
{
	status_t	r	= i2c->write( targ_addr, tx_buf, tx_size, stop );

	last_status	= r;
	check_timeout( r );
	return	avr_code( r );
}

uint8_t	TwoWire::requestFrom( const uint8_t address, const size_t length, bool stop )
{
	size_t	n	= ( length < sizeof( rx_buf ) ) ? length : sizeof( rx_buf );

	rx_data		= rx_buf;
	rx_index	= 0;
	rx_size		= 0;

	status_t	r	= i2c->read( address, rx_buf, n, stop );

	last_status	= r;
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

#if !defined( CPU_MCXC444VLH )	// LPI2C and I3C; FRDM-MCXC444 has the Kinetis I2C
	if ( i2c && !on_i3c_pins() )
		i2c->pin_low_timeout( timeout_us );
#else
	if ( i2c )
		i2c->pin_low_timeout( timeout_us );
#endif
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
#if !defined( CPU_MCXC444VLH )	// LPI2C and I3C; FRDM-MCXC444 has the Kinetis I2C
	if ( status != kStatus_LPI2C_PinLowTimeout )
		return;
#else
	if ( status != kStatus_I2C_Timeout )	// the Kinetis I2C's SCL low timeout
		return;
#endif

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
