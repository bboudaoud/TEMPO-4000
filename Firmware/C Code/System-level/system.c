/*
 * system.c
 *
 *  Created on: Jan 6, 2014
 *      Author: bb3jd
 */
#include <msp430.h>
#include "system.h"
#include "command.h"
#include "infoflash.h"
#include "filesystem.h"
#include "timing.h"
#include "rtc.h"
#include "mpu.h"
#include "hal.h"
#include "ftdi.h"
#include "comm.h"

const Version v = {4,0,1,0};
unsigned int nodeID = 0;

sampInfo samp = {100, AXIS_XYZ_6DOF};
sessStatus closeCause;
cmdPkt* cmd;							///< Command packet to pass to #runCommand() from #updateState()

// Management flags
unsigned char sensBuffFull = 0;
unsigned char cardFull = 0;
unsigned char lowV = 0;
unsigned char highT = 0;
unsigned char sessionInProgress = 0;
unsigned char dataCollectionEn = 0;

sysState state = STATE_NULL;
unsigned long sampleCount = 0;

// Event queue management variables
volatile sysEvent evtQue[EVT_QUE_SIZE];			///< Event queue
unsigned int evtQuePutIdx = 0;					///< Event queue put index
unsigned int evtQueGetIdx = 0;					///< Event queue get index
unsigned int evtQueCount = 0;					///< Event queue length (event count)
unsigned int evtQueEmptyCount = 0;				///< Event queue empty counter

/**************************************************************************//**
 * \brief Setup port direction, selection, and (sometimes) values
 *
 * Initialize ports for operation. Mostly relies on HAL-level definitions for
 * static setup of I/O for items like the LED, switches, charge indicator, MPU,
 * and MMC card.
 *****************************************************************************/
void ioConfig(void)
{
	LED_CONFIG();				// Configure I/O for 2 LEDs (off)
	SW_CONFIG();				// Configure I/O for 2 switches (internal pull-ups)
	EXT_VCC_CONFIG();			// Configure I/O for external reg control (off)
	CHG_CONFIG();				// Configure I/O for charge detection
	//USB_VCC_CONFIG();			// Configure I/O for USB connection detection

	MPU_IO_CONFIG();			// Configure I/O for MPU communications
	MMC_IO_CONFIG();			// Configure I/O for MMC communications

	MCLK_CONFIG();				// Route out MCLK on P4.6 for testing

	chargingIntCfg(True);		// Enable charging interrupt
}

/**************************************************************************//**
 * \brief Initialize the system (after reset)
 *
 * "Boot" up the system, initializing flags, modules, peripherals, ports,
 * buffers, etc., etc.
 *****************************************************************************/
int sysInit(void)
{
	unsigned int retval = 0;

	_disable_interrupts();								// Disable interrupts
	WDTCTL = WDTPW + WDTHOLD;							// Disable watchdog timer

	if(infoInit() == INFO_VALID) {							// Load flags from info flash if marked valid (otherwise do nothing until init)
		if(infoCheckCritical() != INFO_VALID) {				// Check for critical flags
			_bis_SR_register(LPM4_bits);					// Enter LPM4 w/o interrupts on failure
			while(1);
		}
		nodeID = infoGetNodeID();						// Load the node ID number from info flash
	}

	state = STATE_BOOT;									// Only AFTER info is valid set in boot state
	ioConfig();											// Set up system I/O

	blink(1,1);						// BLINK 1: Clock Initialization
		if(clkInit() == -1){							// Run clock init (set MCLK to DCOCLK see timing.c, clocks.h)
			LPM4;										// Enter LPM4 w/o interrupts on failure
			while(1);
		}

	_enable_interrupts();			// Re-enable interrupts

	blink(2,1);						// BLINK 2: RTC Initialization
		// TODO: add an init time here
		//rtcInit();
	blink(3,1);
		mpuInit();					// BLINK 3: MPU Initialization
		mpuSleepEn(True);
	blink(4,1);						// BLINK 4: File System Resume
		if(fsResume() != FS_SUCCESS){
			retval = -3;
		}
	blink(5,1);						// BLINK 5: All is well

	evtQueInit();					// Clear the event queue
	state = STATE_CMD;				// Set the node in the command state

	return retval;
}

