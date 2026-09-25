/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

/*
 *  How the 1024 bytes are kept in flash
 *
 *  The area the linker script sets aside (EEPROM_FLASH) is split into two
 *  halves, only one of which is current. A half is laid out as:
 *
 *    offset 0      header: magic, sequence number, its complement, size
 *    offset 128    image: all 1024 bytes as they were when the half was
 *                  started
 *    offset 1152-  log: one record per programming unit, each a change
 *                  made since, appended in order
 *
 *  A record is [address low, address high, length, CRC-8, data...]. The
 *  programming unit, and so a record, is the smallest the flash takes:
 *  16 bytes on FRDM-MCXA153 (12 of data), a 128-byte page on
 *  FRDM-MCXN947 (124 of data). Each unit is programmed once between
 *  erases.
 *
 *  At the first access the current half (valid header, highest sequence
 *  number) is read into RAM: the image, then every record with a good
 *  CRC, up to the first unit that is still erased. From then on reads come
 *  from RAM, and flash is only written, never read back.
 *
 *  When the log is full, the other half is erased, the RAM copy is
 *  written there as its image, and its header -- written last -- makes it
 *  current. A reset or power loss part way leaves the old half current,
 *  holding everything up to the last completed write. A record cut short
 *  fails its CRC and is skipped.
 */

#include "EEPROM.h"

#include <string.h>

#include "fsl_device_registers.h"

#if defined( CPU_MCXA153VLH )
#include "fsl_romapi.h"
static const uint32_t	UNIT	= 16;	// phrase
#elif defined( CPU_MCXN947VDF )
#include "fsl_flash.h"
static const uint32_t	UNIT	= 128;	// page
#else
#error EEPROM: this board is not supported
#endif

//	Bounds of the area, from the linker script
extern "C" const uint8_t	__base_EEPROM_FLASH[];
extern "C" const uint8_t	__top_EEPROM_FLASH[];

static const uint32_t	SIZE		= E2END + 1;
static const uint32_t	PAGE		= 128;				// image alignment, for page programming
static const uint32_t	IMAGE_OFF	= PAGE;
static const uint32_t	LOG_OFF		= IMAGE_OFF + SIZE;
static const uint32_t	DATA_MAX	= UNIT - 4;
static const uint32_t	MAGIC		= 0x4545584D;		// "MXEE"

EEPROMClass	EEPROM;

