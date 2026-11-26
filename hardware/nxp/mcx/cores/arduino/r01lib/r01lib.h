/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 */

/** @file r01lib.h
 *  Single top-level include for the whole r01lib peripheral driver set
 *  (I2C/I3C, SPI, Serial, AnalogIn, PwmOut, GPIO, InterruptIn, BusInOut)
 *  plus mcu.h's chip bring-up/utility functions.
 *
 *  The same file in the upstream r01lib (github.com/teddokano/r01lib) and
 *  in mcx-arduino-core (cores/arduino/r01lib/), where two things differ:
 *  the SPI header there is r01lib_spi.h (Arduino's SPI.h is the same name
 *  as spi.h on a file system that ignores case), and Ticker is left out
 *  of Arduino builds (ARDUINO_ARCH_MCX): Arduino.h includes this file, and
 *  a class named Ticker here kept sketches and libraries from defining
 *  their own (as ESP32's and several timer libraries' Ticker do). A sketch
 *  that uses r01lib's includes "Ticker.h" itself.
 */

#ifndef R01LIB_R01LIB_H
#define R01LIB_R01LIB_H

extern "C" {
#include	"fsl_debug_console.h"
}

#include	<iostream>
#include	<iomanip>

/** When the SDK's debug console is configured to redirect to itself
 *  (SDK_DEBUGCONSOLE == DEBUGCONSOLE_REDIRECT_TO_SDK, set via platform.txt),
 *  route the standard C I/O functions through it so plain printf()/scanf()/
 *  putchar()/getchar() calls work without callers needing to know that
 *  redirection is happening. Otherwise (the default in this project),
 *  SEMIHOST_OPERATION is defined instead and these go through newlib's
 *  plain ARM-semihosting-based implementation -- note this is unrelated
 *  to Serial's own I/O path (SerialClass talks to LPUART hardware
 *  directly, never through printf()/DbgConsole_*).
 */
#if (defined(SDK_DEBUGCONSOLE) && (SDK_DEBUGCONSOLE == DEBUGCONSOLE_REDIRECT_TO_SDK))
#define printf	DbgConsole_Printf
#define scanf	DbgConsole_Scanf
#define putchar	DbgConsole_Putchar
#define getchar	DbgConsole_Getchar
#else
#define		SEMIHOST_OPERATION
#endif

/** Defined on every supported chip except MCXC444 (which has no I3C
 *  peripheral); gates i3c.h's class definition and every CPU_* branch in
 *  the I3C-touching source files.
 */
#ifdef	CPU_MCXC444VLH
#else
#define		I3C_SUPPORTED
#endif

#include	"i3c.h"
#include	"i2c.h"
#if __has_include( "r01lib_spi.h" )
#include	"r01lib_spi.h"
#else
#include	"spi.h"
#endif
#include	"io.h"
#include	"InterruptIn.h"
#include	"BusInOut.h"
#include	"Serial.h"
#include	"AnalogIn.h"
#include	"PwmOut.h"
#include	"mcu.h"
#if !defined( ARDUINO_ARCH_MCX )
#include	"Ticker.h"
#endif

#endif // R01LIB_R01LIB_H
