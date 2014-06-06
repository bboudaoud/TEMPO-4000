/*
 * interrupts.c
 *
 *  Created on: Apr 8, 2014
 *      Author: bb3jd
 */
#include <msp430.h>
#include "mpu.h"
#include "hal.h"

// Callback function pointers for interrupts
void (*mpuPtr)(void);
void (*sw1Ptr)(void);
void (*sw2Ptr)(void);

void dummyCallback(void){
	_NOP();
	return;
}

void chargingIntCfg(bool en)
{
	unsigned int stat;
	enter_critical(stat);

	CHG_CONFIG();

	if(en){
		P1IE |= BIT5;		// Enable interrupts on pin 1.5
		P1IES |= BIT5;		// Set interrupt on high->low transition
	}
	else{
		P1IE &= ~BIT5;		// Disable interrupts on pin 1.5
	}

	exit_critical(stat);
}

void mpuIntPinCfg(bool en)
{
	unsigned int stat;
	enter_critical(stat);

	MPU_INT_CONFIG();		// Set up pin for MPU interrupt as input

	if(en){
		P1IE |= BIT7;		// Enable interrupts on pin 1.7
		P1IES &= ~BIT7;		// Set interrupt on low->high transition
	}
	else{
		P1IE &= ~BIT7;		// Disable interrupts on pin 1.7
	}
	exit_critical(stat);
}

void registerMPUCallback(void *f)
{
	mpuPtr = f;
}

void clearMPUCallback(void)
{
	mpuPtr = dummyCallback;
}

void sw1IntCfg(bool en)
{
	unsigned int stat;
	enter_critical(stat);

	SW1_CONFIG();			// Set up pin for switch input

	if(en){
		P1IE |= BIT2;		// Enable interrupts on pin 1.2
		P1IES |= BIT2;		// Set interrupt on high->low transition
	}
	else{
		P1IE &= ~BIT2;		// Disable interrupts on pin 1.2
	}
	exit_critical(stat);
}

void registerSW1Callback(void *f(void))
{
	sw1Ptr = f;
}

void clearSW1Callback(void)
{
	sw1Ptr = dummyCallback;
}

void sw2IntCfg(bool en)
{
	unsigned int stat;
	enter_critical(stat);

	SW2_CONFIG();			// Set up pin for switch input

	if(en){
		P1IE |= BIT3;		// Enable interrupts on pin 1.3
		P1IES |= BIT3;		// Set interrupt on high->low transition
	}
	else{
		P1IE &= ~BIT3;		// Disable interrupts on pin 1.3
	}
	exit_critical(stat);
}

void registerSW2Callback(void *f(void))
{
	sw2Ptr = f;
}

void clearSW2Callback(void)
{
	sw2Ptr = dummyCallback;
}

// PORT 1 ISR and affiliate callback structure
#pragma vector=PORT1_VECTOR
__interrupt void port1isr(void)
{
	if(P1IFG & BIT2){	// SW1 Interrupt
		P1IFG &= ~BIT2;
		sw1Ptr();
	}
	if(P1IFG & BIT3){	// SW2 Interrupt
		P1IFG &= ~BIT3;
		sw2Ptr();
	}
	if(P1IFG & BIT7){	// MPU Interrupt
		P1IFG &= ~BIT7;
		mpuPtr();
	}
}





