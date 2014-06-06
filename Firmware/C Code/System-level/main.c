/******************************************************************************
 * TEMPO Firmware (TEMPOS) v4.0
 *
 * Copyright 2014 INERTIA Team
 * http://inertia.ece.virginia.edu
 * Ben Boudaoud
 *
 * This firmware is designed for use with the TEMPO 4000 flash sensor nodes used for
 * wearable human motion capture.
 *****************************************************************************/
#include <msp430.h>
#include "system.h"
#include "hal.h"

void main(void) {
	
	sysInit();						// Initialize the system

	while(1)
	{
		WDTCTL = WDT_ADLY_1000;		// Reset 1s watchdog timer
		evtQueExecute();			// Execute the next event on the queue
		updateState();
	}

	systemReset();					// This should never be exectuted
}
