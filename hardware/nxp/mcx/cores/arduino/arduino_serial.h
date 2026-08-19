/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_ARDUINO_SERIAL_H
#define R01LIB_ARDUINO_SERIAL_H

#include	<stdint.h>
#include	"Serial.h"
#include	"Stream.h"

/**
 * @brief Arduino-compatible Serial class for NXP MCX BSP.
 *
 * Inherits r01lib's Serial (hardware UART primitives) and Stream (the
 * hardware-independent print()/println()/find()/parseInt()/etc. layer --
 * see Print.h/Stream.h). This class itself only needs to wire Stream's
 * required overrides to the hardware primitives it gets from Serial.
 */
class SerialClass : public Serial, public Stream
{
public:
	SerialClass( int tx_pin, int rx_pin ) : Serial( tx_pin, rx_pin ) {}

	/*
	 *  attach() registers a (no-op) RX callback purely to switch getc()/
	 *  readable() from raw single-byte hardware-register polling over to
	 *  the interrupt-driven ring buffer (see Serial::_irq_handler()) --
	 *  without it the RX interrupt is never enabled at all, and bytes
	 *  arriving faster than the sketch calls read() get silently
	 *  overwritten in the 1-deep hardware receive register.
	 */
	void	begin( int baud ) { this->baud( baud ); attach( []{}, RxIrq ); }

	// ---- Print/Stream required overrides (hardware primitives only --
	//      everything else (print/println/find/parseInt/...) is inherited
	//      from Print/Stream, implemented purely in terms of these) ----

	/*
	 *  write(uint8_t)/write(bulk) are declared here (rather than relying on
	 *  `using Serial::write;`) because r01lib's own bulk write() returns
	 *  status_t (0 = success), which would silently read as "0 bytes
	 *  written" if exposed directly to sketches expecting Print's
	 *  size_t-bytes-written contract. Declaring write() here also hides
	 *  Print's const char* overloads by name (same as it hides r01lib
	 *  Serial's own write()), so `using Print::write;` brings those back.
	 */
	using	Print::write;
	size_t	write( uint8_t c ) override                         { putc( c ); return 1; }
	size_t	write( const uint8_t *buffer, size_t size ) override{ Serial::write( buffer, size ); return size; }

	int		available( void ) override { return (int)Serial::available(); }
	int		read( void ) override      { return getc(); }
	int		peek( void ) override      { return Serial::peek(); }
	void	flush( void )              { Serial::flush(); }
	int		availableForWrite( void ) override { return (int)Serial::availableForWrite(); }

	inline operator bool( void ) { return true; }
};

extern SerialClass	Serial;

// Serial1: hardware UART, separate from the USB-bridged Serial. On D0(RX)/
// D1(TX) on most boards; on FRDM-MCXN947 it's on the MikroBus header
// (MB_TX/MB_RX) instead -- see arduino_serial.cpp for why D0/D1 can't
// support it on that board.
extern SerialClass	Serial1;

#endif // !R01LIB_ARDUINO_SERIAL_H
