/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"arduino_serial.h"
#include	"arduino_io.h"

// Global Arduino-compatible Serial instances (no heap, directly inherit r01lib Serial)
// Serial:  USB-bridged UART (USBTX/USBRX, not part of the pin-renumbering table)
// Serial1: hardware UART on D0(RX)/D1(TX) -- arduino_pin_by_number[] converts the
//          renumbered D0/D1 indices back to the raw physical pin macros
//          Serial::resolve_pins() expects (same trick pinMode()/digitalWrite() use).
SerialClass	Serial(  USBTX, USBRX );
SerialClass	Serial1( arduino_pin_by_number[ D1 ], arduino_pin_by_number[ D0 ] );
