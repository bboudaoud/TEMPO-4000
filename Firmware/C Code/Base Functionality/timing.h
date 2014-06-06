/*
 * timing.h
 *
 *  Created on: Aug 12, 2013
 *      Author: bb3jd
 */

#ifndef TIMING_H_
#define TIMING_H_

#include <msp430.h>
#include "clocks.h"

// Clock control macros
#define CLOCK_STAB_PERIOD	4*DCO_FREQ	///< Clock stabilization period
#define	FLLN_MASK			0x03FF		///< FLLN Mask for UCSCTL2 Register
#define ACLK_SRC_MASK		0x0700		///< Bit mask for ACLK source control
#define ACLK_DIV_MASK		0x0700		///< Bit mask for ACLK divisor control

typedef enum auxClkSrc {				/// Typedef for auxiliary clock source control
	LFXT = SELA_0,						///< Low-frequency external oscillator (32.768kHz)
	VLO =  SELA_1,						///< On-chip very low-power oscillator (~10-14kHz)
	REFO = SELA_2,						///< On-chip reference oscillator (32.768kHz)
	DCO =  SELA_3,						///< On-chip digitally controlled oscillator
	DCODIV = SELA_4						///< Fixed divisor of on-chip DCO
} aClkSrc;

typedef enum auxClkDiv {				/// Typedef for auxiliary clock source divider
	DIV1 = 0,							///< Divide by 1
	DIV2 = DIVA_1,						///< Divide by 2
	DIV4 = DIVA_2,						///< Divide by 4
	DIV8 = DIVA_3,						///< Divide by 8
	DIV16 = DIVA_4,						///< Divide by 16
	DIV32 = DIVA_5						///< Divide by 32
} aClkDiv;

typedef struct auxClockConfig			/// Typedef for auxiliary clock configuration
{
	aClkSrc src;						///< Source for ACLK
	aClkDiv div;						///< Divisor for ACLK
} aclkConf;

// Prototypes
void setACLK(aclkConf conf);
int clkInit(void);

#endif /* TIMING_H_ */
