/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"arduino_serial.h"
#include	"arduino_io.h"

// Global Arduino-compatible Serial instance (no heap, directly inherits r01lib Serial)
// Serial:  USB-bridged UART (USBTX/USBRX, not part of the pin-renumbering table)
SerialClass	Serial(  USBTX, USBRX );

// Serial1 (D0/D1 hardware UART) is NOT ported yet on this board. D0/D1 (ARD_D0/
// ARD_D1, physical P4_3/P4_2) appear to carry FC2_P2/FC2_P3 alt-function pins,
// which could plausibly become LPUART2 -- but the clock-attach/reset symbols and
// pin mux value haven't been identified or verified on real hardware yet.
// r01lib's Serial.cpp only has a single s_pinMap[] entry for this chip (USBTX/
// USBRX -> LPUART4/FC4), so constructing a global Serial1 object here with D0/D1
// pins would panic() (unresolved pin combination) at static-init time, before
// setup() even runs -- confirmed on real hardware via GDB. Add Serial1 back once
// FC2 is wired up and verified.
