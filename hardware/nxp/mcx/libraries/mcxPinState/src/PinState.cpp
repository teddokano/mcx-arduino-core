/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

#include	<cstdio>
#include	<cstring>
#include	"PinState.h"
#include	"pin_registry.h"

namespace {

// The mcx-arduino-core release ALIAS_NAMES and KNOWN_INSTANCES (below)
// were last hand-checked against, by reading arduino_io.h's own
// arduino_pin_by_number[]/alias macros. Bump this -- after actually
// re-checking both tables -- whenever mcx-arduino-core ships a release
// newer than this.
//
// A #warning, not a #error: most mcx-arduino-core releases won't touch
// pin naming at all, so a newer version alone isn't proof of drift --
// this is a "go check" nudge for whoever's cutting a mcxPinState release
// against a newer core, not a hard build break for every user who simply
// upgraded mcx-arduino-core. The static_assert on ALIAS_NAMES' own length
// (below) still catches the one kind of drift that would otherwise build
// silently wrong: an alias actually added or removed.
//
// MCX_ARDUINO_CORE_VERSION itself is only defined from mcx-arduino-core
// 0.4.0 onward (mcx_arduino_core_version.h) -- on an older core this
// whole check is silently skipped, not an error, since there's nothing
// to compare against.
#define MCXPINSTATE_VERIFIED_AGAINST	MCX_ARDUINO_CORE_VERSION_VAL( 0, 6, 0 )

#if defined( MCX_ARDUINO_CORE_VERSION ) && ( MCX_ARDUINO_CORE_VERSION > MCXPINSTATE_VERIFIED_AGAINST )
#warning "mcxPinState's ALIAS_NAMES/KNOWN_INSTANCES (PinState.cpp) were last verified against an older mcx-arduino-core release than this build -- re-check them against the current arduino_io.h, then bump MCXPINSTATE_VERIFIED_AGAINST"
#endif

constexpr uint8_t	MAX_ENTRIES		= 32;
constexpr uint8_t	MAX_PINS_EACH	= 4;

// Arduino-level alias names (D0, A2, MB_SDA, ...), in exactly the order
// mcx-arduino-core's own arduino_io.h lists them when building
// arduino_pin_by_number[] (index i here <-> arduino_pin_by_number[i]).
//
// Deliberately not sourced from mcx-arduino-core itself: mcx-arduino-core
// keeps no string table of its own (see io.cpp's pin_registry_pin_name(),
// which synthesizes "P1_17"-style physical names from data it already has
// instead of carrying one) precisely so this cost is opt-in, paid only by
// sketches that construct a PinState. The *values* below can't drift --
// they come from arduino_pin_by_number[] itself, which <Arduino.h>
// defines and which this file includes. Only the *list of names/order*
// is hand-maintained here, checked against arduino_io.h's own array
// (hardware/nxp/mcx/cores/arduino/arduino_api/arduino_io.h) whenever
// mcx-arduino-core cuts a release; the static_assert below catches a
// length mismatch (an added/removed alias) at compile time so that check
// can't be skipped and forgotten -- it just won't build. It can't catch a
// same-length re-ordering, but arduino_io.h's array is append-only in
// practice and change history-visible, so that risk is low.
constexpr const char *ALIAS_NAMES[]	=
{
	"D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9",
	"D10", "D11", "D12", "D13", "D18", "D19",
	"A0", "A1", "A2", "A3", "A4", "A5",
	"SW2", "SW3",
	"MB_AN", "MB_RST", "MB_CS", "MB_SCK", "MB_MISO", "MB_MOSI",
	"MB_PWM", "MB_INT", "MB_RX", "MB_TX", "MB_SCL", "MB_SDA",
	"RED", "GREEN", "BLUE",
	"SPI_CS", "SPI_MOSI", "SPI_MISO", "SPI_SCLK",
	"ARD_CS", "ARD_MOSI", "ARD_MISO", "ARD_SCK",
	"PWM0", "PWM1", "PWM2", "PWM3", "PWM4", "PWM5",
};

static_assert(
	sizeof( ALIAS_NAMES ) / sizeof( ALIAS_NAMES[ 0 ] ) == sizeof( arduino_pin_by_number ) / sizeof( arduino_pin_by_number[ 0 ] ),
	"mcxPinState's ALIAS_NAMES is out of sync with mcx-arduino-core's "
	"arduino_pin_by_number[] (arduino_io.h) -- an alias was added or "
	"removed there. Update ALIAS_NAMES (same order) to match." );

constexpr uint8_t	ALIAS_COUNT	= sizeof( arduino_pin_by_number ) / sizeof( arduino_pin_by_number[ 0 ] );

// Every ALIAS_NAMES entry that shares raw_pin's value, comma-joined, in
// ALIAS_NAMES order (e.g. raw_pin == D10's pin -> "D10, SPI_CS, ARD_CS").
// Linear scan is fine here: this only runs inside print(), a few dozen
// entries, never in a hot path.
void names_for( uint8_t raw_pin, char *buf, uint8_t buf_size )
{
	uint8_t	pos		= 0;
	bool	first	= true;

	buf[ 0 ]	= '\0';

	for ( uint8_t i = 0; i < ALIAS_COUNT && pos < buf_size; i++ )
	{
		if ( arduino_pin_by_number[ i ] != raw_pin )
			continue;

		int	n	= snprintf( buf + pos, buf_size - pos, "%s%s", first ? "" : ", ", ALIAS_NAMES[ i ] );

		if ( n < 0 )
			break;

		pos		+= (uint8_t)n;
		first	= false;
	}
}

struct Entry
{
	const void	*owner					= nullptr;
	const char	*name					= nullptr;
	uint8_t		pins[ MAX_PINS_EACH ]	= {};
	uint8_t		count					= 0;
	uint8_t		wanted_mux				= 0;
	bool		in_use					= false;
};

Entry	registry[ MAX_ENTRIES ];

// How many live registry entries currently claim raw_pin, with their
// names comma-joined into buf ("-" if none). If exactly one owner,
// *wanted_mux_out is set to that owner's requested ALT (untouched
// otherwise -- callers that only care about the single-owner case should
// check the returned count first).
int owners_of( uint8_t pin, char *buf, uint8_t buf_size, uint8_t *wanted_mux_out )
{
	int		owners	= 0;
	uint8_t	pos		= 0;

	buf[ 0 ]	= '\0';

	for ( const Entry &e : registry )
	{
		if ( !e.in_use )
			continue;

		for ( uint8_t j = 0; j < e.count; j++ )
		{
			if ( e.pins[ j ] != pin )
				continue;

			int	n	= snprintf( buf + pos, buf_size - pos, "%s%s", (owners > 0) ? ", " : "", e.name );

			if ( n > 0 )
				pos	+= (uint8_t)n;

			if ( wanted_mux_out )
				*wanted_mux_out	= e.wanted_mux;

			owners++;
			break;
		}
	}

	if ( 0 == owners )
		snprintf( buf, buf_size, "-" );

	return owners;
}

// Whether raw_pin is currently claimed specifically *for* expected_name's
// peripheral function -- not just claimed by anything.
//
// Checking registry ownership by pin alone isn't enough: a good many of
// these known-instance pins double as an ordinary D-numbered pin (e.g.
// SPI's default SCLK is also D13), so something as unrelated as
// `tone(D13, ...)` toggling it as plain GPIO would otherwise read as
// "SPI has claimed its SCLK pin" (found via real hardware testing:
// CombinedPeripheralsAudit's tone() call on D13 made SPI show up as
// PARTIAL even though SPI.begin() was never called). Matching
// owner_name rules out unrelated owners for SPI/Serial (which register
// under those specific class-level labels), but not for the I2C-family
// instances (Wire/Wire1/Wire2's SDA/SCL are DigitalInOut objects, so
// they -- like any plain pinMode() pin -- register generically as
// "GPIO"; see owners_of()'s own comment). wanted_mux != 0 closes that
// gap: DigitalInOut's constructor always registers a fresh plain-GPIO
// pin with wanted_mux == 0 (ALT0, this whole core's universal "plain
// GPIO" convention), while I2C/I3C's begin() re-muxes its SDA/SCL to a
// real peripheral ALT and updates the registry to match (see
// DigitalInOut::pin_mux()'s own history). So requiring both together is
// what actually distinguishes "this specific peripheral is really
// live" from "something else happens to be touching the same pin".
bool owned_by( uint8_t pin, const char *expected_name )
{
	for ( const Entry &e : registry )
	{
		if ( !e.in_use )
			continue;

		for ( uint8_t j = 0; j < e.count; j++ )
			if ( (e.pins[ j ] == pin) && (0 == strcmp( e.name, expected_name )) && (e.wanted_mux != 0) )
				return true;
	}

	return false;
}

// mcx-arduino-core's well-known global peripheral instances, identified
// here only by the pin(s) each one is defined to use -- not by address,
// since telling e.g. Wire and Wire1 apart by matching a registry entry's
// owner pointer against &Wire/&Wire1 would need those pin-owning
// DigitalInOut objects to register under the wrapping TwoWire/SPIClass/
// SerialClass instance's own `this`, which they don't (I2C/I3C's SDA/SCL
// register under their own DigitalInOut's `this`, generically as "GPIO";
// see this library's own development history in the README for how that
// was found). Checking "are this instance's own known pins present in
// the registry at all" sidesteps that entirely, and needs no changes on
// the mcx-arduino-core side.
//
// Pin values: the four *_SDA/*_SCL macros are fixed to raw pin values
// unconditionally by arduino_io.h (deliberately, precisely to close off
// this kind of "which value does this name mean right now" question --
// see mcx-arduino-core's own history around the r01lib_I3C SOS-panic
// bug), so they're used directly. Everything else here went through the
// normal ArduinoPinNum renumbering, so arduino_pin_by_number[] converts
// it back to the raw value pin_registry_note() actually deals in, same
// as names_for() above.
struct KnownInstance
{
	const char	*name;
	const char	*owner_name;	// class-level label this instance's pins register under -- see owned_by()
	uint8_t		pin[ 3 ];
	uint8_t		pin_count;
};

const KnownInstance KNOWN_INSTANCES[]	=
{
	{ "Wire",    "GPIO",   { (uint8_t)I2C_SDA, (uint8_t)I2C_SCL, 0 }, 2 },
	{ "Wire1",   "GPIO",   { (uint8_t)I3C_SDA, (uint8_t)I3C_SCL, 0 }, 2 },
#if defined( FRDM_MCXN947 )
	// A153 has no Wire2 at all: a single physical I2C peripheral, already
	// spoken for by Wire, makes a genuinely independent third I2C bus
	// impossible on that board.
	{ "Wire2",   "GPIO",   { (uint8_t)arduino_pin_by_number[ MB_SDA ], (uint8_t)arduino_pin_by_number[ MB_SCL ], 0 }, 2 },
#endif
	{ "SPI",     "SPI",    { (uint8_t)arduino_pin_by_number[ ARD_MOSI ], (uint8_t)arduino_pin_by_number[ ARD_MISO ], (uint8_t)arduino_pin_by_number[ ARD_SCK ] }, 3 },
	{ "SPI1",    "SPI",    { (uint8_t)arduino_pin_by_number[ MB_MOSI ], (uint8_t)arduino_pin_by_number[ MB_MISO ], (uint8_t)arduino_pin_by_number[ MB_SCK ] }, 3 },
	{ "Serial",  "Serial", { (uint8_t)USBTX, (uint8_t)USBRX, 0 }, 2 },
#if defined( FRDM_MCXA153 )
	{ "Serial1", "Serial", { (uint8_t)arduino_pin_by_number[ D0 ], (uint8_t)arduino_pin_by_number[ D1 ], 0 }, 2 },
#elif defined( FRDM_MCXN947 )
	// On N947, Serial1 lives on the MikroBus header's MB_TX/MB_RX pins --
	// the same two physical pins as Wire1's I3C_SDA/I3C_SCL. Serial always
	// registers as owner_name "Serial" regardless of which pins (its own
	// apply_pin_mux() calls pin_registry_note() directly, bypassing
	// DigitalInOut -- unlike I2C/I3C's SDA/SCL, which is why Serial's pins
	// never show a separate "GPIO" registration alongside "Serial" in the
	// pin table above), so owned_by("Serial") still correctly tells this
	// row apart from Wire1's ("GPIO") on the very same physical pins. The
	// two can't really be begun at once (whichever muxes the pins last
	// wins the physical bus) -- if both show up claimed simultaneously,
	// owners_of()'s raw, name-agnostic count catches it as CONFLICT below.
	{ "Serial1", "Serial", { (uint8_t)arduino_pin_by_number[ MB_TX ], (uint8_t)arduino_pin_by_number[ MB_RX ], 0 }, 2 },
#endif
};

}	// namespace

