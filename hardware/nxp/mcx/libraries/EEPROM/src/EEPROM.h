/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef MCX_EEPROM_H
#define MCX_EEPROM_H

#include <stdint.h>
#include <stddef.h>

/** Last EEPROM address, as AVR's <avr/io.h> defines it; some libraries
 *  size their storage from it. */
#ifndef E2END
#define E2END	0x3FF
#endif

/** Storage for 1024 bytes that keeps its contents across resets, power
 *  cycles and sketch uploads, with the AVR EEPROM library's interface:
 *  read()/write()/update(), EEPROM[i], get()/put() of any type, and
 *  iteration from begin() to end().
 *
 *  The bytes live in the top of the MCU's on-chip flash, which the linker
 *  script keeps the program out of: the last 16KB on FRDM-MCXA153 and 64KB
 *  on FRDM-MCXN947, two halves used in turn. A
 *  write appends a small record to the current half, which takes about
 *  0.1-0.5ms. When that half is full, which happens every ~440 writes on
 *  FRDM-MCXA153 and every ~250 on FRDM-MCXN947, the other half is erased
 *  and the latest contents copied there, so that write takes up to ~6ms
 *  and ~11ms respectively. Reads come from a copy in RAM and take no time.
 *
 *  Writing a byte its current value writes nothing, so write() behaves as
 *  update() here: rewriting unchanged data costs neither time nor wear.
 *
 *  Serial input that keeps arriving during one of those longer writes
 *  can be lost. Measured with a continuous stream at 115200 baud: ~20
 *  bytes per such write on FRDM-MCXA153, where the program runs from the
 *  same flash and so interrupts wait while it is erased or written, and
 *  ~60 on FRDM-MCXN947, where interrupts go on but the 64-byte receive
 *  buffer fills while the write blocks. The other writes lose nothing.
 *
 *  Not for use from an interrupt handler.
 */

class EEPROMClass;

/** One byte of EEPROM, as EEPROM[i] returns it: reads and assigns like a
 *  uint8_t. */
class EERef
{
public:
	EERef( int index ) : index( index ) {}

	uint8_t	operator*() const;
	operator uint8_t() const { return **this; }

	EERef	&operator=( uint8_t value );
	EERef	&operator=( const EERef &ref )	{ return *this = *ref; }
	EERef	&update( uint8_t value )		{ return *this = value; }

	EERef	&operator+=( uint8_t v )	{ return *this = **this + v; }
	EERef	&operator-=( uint8_t v )	{ return *this = **this - v; }
	EERef	&operator*=( uint8_t v )	{ return *this = **this * v; }
	EERef	&operator/=( uint8_t v )	{ return *this = **this / v; }
	EERef	&operator%=( uint8_t v )	{ return *this = **this % v; }
	EERef	&operator&=( uint8_t v )	{ return *this = **this & v; }
	EERef	&operator|=( uint8_t v )	{ return *this = **this | v; }
	EERef	&operator^=( uint8_t v )	{ return *this = **this ^ v; }
	EERef	&operator<<=( uint8_t v )	{ return *this = **this << v; }
	EERef	&operator>>=( uint8_t v )	{ return *this = **this >> v; }

	EERef	&operator++()		{ return *this += 1; }
	EERef	&operator--()		{ return *this -= 1; }
	uint8_t	operator++( int )	{ uint8_t v = **this; ++( *this ); return v; }
	uint8_t	operator--( int )	{ uint8_t v = **this; --( *this ); return v; }

	int		index;
};

/** An EEPROM address that dereferences to an EERef, so a range-for over
 *  EEPROM visits every byte:  for ( EERef b : EEPROM ) b = 0; */
class EEPtr
{
public:
	EEPtr( int index ) : index( index ) {}

	operator int() const				{ return index; }
	EEPtr	&operator=( int i )			{ index = i; return *this; }
	bool	operator!=( const EEPtr &p ) const	{ return index != p.index; }
	EERef	operator*()					{ return EERef( index ); }

	EEPtr	&operator++()		{ ++index; return *this; }
	EEPtr	&operator--()		{ --index; return *this; }
	EEPtr	operator++( int )	{ return EEPtr( index++ ); }
	EEPtr	operator--( int )	{ return EEPtr( index-- ); }
	EEPtr	&operator+=( int n )	{ index += n; return *this; }
	EEPtr	&operator-=( int n )	{ index -= n; return *this; }

	int		index;
};

class EEPROMClass
{
public:
	/** @return the byte at idx; 0xFF if never written, or out of range */
	uint8_t		read( int idx );

	/** Store a byte. Out-of-range addresses are ignored. */
	void		write( int idx, uint8_t value );

	/** Same as write(): neither writes a byte that already holds value */
	void		update( int idx, uint8_t value )	{ write( idx, value ); }

	EERef		operator[]( int idx )	{ return EERef( idx ); }

	EEPtr		begin()		{ return EEPtr( 0 ); }
	EEPtr		end()		{ return EEPtr( length() ); }

	/** @return the number of bytes: 1024 */
	uint16_t	length()	{ return E2END + 1; }

	/** Read an object of any type from idx on. */
	template <typename T>
	T	&get( int idx, T &t )
	{
		read_block( idx, (uint8_t *)&t, sizeof( T ) );
		return t;
	}

	/** Store an object of any type from idx on. Only the bytes that
	 *  differ are written. A put() that is cut short by a reset or power
	 *  loss can leave the object partly old, partly new. */
	template <typename T>
	const T	&put( int idx, const T &t )
	{
		write_block( idx, (const uint8_t *)&t, sizeof( T ) );
		return t;
	}

	/** get()/put() for raw bytes. Anything past the end is left out. */
	void		read_block( int idx, uint8_t *dst, size_t n );
	void		write_block( int idx, const uint8_t *src, size_t n );
};

extern EEPROMClass	EEPROM;

#endif // MCX_EEPROM_H
