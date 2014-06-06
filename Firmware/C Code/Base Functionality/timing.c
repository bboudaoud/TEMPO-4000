/*
 * timing.c
 *
 *  Created on: Feb 4, 2013
 *      Author: bb3jd
 */
#include "util.h"
#include "timing.h"
/**********************************************************************//**
 * \brief	Auxiliary Clock Initialization Routine
 *
 * This function sets up ACLK (the auxilary clock) based on the
 * provided config structure
 *
 * \param	aclkConf	conf	A configuration structure for ACLK control
 *************************************************************************/
void setACLK(aclkConf conf)
{
	unsigned int temp = 0;
	unsigned int state;

	enter_critical(state);

	if(conf.src == LFXT) {					// Check for LFXT selected
		UCSCTL6 &= ~(XTS);					// If so assure XTS is cleared (LF-mode selected)
	}

	temp = UCSCTL4 & ~ACLK_SRC_MASK;		// Read in ACLK source control and clear relevant bits
	temp |= conf.src & ACLK_SRC_MASK;		// Write new source bits into temp
	UCSCTL4 = temp;							// Write back temp

	temp = UCSCTL5 & ~ACLK_DIV_MASK;		// Read in ACLK divisor control and clear relevant bits
	temp |= conf.div & ACLK_DIV_MASK;		// Write new divisor bits into temp
	UCSCTL5 = temp;							// Write back temp

	exit_critical(state);
}

/**********************************************************************//**
 * \brief	FLL Initialization Routine
 *
 * This function sets up FLLN (the FLL frequency multiplier) based on the
 * provided target frequency and the on-board LF reference oscillator (REFO)
 *
 * \retval	-1	The FLL multiplier computed was out-of-bounds
 * \returns	The resulting target frequency for the FLL feedback control
 *************************************************************************/
long setFLL(unsigned long TargetFreq)
{
	unsigned int fllMult = (unsigned int)(TargetFreq >> 15); // fllMult = TargetDCO/32768

	// Basic Universal Clock System (UCS) Init
	UCSCTL3 = SELREF_2;						// Set FLL Reference to REF0 (internal reference oscillator)
	UCSCTL4 = SELA_2 + SELS_4 + SELM_4;		// Set ACLK = REF0, SMCLK = MCLK = DCOCLKDIV

	if(fllMult > 1024) 			// If FLL multiplier cannot be represented in 10 bits
		return -1;				// Return failure

	__bis_SR_register(SCG0);	// Disable the FLL control loop
	UCSCTL0 = 0;				// Set lowest possible DCOx and MODx bits

	// DCO Resistor Selection (RSEL)
	if(fllMult <= 30)			// TargetFreq < 1MHz
		UCSCTL1 = DCORSEL_0;
	else if(fllMult <= 62) 		// 1MHz < TargetFreq < 2MHz
		UCSCTL1 = DCORSEL_1;
	else if(fllMult <= 123) 	// 2MHz < TargetFreq < 4MHz
		UCSCTL1 = DCORSEL_2;
	else if(fllMult <= 245) 	// 4MHz < TargetFreq < 8 MHz
		UCSCTL1 = DCORSEL_3;
	else if(fllMult <= 490) 	// 8MHz < TargetFreq < 16MHz
		UCSCTL1 = DCORSEL_4;
	else if(fllMult <= 611)		// 16MHz < TargetFreq < 20MHz
		UCSCTL1 = DCORSEL_5;
	else						// 20MHz < TargetFreq < 33MHz
		UCSCTL1 = DCORSEL_6;

	UCSCTL2 = fllMult & FLLN_MASK;	// Set FLLN (FLL Multiplier)

	__bic_SR_register(SCG0);	// Enable the FLL control loop
	__delay_cycles(CLOCK_STAB_PERIOD);	// Delay and let clock stabilize

	  // Loop until XT1,XT2 & DCO fault flag is cleared
	do {
		UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG); // Clear XT2,XT1,DCO fault flags
		SFRIFG1 &= ~OFIFG;                      	// Clear fault flags
	} while (SFRIFG1&OFIFG);						// Test oscillator fault flag

	return (fllMult << 15);		// Return DCO frequency
}

/**********************************************************************//**
 * \brief	MCLK, SMCLK, and ACLK Init Routine
 *
 * This function uses REFO (the internal reference oscillator) to initialize
 * the DCO to the value targetFreq
 *
 * \retval	-1	The clock initialization failed
 * \retval	0	The clock initialization was successful
 *
 *************************************************************************/
int clkInit(void)
{
	unsigned int state;
	long retval;

	enter_critical(state);
		retval = setFLL(DCO_FREQ);
	exit_critical(state);

	if(retval != -1) retval = 0;
	return (int)retval;
}

