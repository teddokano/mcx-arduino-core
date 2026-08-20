/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license
 *
 *  Real Arduino sketches expect to find the SPI API via `#include <SPI.h>`
 *  (capital-cased, matching every official Arduino core's convention).
 *  This project's actual SPIClass implementation lives in arduino_spi.h --
 *  this is a thin, filename-only wrapper so that convention holds here
 *  too. Kept as its own file (rather than renaming arduino_spi.h) since
 *  r01lib's own low-level SPI driver is cores/arduino/r01lib/r01lib_spi.h, and
 *  filesystems that are case-insensitive (macOS, Windows) can't hold both
 *  an "SPI.h" and an "spi.h" in the same directory -- keeping this wrapper
 *  separate from arduino_spi.h avoids ever needing a same-directory
 *  case-only pair.
 */

#ifndef R01LIB_SPI_WRAPPER_H
#define R01LIB_SPI_WRAPPER_H

#include "arduino_spi.h"

#endif // !R01LIB_SPI_WRAPPER_H
