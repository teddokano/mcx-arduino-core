/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"arduino_serial.h"

// Captured here, before arduino_io.h's #include below redefines MB_TX/MB_RX
// as small Arduino-renumbered indices (they're part of its
// ARDUINO_PIN_RENUMBERING table, unlike USBTX/USBRX). Serial1 needs the raw
// r01lib io.h physical-pin values Serial::resolve_pins()'s s_pinMap[]
// actually matches against -- passing the renumbered ones through
// unconditionally panic()s ("unsupported TX/RX pin combination") at
// static-init time, before setup() even runs (confirmed on real hardware).
#ifdef	CPU_MCXN947VDF
constexpr int	SERIAL1_TX_PIN	= MB_TX;
constexpr int	SERIAL1_RX_PIN	= MB_RX;
#endif

#include	"arduino_io.h"

// Global Arduino-compatible Serial instance(s) (no heap, directly inherit
// r01lib Serial)
// Serial:  USB-bridged UART (USBTX/USBRX, not part of the pin-renumbering table)
SerialClass	Serial(  USBTX, USBRX );

#ifdef	CPU_MCXN947VDF
// Serial1 (D0/D1 hardware UART) is intentionally NOT supported on this board:
// D0/D1's only UART-capable alt-function is FC2_P2/FC2_P3, the same FlexComm2
// instance Wire already uses for I2C -- a FlexComm can only be one peripheral
// mode at a time, so the two can't coexist in a sketch. See
// variants/frdm_mcxn947/README.md for the full writeup.
//
// Serial1 IS supported on the MikroBus header instead (MB_TX/MB_RX, physical
// P1_17/P1_16) via FC5_P1/FC5_P0 -> LPUART5 -- a FlexComm instance nothing
// else on this board claims. These are the same physical pins as I3C_SDA/
// I3C_SCL (used by Wire1) and are also plain GPIO-capable; pinMode()
// explicitly reclaims ALT0 on every call and Wire1/Serial1's begin() each
// explicitly set their own ALT, so all three switch cleanly on the same two
// pins (just not simultaneously).
SerialClass	Serial1( SERIAL1_TX_PIN, SERIAL1_RX_PIN );
#else
// Serial1: hardware UART on D0(RX)/D1(TX) -- arduino_pin_by_number[] converts
//          the renumbered D0/D1 indices back to the raw physical pin macros
//          Serial::resolve_pins() expects (same trick pinMode()/digitalWrite()
//          use).
SerialClass	Serial1( arduino_pin_by_number[ D1 ], arduino_pin_by_number[ D0 ] );
#endif
