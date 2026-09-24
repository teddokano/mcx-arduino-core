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

/** @name Frame formats for Serial.begin()'s second argument
 *
 *  Same encoding as ArduinoCore-API (the UNO R4, Zephyr and Mbed cores),
 *  so code that builds a format from the SERIAL_DATA_/SERIAL_PARITY_/
 *  SERIAL_STOP_BIT_ parts works too. Only formats the LPUART can produce
 *  are defined: 7 or 8 data bits, no/even/odd parity, 1 or 2 stop bits.
 *  SERIAL_5N1 and friends, mark/space parity and 1.5 stop bits are left
 *  out on purpose, so a sketch asking for one fails to build, naming it,
 *  rather than talking to its device in the wrong format.
 */
///@{
#define	SERIAL_PARITY_EVEN		(0x1ul)
#define	SERIAL_PARITY_ODD		(0x2ul)
#define	SERIAL_PARITY_NONE		(0x3ul)
#define	SERIAL_PARITY_MASK		(0xFul)

#define	SERIAL_STOP_BIT_1		(0x10ul)
#define	SERIAL_STOP_BIT_2		(0x30ul)
#define	SERIAL_STOP_BIT_MASK	(0xF0ul)

#define	SERIAL_DATA_7			(0x300ul)
#define	SERIAL_DATA_8			(0x400ul)
#define	SERIAL_DATA_MASK		(0xF00ul)

#define	SERIAL_7N1				(SERIAL_STOP_BIT_1 | SERIAL_PARITY_NONE | SERIAL_DATA_7)
#define	SERIAL_8N1				(SERIAL_STOP_BIT_1 | SERIAL_PARITY_NONE | SERIAL_DATA_8)
#define	SERIAL_7N2				(SERIAL_STOP_BIT_2 | SERIAL_PARITY_NONE | SERIAL_DATA_7)
#define	SERIAL_8N2				(SERIAL_STOP_BIT_2 | SERIAL_PARITY_NONE | SERIAL_DATA_8)
#define	SERIAL_7E1				(SERIAL_STOP_BIT_1 | SERIAL_PARITY_EVEN | SERIAL_DATA_7)
#define	SERIAL_8E1				(SERIAL_STOP_BIT_1 | SERIAL_PARITY_EVEN | SERIAL_DATA_8)
#define	SERIAL_7E2				(SERIAL_STOP_BIT_2 | SERIAL_PARITY_EVEN | SERIAL_DATA_7)
#define	SERIAL_8E2				(SERIAL_STOP_BIT_2 | SERIAL_PARITY_EVEN | SERIAL_DATA_8)
#define	SERIAL_7O1				(SERIAL_STOP_BIT_1 | SERIAL_PARITY_ODD  | SERIAL_DATA_7)
#define	SERIAL_8O1				(SERIAL_STOP_BIT_1 | SERIAL_PARITY_ODD  | SERIAL_DATA_8)
#define	SERIAL_7O2				(SERIAL_STOP_BIT_2 | SERIAL_PARITY_ODD  | SERIAL_DATA_7)
#define	SERIAL_8O2				(SERIAL_STOP_BIT_2 | SERIAL_PARITY_ODD  | SERIAL_DATA_8)
///@}

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
	 *  A second begin() without end() in between just changes the
	 *  settings. begin() without a format goes back to SERIAL_8N1.
	 *
	 * @param baud baud rate in bps
	 * @param config frame format, one of the SERIAL_8N1-style constants
	 *        above (default SERIAL_8N1). Calls panic() for any other value.
	 */
	void	begin( unsigned long baud, uint16_t config = SERIAL_8N1 );

	/** Wait for pending output to go out, then stop the port: received
	 *  bytes not yet read are dropped, and the TX/RX pins go back to plain
	 *  GPIO inputs, free for pinMode() or another peripheral. begin()
	 *  starts it again. After end(), available() is 0 and read() is -1;
	 *  anything written is discarded.
	 */
	void	end( void ) { Serial::end(); }

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

	/** @name write() of an integer sends its low byte
	 *  As on AVR's HardwareSerial. Without these, `Serial.write(0)` is
	 *  ambiguous: 0 converts to uint8_t and to a const char* equally well. */
	///@{
	size_t	write( unsigned long n ) { return write( (uint8_t)n ); }
	size_t	write( long n )          { return write( (uint8_t)n ); }
	size_t	write( unsigned int n )  { return write( (uint8_t)n ); }
	size_t	write( int n )           { return write( (uint8_t)n ); }
	///@}

	/** Stream::available() override. @return number of bytes waiting to be read */
	int		available( void ) override { return (int)Serial::available(); }
	/** Stream::read() override. @return next byte, or -1 if none available */
	int		read( void ) override      { return getc(); }
	/** Stream::peek() override. @return next byte without consuming it, or -1 if none available */
	int		peek( void ) override      { return Serial::peek(); }
	/** Block until all outgoing data has actually finished transmitting. */
	void	flush( void ) override     { Serial::flush(); }
	/** Stream::availableForWrite() override. @return free space in the TX buffer, in bytes */
	int		availableForWrite( void ) override { return (int)Serial::availableForWrite(); }

	/** Always true -- provided for `while (!Serial)`-style sketch compatibility. */
	inline operator bool( void ) { return true; }
};

/** The class name other cores give Serial/Serial1, for libraries that take
 *  a `HardwareSerial&` (GPS, modem and similar drivers). */
typedef	SerialClass	HardwareSerial;

/** Global Serial instance, USB-CDC-bridged (USBTX/USBRX). */
extern SerialClass	Serial;

// Serial1: hardware UART, separate from the USB-bridged Serial. On D0(RX)/
// D1(TX) on most boards; on FRDM-MCXN947 it's on the MikroBus header
// (MB_TX/MB_RX) instead -- see arduino_serial.cpp for why D0/D1 can't
// support it on that board.
/** Global Serial1 instance -- hardware UART pin pair, board-dependent (see above). */
extern SerialClass	Serial1;

/** Standard cross-core serial-port role aliases, same convention as
 *  ArduinoCore-avr's/ArduinoCore-samd's pins_arduino.h/variant.h -- for
 *  generic sketches/libraries (GPS modules, Bridge-style examples, ...)
 *  written against these names instead of Serial/Serial1 directly.
 *  SERIAL_PORT_USBVIRTUAL is deliberately not defined: that's for cores
 *  with a native USB CDC device in firmware (e.g. SAMD's SerialUSB) --
 *  this board's Serial is a hardware LPUART routed through an external
 *  USB-CDC bridge chip, not a virtual port this core implements, same
 *  reasoning as why ArduinoCore-avr's non-native-USB boards also leave
 *  it undefined.
 */
#define	SERIAL_PORT_MONITOR			Serial
#define	SERIAL_PORT_HARDWARE			Serial1
#define	SERIAL_PORT_HARDWARE_OPEN		Serial1

#endif // !R01LIB_ARDUINO_SERIAL_H