/**************************************************************************//**
 * \brief Put the system to sleep
 *
 * "Sleep" the system, first clear any un-used output ports, the event queue, and
 * set the state to #STATE_SLEEP
 *****************************************************************************/
void sysSleep(void)
{
	WDTCTL = WDTPW + WDTHOLD;		// Disable the WDT

	LEDS_OFF();						// Turn off all LEDs
	MCLK_OFF();						// Turn off MCLK output

	evtQueInit();					// Clear the event queue
	mpuSleepEn(True);				// Put MPU to sleep

	sw1IntCfg(True);
	sw2IntCfg(True);
	registerSW1Callback(sysWake);	// Set up SW1 for wake-up callback
	registerSW2Callback(sysWake);	// Set up SW2 for wake-up callback
	mpuSleepEn(True);				// Put the MPU to sleep
	clearMPUCallback();

	state = STATE_SLEEP;			// Set the system state
	LPM3;							// Goto LPM3: interrupt wake up
}

/**************************************************************************//**
 * \brief Wake the system from sleep to start taking data or enter command
 *
 * Wake up, blink the LED 3 times, then exit LPM3 and determine next task
 * to execute.ki
 *****************************************************************************/
void sysWake(void)
{
	_disable_interrupts();			// Disable interrupts
	WDTCTL = WDTPW + WDTHOLD;		// Disable watchdog timer

	blink(3,1);						// Blink the green LED 3 times
	_bic_SR_register(LPM3_bits);	// Wake the device (clear LPM3 bits)

	if(CHARGING){					// Check for charge signal
		evtQuePut(SYS_ENTER_CMD);
		state = STATE_CMD;
	}
	else if(!CHARGING && dataCollectionEn){		// Check for dataCollectionEn
		evtQuePut(SYS_START_SESSION);
	}
	else {
		evtQuePut(SYS_ENTER_SLEEP);				// Else return to sleep
	}
	_enable_interrupts();
}

/**************************************************************************//**
 * \brief Start a new data session
 *
 * Set up the MPU for taking a data session, interrupts on the MSP, and file
 * system for storing the incoming data then set the state to #STATE_COLLECTING
 *****************************************************************************/
void sysStartSession(void)
{
	unsigned int stat;
	enter_critical(stat);

	LEDS_ON();							// Turn both LEDs on
	delay_ms(100);						// Delay for 100 ms
	LEDS_OFF();							// Turn both LEDs off

	_disable_interrupts();
	if((!CHARGING) && dataCollectionEn){						// Check for off charger and data collection enabled
		// Set up SW1 interrupt for pause
		registerSW1Callback(sysPause);
		sw1IntCfg();

		// Set up SW2 interrupt for end session
		registerSW2Callback(sysClose);
		sw2IntCfg();

		// Set system sampling function as MPU call back
		registerMPUCallback(sysSample);
		mpuIntPinCfg(True);
		mpuSleepEn(False);									// Wake up the MPU for collection
	_enable_interrupts();

		fsStartSession(samp.SR, samp.axis, rtcGetTime());	// Start a session
		sessionInProgress = 1;								// Set the session in progress flag
		state = STATE_COLLECTING;							// Set the state to in collection
	}
	else{
		blink(3, 2);										// Blink red LED 3 times to indicate did not start new data session
		state = STATE_CMD;									// Return to command state
	}

	exit_critical(stat);
}

/**************************************************************************//**
 * \brief End a data session
 *
 * This function ends the current data session being stored to the card and set
 * the system state to #STATE_END_SESSION
 *****************************************************************************/
void sysEndSession(void)
{
	unsigned int stat;
	enter_critical(stat);

	fsEndSession(rtcGetTime(), closeCause, True);		// End the session and update the card info
	// Clear interrupt callbacks
	clearSW1Callback();
	clearSW2Callback();
	clearMPUCallback();

	sessionInProgress = 0;				// Clear the session in progress flag
	// Check for next state to enter
	if(!CHARGING){
		state = STATE_IDLE;
	}
	else
		state = STATE_CMD;

	exit_critical(stat);
}

/**************************************************************************//**
 * \brief Sample the MPU on an interrupt
 *
 * This function is used as a callback for the MPU data ready event. It checks
 * for the desired axes to sample and performs sampling on these channels. It
 * also provides critical flag updates for buffer overrun and event queue overflow
 *****************************************************************************/
