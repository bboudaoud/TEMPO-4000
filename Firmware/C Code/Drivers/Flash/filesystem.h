/**************************************************************************//**
 * \file 	filesystem.h
 * \author	Ben Boudaoud (bb3jd@virginia.edu)
 * \date	Sep 11, 2013
 *
 * \brief	This file contains the TEMPO 4000 file system management code.
 *
 * This library provides a simple interface for all flash operations along with a
 * minimalist, linked-list style implementation of a basic file system. This
 * file system is NOT COMPATIBLE with any previous TEMPO platform, including the
 * TEMPO 3.2F custom file system authored by Jeff Brantly.
 *****************************************************************************/
#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_

#include "rtc.h"
#include "system.h"

// Card Data Locations (Indexes)
#define	CARD_INFO_INDEX			0					///< Location of node info sector for now
#define SESS_START_SECTOR		100					///< Session/data info start sector
#define SESS_NOTES_SIZE			140					///< Session information notes field size
#define	CARD_NOTES_SIZE			140					///< Card information notes field size

// Status and return codes
typedef enum fsReturnCode							/// Enumerated type for file system return codes
{
	FS_SUCCESS = 0,									///< Operation successful
	FS_FAIL_RO = 1,									///< Operation failed: read only
	FS_FAIL_SIP = 2,								///< Operation failed: session in progress
	FS_FAIL_CF = 3,									///< Operation failed: card full
	FS_FAIL_BF = 4,									///< Operation failed: buffer full
	FS_FAIL_WF = 5,									///< Operation failed: write fail
	FS_FAIL_RF = 6,									///< Operation failed: read fail
	FS_FAIL_INIT = 7,								///< Operation failed: need initialization
	FS_FAIL_NR = 8,									///< Operation failed: need resume
	FS_FAIL_INFO = 9,								///< Operation failed: info flash compromised
	FS_FAIL_ID = 10,								///< Operation failed: info card id and actual id mismatch
	FS_FAIL_DCE = 11								///< Operation failed: need to set data collection enabled
} fsRetCode;

typedef struct cardStatusCode						/// Bit field structure for card status
{
	volatile unsigned sessInProgress : 1;			///< Session in progress status bit
	volatile unsigned cardResume : 1;				///< Card present status bit
	volatile unsigned cardFull : 1;					///< Card full status bit
	volatile unsigned readOnly : 1;					///< Read only status bit
	volatile unsigned dataCollectionEn : 1;			///< Data collection enabled status bit
} cardStatus;

// Information structures
typedef struct calibInformation						/// Calibration information structure
{
	unsigned int x_acc_offset;						///< X Accelerometer DC offset
	unsigned int y_acc_offset;						///< Y Accelerometer DC offset
	unsigned int z_acc_offset;						///< Z Accelerometer DC offset
	unsigned int x_acc_sens;						///< X Accelerometer AC sensitivity
	unsigned int y_acc_sens;						///< Y Accelerometer DC sensitivity
	unsigned int z_acc_sens;						///< Z Accelerometer DC sensitivity
	unsigned int x_gyro_offset;						///< X Gyro DC offset
	unsigned int y_gyro_offset;						///< Y Gyro DC offset
	unsigned int z_gyro_offset;						///< Z Gyro DC offset
	unsigned int x_gyro_sens;						///< X Gyro AC sensitivity
	unsigned int y_gyro_sens;						///< Y Gyro AC sensitivity
	unsigned int z_gyro_sens;						///< Z Gyro AC sensitivity
} calibInfo;

/// \warning Do not increase this structure beyond #SECTOR_SIZE bytes!
typedef struct cardInformation						/// Card information structure
{
	unsigned int nodeID;							///< Node identification number
	cardStatus	cStatus;							///< Card status indicator field
	unsigned int epoch;								///< Time epoch number
	unsigned long lastData;							///< Last session data sector index
	unsigned long lastSess;							///< Last session info sector index
	unsigned long startSector;						///< Start sector index
	time initTime;									///< Card initialization time
	unsigned char notes[CARD_NOTES_SIZE];			///< Card information string
} cardInfo;

/// \warning Do not increase this structure beyond #SECTOR_SIZE bytes!
typedef struct sessionInformation					/// Session information
{
	unsigned int serial;							///< Session serial number
	unsigned int nodeID;							///< Node identification number
	unsigned int epoch;								///< Session epoch number
	unsigned long lastSessSector;					///< Sector index of start of last session
	unsigned long nextSessSector;					///< Sector index of start of next session
	unsigned long length;							///< Length of the session in sectors
	unsigned int samplingRate;						///< Sampling rate of session
	time startTime;									///< RTC time stamp for start of session
	time endTime;									///< RTC time stamp for end of session
	axisCtrl axis;									///< Axis control field
	sessStatus status;								///< Session status
	unsigned char notes[SESS_NOTES_SIZE];			///< Notes field
} sessInfo;

