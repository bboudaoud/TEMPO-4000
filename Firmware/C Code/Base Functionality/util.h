/*
 * TempoUtil.c
 *
 *  Created on: Dec 16, 2013
 *      Author: bb3jd
 */
#ifndef TEMPO_UTIL_H_
#define TEMPO_UTIL_H_
#include "hal.h"

// Simple defines for toggle and pause cycles for #blink function
#define TOGGLE_CYC	600000		///< Clock cycles to wait between toggle of LED (for #blink)
#define PAUSE_CYC	1000000		///< Clock cycles to wait between LED pulse sets (for #blink)

// Critical Section Code
#define enter_critical(SR_state)           do { \
  (SR_state) = (_get_SR_register() & 0x08); \
  _disable_interrupts(); \
} while (0) ///< Critical section entrance macro

#define exit_critical(SR_state)         _bis_SR_register(SR_state) ///< Critical section exit macro

// System reset code
#define systemReset()           do { \
	WDTCTL = 0 ; \
    _DINT() ; \
    _c_int00(); \
} while (0)	///< Software-based (WDT Violation) System Reset Macro

// Prototypes
/**************************************************************************//**
 * \brief Blink LED rep times using spin loops for timing
 *
 * Record a new timestamp pair into the timing log of the flash card, using
 * the current timing epoch in effect.
 *
 * \param	rep		Number of times to blink the LED
 *****************************************************************************/
void blink(int rep, int led);

/**************************************************************************//**
 * \brief Perform a 16-bit fletcher checksum on a buffer
 *
 * \param		Buffer		A uchar buffer to be checksummed
 * \param		numBytes	Number of bytes to check (up to Buffer's length)
 * \return		16-bit checksum concatenated with sum1 (checkA) as the low
 * 				byte, and sum2 (checkB) as the high byte.
 * \warning		This checksum is insensitive to 0x00 and 0xFF words, and a
 * 				cleared buffer of 0x00s will checksum to 0x00, which may lead
 * 				to situations where the check passes but does not mean there
 * 				is meaningful information in the buffer.
 * \see			http://en.wikipedia.org/wiki/Fletcher's_checksum#Optimizations
 *****************************************************************************/
unsigned int fletcherChecksum(unsigned char *Buffer, int numBytes, unsigned int checksum);

#endif //TEMPO_UTIL_H_
