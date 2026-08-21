/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	"pin_registry.h"

// Weak, empty defaults -- see pin_registry.h for why these live here and
// what overrides them.
extern "C" {

void __attribute__((weak)) pin_registry_note( const void *owner, const char *owner_name, const uint8_t *pins, uint8_t pin_count, uint8_t wanted_mux )
{
	(void)owner;
	(void)owner_name;
	(void)pins;
	(void)pin_count;
	(void)wanted_mux;
}

void __attribute__((weak)) pin_registry_forget( const void *owner )
{
	(void)owner;
}

}	// extern "C"