// Function Prototypes
/**************************************************************************//**
 * \brief Update the card info in flash
 *
 * This function writes the current #cardInfo structure to the card info
 * sector in the flash
 *
 * \returns		fsRetCode	A file system return code (see #fsRetCode)
 *****************************************************************************/
fsRetCode updateCardInfo(void);

/**************************************************************************//**
 * \brief Update the session info in flash
 *
 * This function writes the current #sessInfo structure to the designated
 * sector in flash
 *
 * \param		sectorNum	The index of the desired write sector
 * \returns		fsRetCode	A file system return code (see #fsRetCode)
 *****************************************************************************/
fsRetCode writeSessInfo(unsigned long sectorNum);

/**************************************************************************//**
 * \brief Clear the card status field
 *
 * This function resets the card status field to whatever should be the default
 * value/set of values.
 *
 * \returns		fsRetCode	A file system return code (see #fsRetCode)
 *****************************************************************************/
inline void cardStatusClear(void);

/**************************************************************************//**
 * \brief Initialize the file system
 *
 * This function reinitializes (read re-formats) the file system based upon
 * the set of inputs provided by the parameter list. This function SHOULD NOT
 * be used to initialize flash on device load (see #fsResume)
 *
 * \param		nodeID	The desired node identification number
 * \param		epoch	The desired time epoch number
 * \param		startSector	The sector at which to start recording data
 * \param 		*nodeNotes	A pointer to a string which contains any futher
 * 				node information
 * \returns		fsRetCode	A file system return code (see #fsRetCode)
 * \sideeffect	Previous data may not be recoverable after running #fsInit
 *****************************************************************************/
fsRetCode fsInit(unsigned int nodeID, unsigned int epoch, unsigned long startSector, unsigned char *nodeNotes);

/**************************************************************************//**
 * \brief Resume the file system
 *
 * This function resume the file system after a power-off or flash-card removal
 * event. This function should be used instead of #fsInit during typical device
 * boot-up procedure.
 *
 * \returns		fsRetCode	A file system return code (see #fsRetCode)
 * \sideeffect	Running this function stores all node metadata in the #cardInfo
 * 				and #sessInfo RAM structures
 *****************************************************************************/
fsRetCode fsResume(void);

/**************************************************************************//**
 * \brief Start a new session
 *
 * This function begins a new data session in the file system. This function
 * should only be called when no other session is in progress, and after the
 * file system has been successfully resumed or initialized.
 *
 * \param		SR			The sampling rate of the data stream
 * \param		axis		The axis bit-field w/ the sampled axes set to '1'
 * \param		startTime	The start time of this data session (to get from
 * 							RTC set Mon field to 0)
 *
 * \retval		fsRetCode	A file system return code (see #fsRetCode)
 *
 * \sideeffect	Sets the session in progress (SIP) flag to prevent multiple
 * 				sessions being open at once.
 *****************************************************************************/
fsRetCode fsStartSession(unsigned int SR, axisCtrl axis, time* startTime);

/**************************************************************************//**
 * \brief End a running session
 *
 * This function ends a session currently being stored to on the file system.

 * \param		endTime		The end time of this data session (to get from RTC
 * 							set Mon field to 0)
 * \param		closeCause	The reason for the session ending (see #sessStatus)
 * \param		cardUpdate	Boolean flag indicating whether or not to update
 * 							the card header
 * \retval		fsRetCode	A file system return code (see #fsRetCode)
 * \sideeffect	Clears the session in progress (SIP) when done
 *****************************************************************************/
fsRetCode fsEndSession(time* endTime, sessStatus closeCause, unsigned char cardUpdate);

/**************************************************************************//**
 * \brief Write data to the current open data session
 *
 * This function write the provided data to the current card data session.

 * \param		*data		Pointer to the data to be written
 * \param		len			Length of the data to be written
 * \retval		fsRetCode	A file system return code (see #fsRetCode)
 *****************************************************************************/
fsRetCode fsWriteData(unsigned char *data, unsigned int len);

/**************************************************************************//**
 * \brief Halt the file system and save the state
 *
 * This function allows the user to quickly close any open sessions and save
 * all metadata to the card. Useful for shutdown and SVS operations.

 * \retval		fsRetCode	A file system return code (see #fsRetCode)
 *****************************************************************************/
fsRetCode fsHalt(void);

#endif /* FILESYSTEM_H_ */