void sysSample(void)
{

	axisData d = {0,0,0};
	fsRetCode retval;
	int i = 0;

	if(!sessionInProgress) return;				// Assure a session is in progress

	// Check/record any accelerometer channels
	if(samp.axis.x_acc | samp.axis.y_acc | samp.axis.z_acc){
		d = mpuGetAccel();
		retval = fsWriteData((unsigned char *)&d, sizeof(axisData));
	}
	if(retval == FS_FAIL_BF){
		sensBuffFull = 1;
		return;
	}

	// Check/record any gyro channels
	if(samp.axis.x_gyro | samp.axis.y_gyro | samp.axis.z_gyro){
		d = mpuGetGyro();
		retval = fsWriteData((unsigned char *)&d, sizeof(axisData));
	}
	if(retval == FS_FAIL_BF){
		sensBuffFull = 1;
		return;
	}

	// Check/record the temperature channel
	if(samp.axis.temp){
		i = mpuGetTemp();
		retval = fsWriteData((unsigned char *)&i, sizeof(int));
	}
	if(retval == FS_FAIL_BF){
		sensBuffFull = 1;
		return;
	}
	// Check for a card full failure
	if(retval == FS_FAIL_CF){
		cardFull = 1;							// Set the cardfull indicator flag
		evtQuePut(SYS_END_SESSION);				// Add an end-session to the event queue
		return;
	}
	else {
		sampleCount++;							// Increment sample counter
		sampleCount %= BLINK_COUNT;				// Wrap sample counter
		if(sampleCount== 0){					// Check for counter wrap around
			LED_GREEN_TOGGLE();					// Toggle the LED to indicate sampling
		}
	}
	return;
}

/**************************************************************************//**
 * \brief Pause the system during data collection
 *
 * This function is used as a callback for the SW1 button press event during
 * collection. It allows the user to pause data collection during a session
 * and results in the LED being permanently lit over the duration of this pause.
 *****************************************************************************/
void sysPause(void)
{
	registerSW1Callback(sysResume);				// Set the call back to resume state
	// SW2 call back still for end session
	mpuSleepEn(True);							// Put the MPU to sleep
	clearMPUCallback();

	LED_GREEN_ON();								// Light green LED to indicate paused
	state = STATE_IDLE;							// Set the system state to idle
}

/**************************************************************************//**
 * \brief Resume collection during a pause from data collection
 *
 * This function is used as a callback for the SW1 button press event during a
 * pause. It allows the user to resume taking data from a pause and clears the
 * LED state in between
 *****************************************************************************/
void sysResume(void)
{
	registerSW1Callback(sysPause);				// Set SW1 callback to pause
	mpuSleepEn(False);
	registerMPUCallback(sysSample);

	LED_GREEN_OFF();							// Turn off the green LED
	state = STATE_COLLECTING;					// Set the system state to collecting
}

/**************************************************************************//**
 * \brief Stop data collection
 *
 * This function is used as a callback for the SW2 button press event during
 * data colelction. It allows the user to end a data session by pressing SW2
 * during operation. This blinks the RED LED 5 times and closes the session.
 *****************************************************************************/
void sysClose(void)
{
	blink(5, 2);								// Blink red LED 5 times to indicate user closed data session
	closeCause = sess_closed_user;				// Set the close cause to user
	evtQuePut(SYS_END_SESSION);					// Push end-session onto the event queue
}

/**************************************************************************//**
 * \brief (Re-)initialize the event queue
 *
 * Reset/flush the event queue.
 *****************************************************************************/
void evtQueInit(void)
{
	evtQuePutIdx = 0;
	evtQueGetIdx = 0;
	evtQueCount = 0;
	evtQueEmptyCount = 0;
}

/**************************************************************************//**
 * \brief Enqueue a #sysEvent in the event queue for execution
 *
 * \sideeffect	If the event queue is already full, the system will record
 * 				a critical error and turn off, and upon the next boot, the
 * 				node will be trapped until serviced by someone familiar with
 * 				the node (i.e., it will appear dead to the user).
 *****************************************************************************/
