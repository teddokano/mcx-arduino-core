/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#ifndef R01LIB_PIN_REGISTRY_H
#define R01LIB_PIN_REGISTRY_H

#include <cstdint>

/** Pin-ownership debug hooks (see the mcxPinState library, a separate
 *  project: https://github.com/teddokano/mcxPinState).
 *
 *  These are weak, empty by default -- pin-owning r01lib classes
 *  (DigitalInOut and friends) call them unconditionally from their
 *  constructor/destructor, but with no sketch pulling in a strong
 *  override, the calls cost only a few bytes per call site and touch
 *  nothing else. Constructing a mcxPinState::PinState object anywhere in
 *  a sketch links in that library's strong override instead (same
 *  mechanism as this core's own weak main(), which a sketch's own main()
 *  overrides the same way) -- from that point on every call here actually
 *  records who holds which pin, so mcxPinState can report it.
 *
 *  Deliberately NOT part of this core's own debug-output story: the real
 *  registry (the array, the table-printing logic) lives entirely in
 *  mcxPinState, outside this core's cores/arduino/ tree -- which is
 *  force-linked whole-archive (see platform.txt's recipe.c.combine.pattern)
 *  precisely so weak/strong overrides like main() resolve correctly. Had
 *  the registry lived in cores/arduino/ instead, that same whole-archive
 *  flag would have pulled it into every build whether any sketch
 *  constructed a PinState or not, defeating the entire point.
 */
extern "C" {

/** Called by a pin-owning object's constructor once it has resolved and
 *  claimed its pin(s).
 * @param owner opaque identity for this object (its own `this`) -- used
 *              only to pair a later pin_registry_forget() call with this
 *              one, never dereferenced
 * @param owner_name short label for the object's class (e.g. "GPIO",
 *                   "Serial1", "SPI") -- a string literal is fine, this
 *                   is never freed
 * @param pins raw r01lib pin values (io.h's pin enum) this object holds
 * @param pin_count number of entries in `pins`
 * @param wanted_mux the PORT_MuxAltN value (0-15) this object just set --
 *                   or was expecting to already be set to -- on all of
 *                   `pins`. Every current caller uses a single shared ALT
 *                   across its whole pin group (e.g. Serial's TX/RX both
 *                   land on the same LPUART ALT, SPI's MOSI/MISO/SCLK all
 *                   share one ALT), so one scalar is enough for now; a
 *                   future caller needing per-pin ALTs would need this
 *                   signature extended.
 */
void pin_registry_note( const void *owner, const char *owner_name, const uint8_t *pins, uint8_t pin_count, uint8_t wanted_mux );

/** Called by a pin-owning object's destructor.
 * @param owner same pointer previously passed to pin_registry_note()
 */
void pin_registry_forget( const void *owner );

}	// extern "C"

/** A raw pin's PCR register, decoded into the fields pin-ownership debug
 *  tooling cares about.
 */
struct PinPcrInfo
{
	uint8_t	mux;	/**< current PORT_MuxAltN value (0-15), or 0xFF if the pin is DISABLED_PIN or otherwise out of range (ibe/ode/pull are meaningless in that case) */
	bool	ibe;	/**< input buffer enabled -- without this, reads/incoming-signal detection on the pin don't work even if mux is otherwise correct (see this core's own history: I3C1_SDA/SCL's mux was right but IBE was left disabled, and every read NAK'd) */
	bool	ode;	/**< open-drain output enabled -- always false on chips whose PCR has no ODE field */
	uint8_t	pull;	/**< 0 = no pull resistor, 1 = pull-down, 2 = pull-up */
};

/** Read back a raw pin's live PCR register, decoded.
 *
 *  The `mux` field is for comparing against what an owner in the registry
 *  claimed to want (pin_registry_note()'s wanted_mux), to catch cases
 *  where something re-muxed the pin out from under its owner after the
 *  fact (see this core's own history for a real example: Serial1
 *  silently re-muxing I3C's pins on FRDM-MCXN947, well after I3C's own
 *  constructor had already set them and considered the job done).
 *
 *  Deliberately NOT extern "C"/weak like the two hooks above -- this
 *  doesn't need overriding, it's just a plain read. Defined in io.cpp,
 *  which is where the per-board pin/PORT lookup tables this needs already
 *  live; --gc-sections drops it from any build that never calls it, the
 *  same as every other unused function here.
 *
 * @param raw_pin r01lib's internal pin number (io.h's pin enum)
 * @return the pin's decoded PCR state; `mux` is 0xFF if the pin is
 *         DISABLED_PIN or otherwise out of range
 */
PinPcrInfo pin_registry_read_pcr( uint8_t raw_pin );

#endif // R01LIB_PIN_REGISTRY_H
