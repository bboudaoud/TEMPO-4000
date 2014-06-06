/*
 * timing.h
 *
 *  Created on: Aug 13, 2013
 *      Author: bb3jd
 */

#ifndef CLOCKS_H_
#define CLOCKS_H_

// Timing definitions for baud rate
#define	DCO_FREQ	8000000				///< DCO frequency (you should call setFLL(DCO_FREQ))
#define	MCLK_FREQ	DCO_FREQ			///< Main clock frequency (change if using MCLK_DIV > 1)
#define SMCLK_FREQ	DCO_FREQ			///< Sub-main clock frequency (change if using SMCLK_DIV > 1)

// Delay macros
#define DCO_MHZ				DCO_FREQ/1000000			///< DCO Rate in MHz
#define DCO_KHZ				DCO_FREQ/1000				///< DCO rate in kHz
#define delay_us(x)			_delay_cycles(x*DCO_MHZ)	///< Blocking delay macro (in us)
#define delay_ms(x)			_delay_cycles(x*DCO_KHZ)	///< Blocking delay macro (in ms)
#define delay_s(x)			_delay_cycles(x*DCO_FREQ)	///< Blocking delay macro (in s)

#endif /* CLOCKS_H_ */
