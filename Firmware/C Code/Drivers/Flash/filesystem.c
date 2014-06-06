/**************************************************************************//**
 * \file 	filesystem.c
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

#include "filesystem.h"
#include "infoflash.h"
#include "flash.h"
#include "rtc.h"
#include <string.h>

cardInfo cardData;										///< Node information data structure
sessInfo sessData;										///< Session information data structure

unsigned char dBuff[CARD_BUFFER_SIZE];					///< Card data buffer
unsigned int putIndex, getIndex = 0;					///< Data indexes
unsigned int bytesToWrite = 0;							///< Available bytes in card data buffer

unsigned long currSessSector = 0;						///< Holds the index of the current sessions info sector
unsigned long currSector = 0;							///< Tracks the current flash card sector

/**************************************************************************//**
 * \brief Update the card info in flash
 *
 * This function writes the current #cardInfo structure to the card info
 * sector in the flash
 *
 * \returns		fsRetCode	A file system return code (see #fsRetCode)
 *****************************************************************************/
fsRetCode updateCardInfo(void)
{
	struct Sector sect;

	sect.type = sect_cardInfo;							// Set the sector type
	memcpy((void *)sect.data, (void *)&cardData, sizeof(cardInfo));		// Copy in the card information
	if(secureFlashWrite(CARD_INFO_INDEX, &sect) != FLASH_SUCCESS) return FS_FAIL_WF; // Write sector
	else return FS_SUCCESS;
}

/**************************************************************************//**
 * \brief Update the session info in flash
 *
 * This function writes the current #sessInfo structure to the designated
 * sector in flash
 *
 * \param		sectorNum	The index of the desired write sector
 *
 * \returns		fsRetCode	A file system return code (see #fsRetCode)
 *****************************************************************************/
fsRetCode writeSessInfo(unsigned long sectorNum)
{
	struct Sector sect;

	sect.type = sect_sessInfo;							// Set the sector type
	memcpy((void *)sect.data, (void *)&sessData, sizeof(sessInfo));	// Copy in the session information
	if(secureFlashWrite(sectorNum, &sect) != FLASH_SUCCESS) return FS_FAIL_WF;	// Write sector
	else return FS_SUCCESS;
}

/**************************************************************************//**
 * \brief Clear the card status field
 *
 * This function resets the card status field to whatever should be the default
 * value/set of values.
 *
 * \returns		fsRetCode	A file system return code (see #fsRetCode)
 *****************************************************************************/
