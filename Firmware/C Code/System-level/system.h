/*
 * system.h
 *
 *  Created on: Jan 6, 2014
 *      Author: bb3jd
 */

#ifndef SYSTEM_H_
#define SYSTEM_H_

#define EVT_QUE_SIZE	32				///< Size of the event queue (in events)
#define BLINK_COUNT		50				///< Number of samples to take between blinking the LED

// Firmware versioning
typedef struct firmwareVersion {		/// Firmware version typedef
	unsigned char majorRev;				///< Major revision number (always '4' for TEMPO 4000)
	unsigned char minorRev;				///< Minor revision number (should only change for core structural/functional code alterations)
	unsigned char majorUpdate;			///< Major update number (should change for any major fixes/revisions in firmware operation)
	unsigned char minorUpdate;			///< Minor update number (should change for all other minor code/bug fixes/patches)
} Version;

// Axis control
typedef struct axisField							/// Type for axis collection control
{
	volatile unsigned x_acc : 1;					///< X Accelerometer enable bit
	volatile unsigned y_acc : 1;					///< Y Accelerometer enable bit
	volatile unsigned z_acc : 1;					///< Z Accelerometer enable bit
	volatile unsigned x_gyro : 1;					///< X Gyro enable bit
	volatile unsigned y_gyro : 1;					///< Y Gyro enable bit
	volatile unsigned z_gyro : 1;					///< Z Gyro enable bit
	volatile unsigned temp : 1;						///< Temperaure enable bit
} axisCtrl;

// Usefeul axisCtrl field macros
#define AXIS_XYZ_ACC	{1,1,1,0,0,0,0}				///< Accelerometer only axis ctrl field define
#define AXIS_XYZ_GYRO	{0,0,0,1,1,1,0}				///< Gyro only axis ctrl field define
#define AXIS_XYZ_6DOF	{1,1,1,1,1,1,0}				///< Accel + gyro (6 DoF) axis ctrl filed
#define AXIS_TEMP		{0,0,0,0,0,0,1}				///< Temperature only axis ctrl field define
#define AXIS_ALL		{1,1,1,1,1,1,1}				///< All sensors enabled

typedef enum systemState{				/// Enumerated type for system state
	STATE_NULL = 0,
	STATE_BOOT = 1,						///< System boot state
	STATE_CMD = 2,						///< System command (charging) state
	STATE_COLLECTING = 3,				///< System collection state
	STATE_IDLE = 4,						///< System idle state
	STATE_SLEEP = 5						///< System sleep state
} sysState;

typedef enum sessionStatusCode			/// Enumerated type for session close cause
{
	sess_open = 0x00,					///< Session was not closed
	sess_closed_chg = 0x01,				///< Session was closed on contact with charger
	sess_closed_lowv = 0x02,			///< Session was closed on low voltage condition
	sess_closed_full = 0x03,			///< Session was closed on card full condition
	sess_closed_ovfl = 0x04,			///< Session was closed on event queue overflow
	sess_closed_halt = 0x05,			///< Session was closed on file system halt
	sess_closed_user = 0x06				///< Session was closed by user push button
} sessStatus;

typedef struct samplingInfo{			/// Typedef for sampling control
	unsigned int SR;					///< Sampling rate
	axisCtrl axis;						///< Axis selection
} sampInfo;

typedef enum systemEvent{
	SYS_QUE_EMPTY = 0,
	SYS_START_SESSION = 1,
	SYS_END_SESSION = 2,
	SYS_BUFF_INIT = 3,
	SYS_ENTER_CMD = 4,
	SYS_ENTER_SLEEP = 5,
	SYS_RUN_CMD = 6
} sysEvent;


#define FTDI_OPEN_RETRIES		100		///< Number of times to check for the FTDI UART available

// Prototypes
/**************************************************************************//**
 * \brief Setup port direction, selection, and (sometimes) values
 *
 * Initialize ports for operation. Mostly relies on HAL-level definitions for
 * static setup of I/O for items like the LED, switches, charge indicator, MPU,
 * and MMC card.
 *****************************************************************************/
void ioConfig(void);

/**************************************************************************//**
 * \brief Initialize the system (after reset)
 *
 * "Boot" up the system, initializing flags, modules, peripherals, ports,
 * buffers, etc., etc.
 *
 * If you want to know everything it does, read the code!
 *****************************************************************************/
int sysInit(void);

/**************************************************************************//**
 * \brief Put the system to sleep
 *
 * "Sleep" the system, first clear any un-used output ports, the event queue, and
 * put the MPU to sleep.
 *
 * If you want to know everything it does, read the code!
 *****************************************************************************/
void sysSleep(void);

/**************************************************************************//**
 * \brief Wake the system from sleep to start taking data or enter command
 *
 * Wake up, blink the LED 3 times, then exit LPM3 and determine next task
 * to execute.
 *****************************************************************************/
void sysWake(void);

void sysPause(void);
void sysClose(void);
void sysResume(void);

/**************************************************************************//**
 * \brief Start a new data session
 *
 * Set up the MPU for taking a data session, interrupts on the MSP, and file
 * system for storing the incoming data then set the state to #STATE_COLLECTING
 *
 * If you want to know everything it does, read the code!
 *****************************************************************************/
void sysStartSession(void);

/**************************************************************************//**
 * \brief Sample the MPU on an interrupt
 *
 * This function is used as a callback for the MPU data ready event. It checks
 * for the desired axes to sample and performs sampling on these channels. It
 * also provides critical flag updates for buffer overrun and event queue overflow
 *
 * If you want to know everything it does, read the code!
 *****************************************************************************/
void sysSample(void);

/**************************************************************************//**
 * \brief End a data session
 *
 * This function ends the current data session being stored to the card and set
 * the system state to #STATE_END_SESSION
 *****************************************************************************/
void sysEndSession(void);

/**************************************************************************//**
 * \brief (Re-)initialize the event queue
 *
 * Reset/flush the event queue.
 *****************************************************************************/
void evtQueInit(void);

/**************************************************************************//**
 * \brief Enqueue a #sysEvent in the event queue for execution
 *
 * \sideeffect	If the event queue is already full, the system will record
 * 				a critical error and turn off, and upon the next boot, the
 * 				node will be trapped until serviced by someone familiar with
 * 				the node (i.e., it will appear dead to the user).
 *****************************************************************************/
void evtQuePut(sysEvent passEvent);

/**************************************************************************//**
 * \brief Pop the next pending #SYS_EVENT off the event queue
 *
 * Record a new timestamp pair into the timing log of the flash card, using
 * the current timing epoch in effect.
 *
 * \return		The next #SYS_EVENT on the queue
 * \retval		#SYS_QUE_EMPTY event if queue empty
 *****************************************************************************/
sysEvent evtQueGet(void);

/**************************************************************************//**
 * \brief Execute the next event waiting on the event queue
 *
 * If the queue is empty,
 *****************************************************************************/
void evtQueExecute(void);

/**************************************************************************//**
 * \brief Update the system state
 *****************************************************************************/
void updateState(void);


#endif /* SYSTEM_H_ */
