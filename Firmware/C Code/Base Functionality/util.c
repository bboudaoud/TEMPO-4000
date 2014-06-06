/*
 * util.c
 *
 *  Created on: Apr 17, 2014
 *      Author: bb3jd
 */
#include "util.h"

/**************************************************************************//**
 * \brief Blink LED rep times using spin loops for timing
 *
 * Record a new timestamp pair into the timing log of the flash card, using
 * the current timing epoch in effect.
 *
 * \param	rep		Number of times to blink the LED
 *****************************************************************************/
void blink(int rep, int led)
{

	if((led != 1) && (led != 2)) return;
	for (; rep > 0; rep--) {
		if(led == 1) LED1_ON();
		else if (led == 2) LED2_ON();

		__delay_cycles(TOGGLE_CYC);

		if(led == 1) LED1_OFF();
		else if (led == 2) LED2_OFF();

		__delay_cycles(TOGGLE_CYC);
	}

	__delay_cycles(PAUSE_CYC);
}

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
unsigned int fletcherChecksum(unsigned char *Buffer, int numBytes, unsigned int checksum)
{

	int len = numBytes;
	unsigned char *data = 0;
	unsigned int sum1, sum2;

	if (checksum == 0) { // Check for non-initialized checksum (0 impossible result)
		sum1 = 0xff;
		sum2 = 0xff;
	} else { // Parse the previous two component sums out of the old checksum
		sum2 = checksum >> 8;
		sum1 = checksum & 0xff;
	}

	data = Buffer;

	while (len) {
		int tlen = len > 20 ? 20 : len; // Require tlen < 20 to avoid second order accumulation overflow
		len -= tlen;
		do {
			sum1 += *data++;
			sum2 += sum1;
		} while (--tlen);

		// The reduction step below is equivalent to a partial modulo 255 (may have a 1 in the upper byte)
		// This works by taking sum1 % 256 then adding a 1 for each 256 in sum1 (sum1/256 = sum1 >> 8)

		sum1 = (sum1 & 0xff) + (sum1 >> 8);
		sum2 = (sum2 & 0xff) + (sum2 >> 8);
	}

	// Second reduction step to assure reduction to 8 bits (no overlap in combination for final checksum value)

	sum1 = (sum1 & 0xff) + (sum1 >> 8);
	sum2 = (sum2 & 0xff) + (sum2 >> 8);

	checksum = (sum2 << 8) | sum1;
	return checksum;
}


