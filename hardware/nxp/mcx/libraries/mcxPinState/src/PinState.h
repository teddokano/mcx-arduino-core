/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef MCX_PIN_STATE_H
#define MCX_PIN_STATE_H

#include <Arduino.h>

/** Debug utility for mcx-arduino-core: reports which r01lib
 *  peripheral/GPIO objects are currently alive and which physical pins
 *  each one holds, flagging any pin claimed by more than one object at
 *  once. Also cross-checks the pin's actual, currently-live PORT MUX
 *  (ALT) register value against what its (sole) owner says it wanted,
 *  flagging a mismatch -- catching cases where a pin was correctly
 *  claimed at some point but then silently re-muxed to something else
 *  afterward (a real bug this library's own development turned up in
 *  mcx-arduino-core: Serial1's constructor re-muxing I3C's already-
 *  claimed pins on FRDM-MCXN947, well after I3C's own constructor had
 *  already set them and considered the job done).
 *
 *  Depends directly on mcx-arduino-core's internal pin representation --
 *  not usable with any other Arduino core.
 *
 *  Costs nothing unless used: mcx-arduino-core's peripheral classes call
 *  a pair of weak, empty hook functions (pin_registry_note()/
 *  pin_registry_forget(), declared in mcx-arduino-core's own
 *  pin_registry.h) from their constructor/destructor. Those calls are
 *  always present (a few bytes each), but do nothing on their own.
 *  Constructing a PinState object anywhere in a sketch links in this
 *  library's strong override of those two functions -- along with the
 *  actual registry and this print() method -- which is what actually
 *  starts recording ownership. Without a PinState instance, none of that
 *  code is even linked in.
 *
 *  Pins are reported by their physical name ("P1_17" style), synthesized
 *  from data mcx-arduino-core's io.cpp already keeps for itself (no name
 *  string table added there -- see pin_registry_pin_name()'s own comment),
 *  plus the pin's Arduino-level alias in parentheses when it has one
 *  (e.g. "P1_17 (MB_RX)") -- looked up against mcx-arduino-core's own
 *  arduino_pin_by_number[] so the *values* can never drift, from a name
 *  list kept in this library and checked for length against that array at
 *  compile time (see PinState.cpp). That keeps the string-table cost
 *  entirely opt-in: paid only by sketches that construct a PinState, never
 *  added to mcx-arduino-core's own footprint.
 */
class PinState
{
public:
	PinState() = default;

	/** Print a table with one row per *named* pin (every raw pin that
	 *  appears in mcx-arduino-core's arduino_pin_by_number[], i.e. has at
	 *  least one Arduino-level alias) -- not just pins currently claimed
	 *  by a live object. Pins sharing one physical pin (e.g. D10/SPI_CS/
	 *  ARD_CS) are grouped into a single row, their names comma-joined.
	 *
	 *  Columns: Name(s), physical Pin name, live MUX/IBE/ODE/Pull register
	 *  state (all "-" if nothing currently owns the pin), Owner(s)
	 *  (comma-joined if more than one), and Status:
	 *    - "-" if no live object currently claims the pin
	 *    - "OK" if exactly one owner and the live MUX matches what it
	 *      requested
	 *    - "CONFLICT" if more than one live owner claims the pin
	 *    - "MISMATCH" if there's exactly one owner but the live MUX
	 *      register doesn't match the ALT value that owner requested
	 *      (something re-muxed the pin out from under its owner)
	 * @param out stream to print to, defaults to Serial
	 */
	void print( Print &out = Serial ) const;
};

#endif // MCX_PIN_STATE_H
