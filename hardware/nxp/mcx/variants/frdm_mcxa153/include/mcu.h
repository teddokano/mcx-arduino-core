/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef R01LIB_MCU_H
#define R01LIB_MCU_H

#include "r01lib.h"

void	init_mcu( void );
void	wait( double delayTime_sec );
void	wait_ms( unsigned int milloseconds );
void	wait_us( unsigned int microseconds );
void 	panic( const char *s );


#endif // R01LIB_MCU_H