namespace {

flash_config_t	flash;
bool			started		= false;
bool			have_half	= false;	// false until something is written
int				current;				// 0 or 1
uint32_t		seq;
uint32_t		log_next;				// offset in the current half
alignas( 4 ) uint8_t	ram[ SIZE ];		// also a source buffer for the flash API
uint32_t		unit_buf[ ( PAGE > UNIT ? PAGE : UNIT ) / 4 ];

uint32_t area_base( void )	{ return (uint32_t)__base_EEPROM_FLASH; }
uint32_t half_size( void )	{ return ( (uint32_t)__top_EEPROM_FLASH - (uint32_t)__base_EEPROM_FLASH ) / 2; }
uint32_t half_base( int h )	{ return area_base() + h * half_size(); }

uint8_t crc8( const uint8_t *p, size_t n, uint8_t c = 0 )
{
	while ( n-- )
	{
		c	^= *p++;
		for ( int i = 0; i < 8; i++ )
			c	= ( c & 0x80 ) ? (uint8_t)( ( c << 1 ) ^ 0x07 ) : (uint8_t)( c << 1 );
	}
	return c;
}

void clear_cache( void )
{
	//	Drop the code cache's lines, as NXP's flashiap example does after
	//	each erase and program, so nothing reads back what the flash held
	//	before. Nothing here reads flash after start(), but a sketch might
	SYSCON->LPCAC_CTRL	|= SYSCON_LPCAC_CTRL_CLR_LPCAC_MASK;
}

bool erase_half( int h )
{
	status_t	s;
#if defined( CPU_MCXA153VLH )
	s	= FLASH_EraseSector( &flash, half_base( h ), half_size(), kFLASH_ApiEraseKey );
#else
	s	= FLASH_Erase( &flash, half_base( h ), half_size(), kFLASH_ApiEraseKey );
#endif
	clear_cache();
	return s == kStatus_Success;
}

//	Program n bytes (a multiple of UNIT, or of PAGE for pages) at addr
bool program( uint32_t addr, const void *src, uint32_t n )
{
	status_t	s;
#if defined( CPU_MCXA153VLH )
	if ( ( n % PAGE == 0 ) && ( addr % PAGE == 0 ) )
		s	= FLASH_ProgramPage( &flash, addr, (uint8_t *)src, n );
	else
		s	= FLASH_ProgramPhrase( &flash, addr, (uint8_t *)src, n );
#else
	s	= FLASH_Program( &flash, addr, (uint8_t *)src, n );
#endif
	clear_cache();
	return s == kStatus_Success;
}

bool erased( const uint8_t *p, size_t n )
{
	while ( n-- )
		if ( *p++ != 0xFF )
			return false;
	return true;
}

bool header_valid( int h, uint32_t *s )
{
	const uint32_t	*w	= (const uint32_t *)half_base( h );

	if ( ( w[ 0 ] != MAGIC ) || ( w[ 1 ] != ~w[ 2 ] ) || ( w[ 3 ] != SIZE ) )
		return false;

	*s	= w[ 1 ];
	return true;
}

void start( void )
{
	if ( started )
		return;

	started	= true;
	memset( &flash, 0, sizeof( flash ) );
#if defined( CPU_MCXA153VLH )
	FLASH_API->flash_init( &flash );
#else
	FLASH_Init( &flash );
#endif

	uint32_t	s0, s1;
	bool		v0	= header_valid( 0, &s0 );
	bool		v1	= header_valid( 1, &s1 );

	memset( ram, 0xFF, SIZE );

	if ( !v0 && !v1 )
		return;		// nothing written yet: every byte reads 0xFF, as a new AVR's

	have_half	= true;
	current		= ( v0 && ( !v1 || (int32_t)( s0 - s1 ) > 0 ) ) ? 0 : 1;
	seq			= current ? s1 : s0;

	const uint8_t	*base	= (const uint8_t *)half_base( current );

	memcpy( ram, base + IMAGE_OFF, SIZE );

	for ( log_next = LOG_OFF; log_next + UNIT <= half_size(); log_next += UNIT )
	{
		const uint8_t	*r	= base + log_next;

		if ( erased( r, UNIT ) )
			break;

		uint32_t	addr	= r[ 0 ] | ( r[ 1 ] << 8 );
		uint32_t	len		= r[ 2 ];

		if ( ( len == 0 ) || ( len > DATA_MAX ) || ( addr + len > SIZE ) )
			continue;
		if ( crc8( r + 4, len, crc8( r, 3 ) ) != r[ 3 ] )
			continue;	// cut short by a reset: skip it

		memcpy( ram + addr, r + 4, len );
	}
}

//	Write the RAM copy into the other half and make that one current
void compact( void )
{
	int	next	= have_half ? 1 - current : 0;

	if ( !erase_half( next ) )
		return;

	//	The image, a page at a time; pages still all 0xFF need no programming
	for ( uint32_t i = 0; i < SIZE; i += PAGE )
		if ( !erased( ram + i, PAGE ) )
			if ( !program( half_base( next ) + IMAGE_OFF + i, ram + i, PAGE ) )
				return;

	uint32_t	new_seq	= have_half ? seq + 1 : 1;

	memset( unit_buf, 0xFF, UNIT );
	unit_buf[ 0 ]	= MAGIC;
	unit_buf[ 1 ]	= new_seq;
	unit_buf[ 2 ]	= ~new_seq;
	unit_buf[ 3 ]	= SIZE;

	if ( !program( half_base( next ), unit_buf, UNIT ) )
		return;

	have_half	= true;
	current		= next;
	seq			= new_seq;
	log_next	= LOG_OFF;
}

//	Record ram[addr..addr+len) as one log record, len <= DATA_MAX
bool append( uint32_t addr, uint32_t len )
{
	if ( !have_half || ( log_next + UNIT > half_size() ) )
		return false;

	uint8_t	*r	= (uint8_t *)unit_buf;

	memset( r, 0xFF, UNIT );
	r[ 0 ]	= addr & 0xFF;
	r[ 1 ]	= addr >> 8;
	r[ 2 ]	= len;
	memcpy( r + 4, ram + addr, len );
	r[ 3 ]	= crc8( r + 4, len, crc8( r, 3 ) );

	bool	ok	= program( half_base( current ) + log_next, r, UNIT );

	log_next	+= UNIT;	// a failed unit is not reused either: it was programmed once
	return ok;
}

}	// namespace

uint8_t EEPROMClass::read( int idx )
{
	start();
	return ( ( idx >= 0 ) && ( (uint32_t)idx < SIZE ) ) ? ram[ idx ] : 0xFF;
}

void EEPROMClass::write( int idx, uint8_t value )
{
	write_block( idx, &value, 1 );
}

void EEPROMClass::read_block( int idx, uint8_t *dst, size_t n )
{
	start();

	for ( size_t i = 0; i < n; i++ )
		dst[ i ]	= read( idx + i );
}

void EEPROMClass::write_block( int idx, const uint8_t *src, size_t n )
{
	start();

	if ( idx < 0 )
	{
		if ( (size_t)( -idx ) >= n )
			return;
		src	+= -idx;
		n	-= -idx;
		idx	= 0;
	}
	if ( (uint32_t)idx >= SIZE )
		return;
	if ( idx + n > SIZE )
		n	= SIZE - idx;

	//	Only the span from the first to the last byte that differs
	size_t	first	= 0;
	while ( ( first < n ) && ( ram[ idx + first ] == src[ first ] ) )
		first++;
	if ( first == n )
		return;

	size_t	last	= n - 1;
	while ( ram[ idx + last ] == src[ last ] )
		last--;

	memcpy( ram + idx + first, src + first, last - first + 1 );

	for ( uint32_t a = idx + first; a <= idx + last; a += DATA_MAX )
	{
		uint32_t	len	= idx + last + 1 - a;

		if ( len > DATA_MAX )
			len	= DATA_MAX;

		if ( !append( a, len ) )
		{
			//	Log full (or nothing written before): the new image holds
			//	this write and the rest of it
			compact();
			return;
		}
	}
}

uint8_t EERef::operator*() const
{
	return EEPROM.read( index );
}

EERef &EERef::operator=( uint8_t value )
{
	EEPROM.write( index, value );
	return *this;
}
