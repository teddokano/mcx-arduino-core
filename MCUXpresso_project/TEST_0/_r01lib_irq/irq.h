/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef R01LIB_IRQ_H
#define	R01LIB_IRQ_H

#if 0
#ifdef	CPU_MCXN947VDF
void GPIO00_IRQHandler( void );
void GPIO10_IRQHandler( void );
void GPIO20_IRQHandler( void );
void GPIO30_IRQHandler( void );
void GPIO40_IRQHandler( void );
void GPIO50_IRQHandler( void );

#else	// CPU_MCXN947VDF
void GPIO0_IRQHandler( void );
void GPIO1_IRQHandler( void );
void GPIO2_IRQHandler( void );
void GPIO3_IRQHandler( void );
#endif	// CPU_MCXN947VDF
#endif	// 0

void irq_handler( int num );

#endif // R01LIB_IRQ_H
