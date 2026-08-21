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
 */
void pin_registry_note( const void *owner, const char *owner_name, const uint8_t *pins, uint8_t pin_count );

/** Called by a pin-owning object's destructor.
 * @param owner same pointer previously passed to pin_registry_note()
 */
void pin_registry_forget( const void *owner );

}	// extern "C"

#endif // R01LIB_PIN_REGISTRY_H
