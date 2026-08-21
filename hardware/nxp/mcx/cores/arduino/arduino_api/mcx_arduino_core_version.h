/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

/** @file mcx_arduino_core_version.h
 *  This package's own release version -- not the Arduino API level
 *  (`ARDUINO`, unrelated) or the board identity (`ARDUINO_FRDM_MCXA153`
 *  etc). MCX_ARDUINO_CORE_VERSION_MAJOR/_MINOR/_PATCH come from
 *  platform.txt's version.major/version.minor/version.patch properties
 *  (compiler.defines), kept next to platform.txt's own `version=` line so
 *  both get bumped together at release time -- there's no way for
 *  platform.txt's build-property substitution to split `{version}` on
 *  its dots itself.
 *
 *  Separate MAJOR/MINOR/PATCH integers (plus MCX_ARDUINO_CORE_VERSION_VAL()
 *  to pack them into one comparable integer) exist so code can gate on a
 *  minimum core version at compile time -- something a plain version
 *  *string* can't do, since the preprocessor's #if only evaluates integer
 *  constant expressions:
 *
 *    #if MCX_ARDUINO_CORE_VERSION >= MCX_ARDUINO_CORE_VERSION_VAL(0, 4, 0)
 *      // use a feature only present from 0.4.0 onward
 *    #endif
 *
 *  Modeled on arduino-esp32's esp_arduino_version.h (MIT-compatible
 *  design pattern, not copied code -- Apache-2.0 original, this is an
 *  independent reimplementation of the same scheme, same reasoning that
 *  applied to this project's own Print/Stream and String classes: the
 *  class/macro *shape* here isn't copyrightable, and the actual
 *  implementation is original).
 */

#ifndef MCX_ARDUINO_CORE_VERSION_H
#define MCX_ARDUINO_CORE_VERSION_H

#ifndef MCX_ARDUINO_CORE_VERSION_MAJOR
#define MCX_ARDUINO_CORE_VERSION_MAJOR	0
#endif

#ifndef MCX_ARDUINO_CORE_VERSION_MINOR
#define MCX_ARDUINO_CORE_VERSION_MINOR	0
#endif

#ifndef MCX_ARDUINO_CORE_VERSION_PATCH
#define MCX_ARDUINO_CORE_VERSION_PATCH	0
#endif

/** Pack major/minor/patch into one integer comparable with </<=/>/>=. */
#define MCX_ARDUINO_CORE_VERSION_VAL( major, minor, patch )	( ( (major) << 16 ) | ( (minor) << 8 ) | (patch) )

/** This build's mcx-arduino-core version, as one comparable integer. */
#define MCX_ARDUINO_CORE_VERSION	MCX_ARDUINO_CORE_VERSION_VAL( MCX_ARDUINO_CORE_VERSION_MAJOR, MCX_ARDUINO_CORE_VERSION_MINOR, MCX_ARDUINO_CORE_VERSION_PATCH )

#define MCX_ARDUINO_CORE_VERSION_XSTR( s )	#s
#define MCX_ARDUINO_CORE_VERSION_STR2( s )	MCX_ARDUINO_CORE_VERSION_XSTR( s )

/** This build's mcx-arduino-core version, as a "MAJOR.MINOR.PATCH" string. */
#define MCX_ARDUINO_CORE_VERSION_STR	MCX_ARDUINO_CORE_VERSION_STR2( MCX_ARDUINO_CORE_VERSION_MAJOR ) "." MCX_ARDUINO_CORE_VERSION_STR2( MCX_ARDUINO_CORE_VERSION_MINOR ) "." MCX_ARDUINO_CORE_VERSION_STR2( MCX_ARDUINO_CORE_VERSION_PATCH )

#endif // MCX_ARDUINO_CORE_VERSION_H
