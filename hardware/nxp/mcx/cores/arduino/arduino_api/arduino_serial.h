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
	/** Construct on the given TX/RX pin pair. Hardware isn't touched until
	 *  begin(); see Serial's constructor (which this delegates straight to)
	 *  for the actual pin-resolution/panic() behavior.
	 * @param tx_pin transmit pin
	 * @param rx_pin receive pin
	 */
	SerialClass( int tx_pin, int rx_pin ) : Serial( tx_pin, rx_pin ) {}

	/** Start the port at the given baud rate and switch it into
	 *  interrupt-driven RX mode.
	 *
	 *  apply_pin_mux() is what actually routes the TX/RX pins to the UART.
	 *  The constructor deliberately leaves them alone so that a port the
	 *  sketch never begin()s doesn't hold pins another peripheral may want
	 *  -- which matters on FRDM-MCXN947, where Serial1 shares its pins
	 *  with I3C_SDA/I3C_SCL (see Serial's constructor for the full story).
	 *
	 *  attach() registers a (no-op) RX callback purely to switch getc()/
	 *  readable() from raw single-byte hardware-register polling over to
	 *  the interrupt-driven ring buffer (see Serial::_irq_handler()) --
	 *  without it the RX interrupt is never enabled at all, and bytes
	 *  arriving faster than the sketch calls read() get silently
	 *  overwritten in the 1-deep hardware receive register.
	 *
	 * @param baud baud rate in bps
	 */
	void	begin( int baud ) { apply_pin_mux(); this->baud( baud ); attach( []{}, RxIrq ); }

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

	/** Print::write() override: send one byte. @param c byte to send @return 1 */
	size_t	write( uint8_t c ) override                         { putc( c ); return 1; }
	/** Print::write() override: send a buffer. @param buffer bytes to send @param size buffer length @return size */
	size_t	write( const uint8_t *buffer, size_t size ) override{ Serial::write( buffer, size ); return size; }

	/** Stream::available() override. @return number of bytes waiting to be read */
	int		available( void ) override { return (int)Serial::available(); }
	/** Stream::read() override. @return next byte, or -1 if none available */
	int		read( void ) override      { return getc(); }
	/** Stream::peek() override. @return next byte without consuming it, or -1 if none available */
	int		peek( void ) override      { return Serial::peek(); }
	/** Block until all outgoing data has actually finished transmitting. */
	void	flush( void )              { Serial::flush(); }
	/** Stream::availableForWrite() override. @return free space in the TX buffer, in bytes */
	int		availableForWrite( void ) override { return (int)Serial::availableForWrite(); }

	/** Always true -- provided for `while (!Serial)`-style sketch compatibility. */
	inline operator bool( void ) { return true; }
};

/** Global Serial instance, USB-CDC-bridged (USBTX/USBRX). */
extern SerialClass	Serial;

// Serial1: hardware UART, separate from the USB-bridged Serial. On D0(RX)/
// D1(TX) on most boards; on FRDM-MCXN947 it's on the MikroBus header
// (MB_TX/MB_RX) instead -- see arduino_serial.cpp for why D0/D1 can't
// support it on that board.
/** Global Serial1 instance -- hardware UART pin pair, board-dependent (see above). */
extern SerialClass	Serial1;

#endif // !R01LIB_ARDUINO_SERIAL_H