inline void cardStatusClear(void)
{
	cardData.cStatus.cardFull = 0;
	cardData.cStatus.cardResume = 0;
	cardData.cStatus.dataCollectionEn = 0;
	cardData.cStatus.readOnly = 0;
	cardData.cStatus.sessInProgress = 0;
}

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
 *
 * \returns		fsRetCode	A file system return code (see #fsRetCode)
 *
 * \sideeffect	Previous data may not be recoverable after running #fsInit
 *****************************************************************************/
fsRetCode fsInit(unsigned int nodeID, unsigned int epoch, unsigned long startSector, unsigned char *nodeNotes)
{
	unsigned char cardID[CARD_ID_LEN];
	bytesToWrite = 0;									// Clear "to write" count

	// Info flash update
	if(infoInit() == INFO_VALID){						// Initialize the info flash (check for failures)
		currSector = startSector;						// Reset the start sector
		currSessSector = startSector;					// Reset the session sector
		infoCardInit(cardID, nodeID, startSector, startSector, epoch); // Update info flash
	}
	else return FS_FAIL_INFO;							// Info flash failure

	cardStatusClear();									// Clear the card status

	// Clear data buffer management
	bytesToWrite = 0;
	putIndex = 0;
	getIndex = 0;

	switch(flashInit())									// Initialize the flash
	{
		case FLASH_NO_CARD:	// No card found set flag and return
			return FS_FAIL_CF;
		case FLASH_TIMEOUT:	// MMC communication timeout set readOnly and return
			cardData.cStatus.readOnly = 1;
			return FS_FAIL_RO;
		case FLASH_SUCCESS:	// MMC init successful
			cardData.cStatus.readOnly = 0;
			break;
		default:
			break;
	}
	flashReadCardID(cardID);							// Read the flash card ID

	// Create card info structure
	cardData.nodeID = nodeID;							// Copy node id
	cardData.epoch = epoch;								// Copy time epoch
	cardData.lastData = startSector;					// Set data start sector
	cardData.lastSess = startSector;					// Set session start sector
	cardData.startSector = startSector;					// Set start sector
	cardData.cStatus.cardResume = 1;					// Put the card in resumed mode
	memcpy((void *)&cardData.initTime, (void *)rtcGetTime(), sizeof(time)); // Get the current time of init
	memcpy(cardData.notes, nodeNotes, CARD_NOTES_SIZE);		// Copy the card notes into the header

	if(updateCardInfo() != FS_SUCCESS) return FS_FAIL_WF;	// Update the card info sector

	cardData.cStatus.cardResume = 1;					// Set the resume bit
	return FS_SUCCESS;
}

/**************************************************************************//**
 * \brief Resume the file system
 *
 * This function resume the file system after a power-off or flash-card removal
 * event. This function should be used instead of #fsInit during typical device
 * boot-up procedure.
 *
 * \returns		fsRetCode	A file system return code (see #fsRetCode)
 *
 * \sideeffect	Running this function stores all node metadata in the #cardInfo
 * 				and #sessInfo RAM structures
 *****************************************************************************/
fsRetCode fsResume(void)
{
	unsigned char cardID[CARD_ID_LEN], infoID[CARD_ID_LEN];
	struct Sector sect;
	bytesToWrite = 0;									// Clear the to-write count

	switch(flashInit())									// Initialize the flash
	{
		case FLASH_NO_CARD:								// No card found set flag and return
			return FS_FAIL_CF;
		case FLASH_TIMEOUT:								// MMC communication timeout set readOnly and return
			cardData.cStatus.readOnly = 1;						// Set the read only bit
			return FS_FAIL_RO;
		case FLASH_SUCCESS:								// MMC init successful
			cardData.cStatus.readOnly = 0;						// Clear read only condition
			break;
		default:
			break;
	}

	flashReadCardID(cardID);							// Read the flash card ID

	if(infoInit() == INFO_VALID){						// Check the system info flash for validity
		infoGetCardID(infoID);							// Get the card ID from the info flash
		if(memcmp(cardID, infoID, CARD_ID_LEN) != 0){	// Compare the two IDs for card verification
			return FS_FAIL_ID;							// Return ID mismatch failure
		}
	}
	else return FS_FAIL_INFO;

	// Read node info sector and parse data
	if(secureFlashRead(CARD_INFO_INDEX, &sect) != FLASH_SUCCESS){
		return FS_FAIL_RF;								// Read failure: cannot read card info sector
	}
	memcpy((void *)&cardData, (void *)&sect, sizeof(cardInfo));		// Copy over the card info

	if(cardData.cStatus.cardFull){						// Check for card full status bit
		return FS_FAIL_CF;
	}

	// Clear data buffer management
	bytesToWrite = 0;
	putIndex = 0;
	getIndex = 0;

	currSector = cardData.lastData;
	currSessSector = cardData.lastSess;

	cardData.cStatus.cardResume = 1;					// Set card status to resumed
	return FS_SUCCESS;									// Return success
}

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
fsRetCode fsStartSession(unsigned int SR, axisCtrl axis, time* startTime)
{
	// Condition code checks
	if(cardData.cStatus.readOnly == 1) return FS_FAIL_RO;					// Check for read only condition
	else if (cardData.cStatus.sessInProgress == 1) return FS_FAIL_SIP;		// Check for another session in progress
	else if (cardData.cStatus.cardResume == 1) return FS_FAIL_NR;			// Check for resume condition
	else if (cardData.cStatus.dataCollectionEn == 0) return FS_FAIL_DCE;	// Check for data collection enabled
	else if(currSector >= TOTAL_SECTORS || cardData.cStatus.cardFull) {		// Check for card full condition
		cardData.cStatus.cardFull = 1;
		return FS_FAIL_CF;													// Card full return code
	}

	// Previous session info update (write 1)
	sessData.nextSessSector = ++currSector;									// Set the last sessions 'next' value to this session's
	if(writeSessInfo(currSessSector) != FS_SUCCESS) return FS_FAIL_WF;		// Update the old session

	// Current session info update (write 2)
	sessData.serial = sessData.serial + 1;									// Increment the session serial number
	sessData.lastSessSector = currSessSector;								// Set the last start sector
	sessData.nextSessSector = (unsigned int)(-1);							// Set the next start sector to -1
	sessData.length = 0;													// Set the starting length to 0
	sessData.samplingRate = SR;												// Set the sampling rate to the provided rate
	sessData.axis = axis;													// Set the axis bit field to provided value
	sessData.status = sess_open;											// Set the session status to open
	if(startTime->mon == 0){												// Check for null start time (month cannot be 0)
		memcpy((void *)&(sessData.startTime), (void *)rtcGetTime(), sizeof(time));	// If needed get a valid start time from the RTC
	}
	else {
		memcpy((void *)&(sessData.startTime), (void *)&startTime, sizeof(time));	// Copy over start time object
	}
	if(writeSessInfo(currSector) != FS_SUCCESS) return FS_FAIL_WF;			// Set sector information and write sector to card

	// Clear data buffer management
	bytesToWrite = 0;
	putIndex = 0;
	getIndex = 0;

	// Increment/refresh sector management
	currSessSector = currSector;											// Update to the most recent session sector
	++currSector;															// Increment the current sector (for first data write)
	cardData.cStatus.sessInProgress = 1;

	return FS_SUCCESS;
}

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
fsRetCode fsEndSession(time* endTime, sessStatus closeCause, unsigned char cardUpdate)
{
	struct Sector sect;

	// Check card status to assure a session is in progress
	if(cardData.cStatus.sessInProgress == 0) return FS_FAIL_SIP;			// No session in progress to end

	// Buffer emptying
	if(bytesToWrite > 0) {													// Empty data buffer if data is present
		sect.type = sect_data;
		memcpy((void *)sect.data, (void *)(&dBuff[getIndex]), bytesToWrite);
		if(secureFlashWrite(currSector, &sect) == -1) return FS_FAIL_WF;		// Attempt flash write
		else {												// If flash write succeeds clear the "to-write" count
			bytesToWrite = 0;
		}
	}

	// Duration and time stamping
	sessData.length = currSector - currSessSector;			// Determine length from current sector
	if(endTime->mon == 0){									// Check for valid time stamp (month cannot be 0)
		endTime = rtcGetTime();								// If needed get time from RTC
	}
	sessData.status = closeCause;							// Copy over the close cause (session status field)
	memcpy((void *)(&(sessData.endTime)), (void *)endTime, sizeof(time));	// Copy over the session end time
	if(writeSessInfo(++currSector) != FS_SUCCESS) return FS_FAIL_WF;

	// Update the card info
	cardData.lastData = currSector;
	cardData.lastSess = currSessSector;
	cardData.cStatus.sessInProgress = 0;
	if(cardUpdate != 0){
		if(updateCardInfo() != FS_SUCCESS) return FS_FAIL_WF;
	}
	return FS_SUCCESS;
}

/**************************************************************************//**
 * \brief Write data to the current open data session
 *
 * This function write the provided data to the current card data session.

 * \param		*data		Pointer to the data to be written
 * \param		len			Length of the data to be written
 * \retval		fsRetCode	A file system return code (see #fsRetCode)
 *****************************************************************************/
fsRetCode fsWriteData(unsigned char *data, unsigned int len)
{
	unsigned int sectCount = 0;
	unsigned int i;
	fsRetCode retval = FS_SUCCESS;
	struct Sector sect;

	if(cardData.cStatus.readOnly == 1) return FS_FAIL_RO;				// Read only mode (no data write)
	else if(cardData.cStatus.sessInProgress == 0) return FS_FAIL_SIP;	// No session in progress (no data write)
	else if(currSector > TOTAL_SECTORS || cardData.cStatus.cardFull){	// End of card (no data write)
		cardData.cStatus.cardFull = 1;									// Set card full status bit
		return FS_FAIL_CF;												// Card full return code
	}

	if(len > (CARD_BUFFER_SIZE - bytesToWrite)){				// Check if requested size can be written to buffer
		len = CARD_BUFFER_SIZE - bytesToWrite;					// Choose maximum amount of data that will fit into the buffer
		retval = FS_FAIL_BF;									// Set buffer full return code
	}

	for(i = 0; i < len; i++){
		dBuff[putIndex++] = *(data++);							// Copy over a byte of data
		if(putIndex >= CARD_BUFFER_SIZE) putIndex = 0;			// Check for wrap case
		if(putIndex == getIndex) break;							// Check for reach get index (buffer full)
	}
	bytesToWrite += len;										// Increment the bytes to write value
	sectCount = bytesToWrite/SECTOR_DATA_SIZE;					// Update the sector count

	if(sectCount > 0){											// Check if we have enough bytes to write a sector
		sect.type = sect_data;									// Set sector type to "data"
		memcpy((void *)sect.data, (void *)(&dBuff[getIndex]), SECTOR_DATA_SIZE);	// Copy over bytes to be written
		for(i = 0; i < sectCount; i++){							// Write each buffered sector of data to the card
			if(secureFlashWrite(currSector++, &sect) == -1){	// Check for write failures
				retval = FS_FAIL_WF;							// If secure flash write fails return write failure
				break;											// Break the loop
			}
			else{
				getIndex += SECTOR_DATA_SIZE;					// Increment the get index
				getIndex %= CARD_BUFFER_SIZE;					// Wrap the get index if necessary
				bytesToWrite -= SECTOR_DATA_SIZE;				// Decrement the data buffer size
			}
		}
		sectCount = 0;											// Clear the sector count
	}
	return retval;
}

/**************************************************************************//**
 * \brief Halt the file system and save the state
 *
 * This function allows the user to quickly close any open sessions and save
 * all metadata to the card. Useful for shutdown and SVS operations.

 * \retval		fsRetCode	A file system return code (see #fsRetCode)
 *****************************************************************************/
fsRetCode fsHalt(void)
{
	time t = { 0 };

	if(cardData.cStatus.readOnly == 1) return FS_FAIL_RO;			// Check for read only flag
	else if(cardData.cStatus.cardResume == 0) return FS_FAIL_NR;	// Check for resume state

	infoUpdateLastSector(currSector, currSessSector, cardData.epoch);	// Update the info B segment

	if(cardData.cStatus.sessInProgress == 1){
		return fsEndSession(&t, sess_closed_halt , 1);		// Close session and update card info
	}
	else {
		return updateCardInfo();							// Update card info
	}
}