void evtQuePut(sysEvent passEvent)
{
	if(evtQueCount >= EVT_QUE_SIZE) {
		infoSetEvtQueueOvf();				// Event queue overflow
		if(sessionInProgress)				// Check for session in progress
			fsEndSession(rtcGetTime(), sess_closed_user, True);	// Close the session in progress if necessary
		systemReset();						// Reset the system
	}
	else{
		evtQue[evtQuePutIdx] = passEvent;					// Add event
		evtQuePutIdx = (evtQuePutIdx + 1) % EVT_QUE_SIZE;	// Increment and wrap index
		evtQueCount++;										// Increment event queue size
	}
}

/**************************************************************************//**
 * \brief Pop the next pending #SYS_EVENT off the event queue
 *
 * Record a new timestamp pair into the timing log of the flash card, using
 * the current timing epoch in effect.
 *
 * \return		The next #SYS_EVENT on the queue
 * \retval		#SYS_QUE_EMPTY event if queue empty
 *****************************************************************************/
sysEvent evtQueGet(void)
{
	sysEvent temp;

	if(evtQueCount <= 0){
			return SYS_QUE_EMPTY;
	}
	else {
		temp = evtQue[evtQueGetIdx];
		evtQueGetIdx = (evtQueGetIdx + 1) % EVT_QUE_SIZE;	// Increment and wrap index
		evtQueCount--;
	}
	return temp;
}

/**************************************************************************//**
 * \brief Execute the next event waiting on the event queue
 *
 * If the queue is empty,
 *****************************************************************************/
void evtQueExecute(void)
{
	switch(evtQueGet())
	{
		case SYS_QUE_EMPTY:
			evtQueEmptyCount++;
			break;
		case SYS_START_SESSION:
			sysStartSession();
			break;
		case SYS_END_SESSION:
			sysEndSession();
			break;
		case SYS_ENTER_SLEEP:
			sysSleep();
			break;
		case SYS_BUFF_INIT:
			mpuClearBuff();
			break;
		case SYS_RUN_CMD:
			runCommand(cmd);
			break;
		default:
			systemReset();
			break;
	}
	return;
}

/**************************************************************************//**
 * \brief Update the system state
 *****************************************************************************/
void updateState(void)
{
	extern unsigned char processingCmd;			// Processing command flag from command.c
	extern cardInfo cardData;					// Card data field from filesystem.c

	ftdiPacket inPacket;			// In packet read from FTDI (USCI A0 UART Mode)

	unsigned int i = 0;

	switch(state){
		case STATE_COLLECTING:		// COLLECTION MODE: Check for entering charge, card full, or data overflow
			if(CHARGING){
				closeCause = sess_closed_chg;
				evtQuePut(SYS_END_SESSION);
			}
			else if(cardFull){
				evtQueInit();
				closeCause = sess_closed_full;
				evtQuePut(SYS_END_SESSION);
			}
			else if(sensBuffFull){
				closeCause = sess_closed_ovfl;
				evtQuePut(SYS_END_SESSION);
			}
			break;
		case STATE_CMD:				// COMMAND MODE: Check for leaving charger or receiving info
			if(!CHARGING){			// Check for left charger
				evtQueInit();		// Clear the event queue
				ftdiFlush();		// Flush the FTDI comm buffer
				if(dataCollectionEn){
					evtQuePut(SYS_START_SESSION);
				}
				else{
					evtQuePut(SYS_ENTER_SLEEP);
				}
			}
			// Command processing
			else if(!processingCmd && ftdiGetBuffSize() >= CMD_LEN){
				for(i = 0; i < FTDI_OPEN_RETRIES; i++){	// Wait for module to become available
					if(ftdiGetStatus() == OPEN) break;
				}
				inPacket = ftdiRead(CMD_LEN);			// Read in a command from the buffer
				for(i = 0; i < CMD_LEN; i++){
					if(inPacket.data[i] == CMD_START_OF_SEQ) break;
				}
				if(i != CMD_LEN){
					cmd = (cmdPkt *)(&inPacket.data[i]);	// Cast in packet
					cmd->command = (cmd->command)>>8;		// Shift over command field
					LED_GREEN_ON();							// Turn LED on
					runCommand(cmd);						// Process command and send response
					LED_GREEN_OFF();						// Turn LED off
				}
			}
			break;
		case STATE_IDLE:
			if(CHARGING){
				evtQuePut(SYS_ENTER_CMD);
			}
			break;
		case STATE_SLEEP:
			break;
		default:
			break;

	}
}