// Strong overrides of mcx-arduino-core's weak pin_registry_note()/
// pin_registry_forget() -- see PinState.h for why constructing a
// PinState object is what makes this translation unit (and so these
// definitions) link in at all.
extern "C" {

void pin_registry_note( const void *owner, const char *owner_name, const uint8_t *pins, uint8_t pin_count, uint8_t wanted_mux )
{
	Entry	*slot	= nullptr;

	// Re-registering the same owner (e.g. a pin re-muxed after begin())
	// reuses its existing slot rather than leaking a second one.
	for ( Entry &e : registry )
	{
		if ( e.in_use && e.owner == owner )
		{
			slot	= &e;
			break;
		}
	}

	if ( !slot )
	{
		for ( Entry &e : registry )
		{
			if ( !e.in_use )
			{
				slot	= &e;
				break;
			}
		}
	}

	// Table full -- this is a diagnostic aid, not something that should
	// be able to crash or panic a sketch, so just drop the registration.
	if ( !slot )
		return;

	slot->owner			= owner;
	slot->name			= owner_name;
	slot->count			= ( pin_count > MAX_PINS_EACH ) ? MAX_PINS_EACH : pin_count;
	slot->wanted_mux	= wanted_mux;

	for ( uint8_t i = 0; i < slot->count; i++ )
		slot->pins[ i ]	= pins[ i ];

	slot->in_use	= true;
}

void pin_registry_forget( const void *owner )
{
	for ( Entry &e : registry )
	{
		if ( e.in_use && e.owner == owner )
		{
			e.in_use	= false;
			e.owner		= nullptr;
			return;
		}
	}
}

}	// extern "C"

