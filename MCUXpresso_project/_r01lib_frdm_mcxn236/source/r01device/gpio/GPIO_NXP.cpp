/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#include	"gpio/GPIO_NXP.h"

/* ******** GPIO_base ******** */

GPIO_base::GPIO_base( I2C& interface, uint8_t i2c_address, int nbits, const uint8_t* ar, uint8_t ai ) :
	n_bits( nbits ),
	n_ports( (nbits + 7) / 8 ),
	arp( ar ),
	auto_increment( ai ),
	intfp( new I2C_device( interface, i2c_address ) )
{
	init();
}

GPIO_base::GPIO_base( SPI& interface, uint8_t dev_address, int nbits, const uint8_t* ar, uint8_t ai ) :
	n_bits( nbits ),
	n_ports( (nbits + 7) / 8 ),
	arp( ar ),
	auto_increment( ai ),
	intfp( new GPIO_SPI( interface, dev_address ) )
{
	init();
}

void GPIO_base::init( void )
{
	constexpr uint16_t	i	= 0x0001;
	uint8_t*			tp	= (uint8_t*)(&i);

	endian	= tp[ 0 ];	//	'true' if LittleEndian
}

GPIO_base::~GPIO_base()
{
}

void GPIO_base::begin( board env )
{
	if ( env ) {
		DigitalOut	reset( RESET_PIN, 1 );
		DigitalOut	addr(  ADDR_PIN,  1 );

		addr	= 0;
		wait( 0.01 );
		
		reset	= 0;
		wait( 0.001 );
		
		reset	= 1;
		wait( 0.001 );
	}
}

void GPIO_base::output( int port, uint8_t value, uint8_t mask )
{
	if ( mask )
		intfp->bit_op8( *(arp + OUT) + port, mask, value );

	intfp->write_r8( *(arp + OUT) + port, value );
}

void GPIO_base::output( const uint8_t *vp )
{
	write_port( OUT, vp );
}

uint8_t GPIO_base::input( int port )
{
	return intfp->read_r8( *(arp + IN) + port );
}

uint8_t* GPIO_base::input( uint8_t *vp )
{
	read_port( IN, vp );
	
	return vp;
}

void GPIO_base::config( int port, uint8_t config, uint8_t mask )
{
	if ( mask )
		intfp->bit_op8( *(arp + CONFIG) + port, mask, config );

	intfp->write_r8( *(arp + CONFIG) + port, config );
}

void GPIO_base::config( const uint8_t* vp )
{
	write_port( CONFIG, vp );
}

void GPIO_base::write_port( access_word w, const uint8_t* vp )
{
	if ( auto_increment ) {
		intfp->reg_w( auto_increment | *(arp + w), vp, n_ports );		
	}
	else {
		for ( int i = 0; i < n_ports; i++ )
			intfp->write_r8( *(arp + w) + i, *vp++ );
	}
}

void GPIO_base::write_port16( access_word w, const uint16_t* vp )
{
	uint16_t	b[ n_ports ];
	
	if ( endian ) {
		uint16_t	temp;
		for ( int i = 0; i < n_ports; i++ ) {
			temp	= vp[ i ] << 8;
			b[ i ]	= temp | vp[ i ] >> 8;			
		}
	}
	else {
		memcpy( b, vp, n_ports << 1 );
	}
	
	int	n_bytes	= (n_bits * 2 + 7) / 8;

	if ( auto_increment ) {
		intfp->reg_w( auto_increment | *(arp + w), (uint8_t*)b, n_bytes );		
	}
	else {
		for ( int i = 0; i < n_bytes; i++ )
			intfp->write_r8( *(arp + w) + i, b[ i ] );
	}
}

uint8_t* GPIO_base::read_port( access_word w, uint8_t* vp )
{
	if ( auto_increment ) {
		intfp->reg_r( auto_increment | *(arp + w), vp, n_ports );		
	}
	else {
		for ( int i = 0; i < n_ports; i++ )
			*(vp + i)	= intfp->read_r8( *(arp + w) + i );
	}
	
	return vp;
}