void PinState::print( Print &out ) const
{
	out.println( "=== Pin MUX state (named pins only) ===" );
	out.println( "Name(s)                    Pin      MUX  IBE  ODE  Pull  Owner        Status" );
	out.println( "--------------------------------------------------------------------------------" );

	uint8_t	seen[ ALIAS_COUNT ];
	uint8_t	seen_count	= 0;

	for ( uint8_t i = 0; i < ALIAS_COUNT; i++ )
	{
		uint8_t	pin		= (uint8_t)arduino_pin_by_number[ i ];

		// io.h's pin enum puts DISABLED_PIN (an alias with no physical pin
		// on this board at all, e.g. N947's A0/A1/MB_AN) at value 0, always
		// -- not a real, shared pin, so grouping every disabled alias into
		// one misleading "they share a pin" row (and reporting live PCR
		// state for pin 0, which is meaningless) would be worse than just
		// leaving them out of a table that's specifically about pins.
		if ( 0 == pin )
			continue;

		bool	already	= false;

		for ( uint8_t s = 0; s < seen_count; s++ )
		{
			if ( seen[ s ] == pin )
			{
				already	= true;
				break;
			}
		}

		if ( already )
			continue;

		seen[ seen_count++ ]	= pin;

		char	names_buf[ 40 ];
		char	pin_buf[ 12 ];
		char	owner_buf[ 24 ];

		names_for( pin, names_buf, sizeof( names_buf ) );
		pin_registry_pin_name( pin, pin_buf, sizeof( pin_buf ) );

		PinPcrInfo	pcr			= pin_registry_read_pcr( pin );
		uint8_t		actual_mux	= pcr.mux;
		bool		valid		= actual_mux != 0xFF;

		uint8_t	single_wanted	= 0;
		int		owners			= owners_of( pin, owner_buf, sizeof( owner_buf ), &single_wanted );

		const char	*status;

		if ( owners == 0 )
			status	= "-";
		else if ( owners > 1 )
			status	= "CONFLICT";
		else if ( !valid || actual_mux != single_wanted )
			status	= "MISMATCH";
		else
			status	= "OK";

		char	line[ 100 ];

		if ( valid )
		{
			char	ibe_s[ 3 ]	= "-";
			char	ode_s[ 3 ]	= "-";
			char	pull_s[ 3 ]	= "-";

			if ( pcr.ibe )
				snprintf( ibe_s, sizeof( ibe_s ), "ON" );
			if ( pcr.ode )
				snprintf( ode_s, sizeof( ode_s ), "ON" );
			if ( pcr.pull == 1 )
				snprintf( pull_s, sizeof( pull_s ), "PD" );
			else if ( pcr.pull == 2 )
				snprintf( pull_s, sizeof( pull_s ), "PU" );

			snprintf( line, sizeof( line ), "%-26s %-8s %-4u %-4s %-4s %-5s %-12s %s",
				names_buf, pin_buf, actual_mux, ibe_s, ode_s, pull_s, owner_buf, status );
		}
		else
		{
			snprintf( line, sizeof( line ), "%-26s %-8s %-4s %-4s %-4s %-5s %-12s %s",
				names_buf, pin_buf, "-", "-", "-", "-", owner_buf, status );
		}

		out.println( line );
	}

	out.println();
	out.println( "=== Peripheral instance state ===" );
	out.println( "Instance     begun()?  Holds pins                       Status" );
	out.println( "----------------------------------------------------------------------" );

	for ( const KnownInstance &k : KNOWN_INSTANCES )
	{
		char	holds_buf[ 48 ];
		uint8_t	pos			= 0;
		uint8_t	held		= 0;
		bool	conflict	= false;

		holds_buf[ 0 ]	= '\0';

		for ( uint8_t i = 0; i < k.pin_count; i++ )
		{
			char	owner_buf[ 24 ];
			int		owners	= owners_of( k.pin[ i ], owner_buf, sizeof( owner_buf ), nullptr );

			if ( owners > 1 )
				conflict	= true;

			if ( !owned_by( k.pin[ i ], k.owner_name ) )
				continue;

			char	pin_name[ 12 ];

			held++;
			pin_registry_pin_name( k.pin[ i ], pin_name, sizeof( pin_name ) );

			int	n	= snprintf( holds_buf + pos, sizeof( holds_buf ) - pos, "%s%s", (pos > 0) ? ", " : "", pin_name );

			if ( n > 0 )
				pos	+= (uint8_t)n;
		}

		if ( 0 == pos )
			snprintf( holds_buf, sizeof( holds_buf ), "-" );

		const char	*begun_s	= (held == k.pin_count) ? "yes" : "no";
		const char	*status;

		if ( conflict )
			status	= "CONFLICT";
		else if ( 0 == held )
			status	= "-";
		else if ( held < k.pin_count )
			status	= "PARTIAL";
		else
			status	= "OK";

		char	line[ 90 ];

		snprintf( line, sizeof( line ), "%-12s %-9s %-32s %s", k.name, begun_s, holds_buf, status );
		out.println( line );
	}
}