uint16_t*  GPIO_base::read_port16( access_word w, uint16_t* vp )
{
	int	n_bytes	= (n_bits * 2 + 7) / 8;
	
	if ( auto_increment ) {
		intfp->reg_r( auto_increment | *(arp + w), (uint8_t*)vp, n_bytes );	
	}
	else {
		for ( int i = 0; i < n_bytes; i++ )
			*(vp + i)	= intfp->read_r8( *(arp + w) + i );		
	}
		

	if ( endian ) {
		uint16_t	temp;

		for ( int i = 0; i < n_ports; i++ ) {
			temp	= vp[ i ] << 8;
			vp[ i ]	= temp | vp[ i ] >> 8;			
		}
	}
	
	return vp;
}

void GPIO_base::write_port( access_word w, uint8_t value, int port_num )
{
	intfp->write_r8( *(arp + w) + port_num, value );
}

void GPIO_base::write_port16( access_word w, uint16_t value, int port_num )
{
	intfp->write_r16( *(arp + w) + port_num, value );
}

uint8_t GPIO_base::read_port( access_word w, int port_num )
{
	return intfp->read_r8( *(arp + w) + port_num );
}

uint16_t GPIO_base::read_port16( access_word w, int port_num )
{
	return intfp->read_r16( *(arp + w) + port_num );
}

void GPIO_base::print_bin( uint8_t v )
{
	PRINTF( " 0b" );
	for ( int i = 7; 0 <= i; i-- )
		PRINTF( "%01u", (v >> i) & 0x1 );
}


/* ******** PCA9554 ******** */

PCA9554::PCA9554( I2C& interface, uint8_t i2c_address ) :
	GPIO_base( interface, i2c_address, 8, access_ref, 0x00 )
{
}

PCA9554::~PCA9554()
{
}

constexpr uint8_t PCA9554::access_ref[];


/* ******** PCA9555 ******** */
PCA9555::PCA9555( I2C& interface, uint8_t i2c_address ) :
	GPIO_base( interface, i2c_address, 16, access_ref, 0x00 )
{
}

PCA9555::~PCA9555()
{
}

constexpr uint8_t PCA9555::access_ref[];

PCAL6xxx_base::PCAL6xxx_base( I2C& interface, uint8_t i2c_address, const int nbits, const uint8_t arp[], uint8_t ai ) :
	GPIO_base( interface, i2c_address, nbits, arp, ai )
{
}

PCAL6xxx_base::~PCAL6xxx_base()
{
}


/* ******** PCAL6408A ******** */

PCAL6408A::PCAL6408A( I2C& interface, uint8_t i2c_address ) :
	PCAL6xxx_base( interface, i2c_address, 8, access_ref, 0 )
{
}

PCAL6408A::~PCAL6408A()
{
}

constexpr uint8_t PCAL6408A::access_ref[];


/* ******** PCAL6416A ******** */

PCAL6416A::PCAL6416A( I2C& interface, uint8_t i2c_address ) :
	PCAL6xxx_base( interface, i2c_address, 16, access_ref, 0 )
{
}

PCAL6416A::~PCAL6416A()
{
}

constexpr uint8_t PCAL6416A::access_ref[];


/* ******** PCAL6524 ******** */

PCAL6524::PCAL6524( I2C& interface, uint8_t i2c_address ) :
	PCAL6xxx_base( interface, i2c_address, 24, access_ref, 0 )
{
}

PCAL6524::~PCAL6524()
{
}

constexpr uint8_t PCAL6524::access_ref[];


/* ******** PCAL6534 ******** */

PCAL6534::PCAL6534( I2C& interface, uint8_t i2c_address ) :
	PCAL6xxx_base( interface, i2c_address, 34, access_ref, 0 )
{
}

PCAL6534::~PCAL6534()
{
}

constexpr uint8_t PCAL6534::access_ref[];


/* ******** PCAL9722 ******** */

PCAL97xx_base::PCAL97xx_base( SPI& interface, uint8_t dev_address, const int nbits, const uint8_t arp[], uint8_t ai ) :
	GPIO_base( interface, dev_address, nbits, arp, ai )
{
}

PCAL97xx_base::~PCAL97xx_base()
{
}




PCAL9722::PCAL9722( SPI& interface, uint8_t i2c_address ) :
	PCAL97xx_base( interface, i2c_address, 24, access_ref, 0x80 )
{
}

PCAL9722::~PCAL9722()
{
}

void PCAL9722::begin( board env )
{
	if ( env ) {
		DigitalOut	reset( RESET_PIN_PCAL9722, 1 );

		reset	= 0;
		wait( 0.001 );
		
		reset	= 1;
		wait( 0.001 );
	}
}

constexpr uint8_t PCAL9722::access_ref[];


