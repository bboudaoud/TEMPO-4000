/**************************************************************************//**
 * \file 	infoflash.h
 * \author	Ben Boudaoud (bb3jd@virginia.edu)
 * \date	Dec 20, 2013
 *
 * \brief	This file contains the TEMPO 4000 info flash management code
 *
 * This library provides a simple interface for reading and writing the MSP430
 * info flash on 5xxx series devices. The A and B segments are used for storage
 * of critical non-volatile data including system state and critical flags.
 *****************************************************************************/
#ifndef INFOFLASH_H_
#define INFOFLASH_H_
#include "flash.h"

/// \warning Do not increase this structure beyond 128 bytes!
static volatile struct {				/// TEMPO 4000 info A structure
	unsigned int lowVoltage;			///< Critical flag: low voltage
	unsigned int highTemp;				///< Critical flag: high temperature
	unsigned int evtQueueOvf;			///< Critical flag: event queue overflow
	unsigned int infoBFail;				///< Critical flag: info B fail
	unsigned int validityCode;			///< Sector valid code (always read as 0xAAAA)
} _tempoInfoA;

/// \warning Do not increase this structure beyond 128 bytes!
static struct {							/// TEMPO 4000 info B structure
	unsigned int nodeID;				///< TEMPO 4000 Node ID
	unsigned char cid[CARD_ID_LEN];		///< MMC card ID
	unsigned long lastSector;			///< Last sector written in file system
	unsigned long lastSessSector;		///< Last session info sector written in file system
	unsigned int currEpoch;				///< Current card epoch number
	unsigned int lastTime;				///< Last valid RTC time stamp
	unsigned int validityCode;			///< Valid code (always read 0xAAAA)
	unsigned int checksum;				///< Sector checksum
} _tempoInfoB;

// Addresses of info flash segments
#define		INFO_A_ADDR		(0x1980)	///< Address of info flash segment A
#define		INFO_B_ADDR		(0x1900)	///< Address of info flash segment B
#define		INFO_C_ADDR		(0x1880)	///< Address of info flash segment C
#define		INFO_D_ADDR		(0x1800) 	///< Address of info flash segment D

#define	INFO_VALID_CODE		0xAAAA		///< Critical flag validity code

#define INFO_VALID		0				///< Info flash segments all valid return code
#define INFO_A_INVALID	-1				///< Info flash segment A invalid return code
#define INFO_B_INVALID	-2				///< Info flash segment B invalid return code
#define INFO_CRITICAL	-3				///< Info flash segment A contains critical code

// Prototypes
/**************************************************************************//**
 * \brief Initialize RAM copies of Info flash values
 *
 * Copy values from info flash into RAM-cached copies and run a validity
 * check on the protected information (CID, calib, serials, ...)
 *
 * \retval	0 success
 * \retval	-1 validity check failure
 *
 * \sideeffect	Sets #infoRO if validity check fails
 *****************************************************************************/
int infoInit(void);

/**************************************************************************//**
 * \brief Write RAM copy of critical flags back to info flash
 *****************************************************************************/
static void _writeTempoInfoA();

/**************************************************************************//**
 * \brief Write RAM copy of info B structure back to info flash
 *****************************************************************************/
static int _writeTempoInfoB();

/**************************************************************************//**
 * \brief Check whether any critical condition flags are set in info flash
 *
 * \retval	0 no critical flags
 * \retval	1 critical flag found
 *****************************************************************************/
int infoCheckCritical(void);

/**************************************************************************//**
 * \brief Write the low voltage critical flag to info flash
 *****************************************************************************/
void infoSetLowVoltage(void);

/**************************************************************************//**
 * \brief Write the high temperature critical flag to info flash
 *****************************************************************************/
void infoSetHighTemp(void);

/**************************************************************************//**
 * \brief Write the event queue overflow critical flag to info flash
 *****************************************************************************/
void infoSetEvtQueueOvf(void);

/**************************************************************************//**
 * \brief Write the info flash SegmentB failure critical flag to info flash
 *****************************************************************************/
void infoSetInfoBFail(void);

/**************************************************************************//**
 * \brief Clear all critical flags from the info flash
 *****************************************************************************/
void infoClearCriticalFlags(void);

/**************************************************************************//**
 * \brief Update last session serial and time epoch to info flash
 *
 * \retval	0 success
 * \retval	-1 #infoRO and/or verification check failed
 *
 * \sideeffect	Sets #infoRO if validity check fails
 *****************************************************************************/
int infoUpdateLastSector(unsigned int lastSector, unsigned int lastSessSector, unsigned int epoch);

/**************************************************************************//**
 * \brief Retrieve last sector index from infoflash
 *****************************************************************************/
unsigned int infoGetLastSector(void);

/**************************************************************************//**
 * \brief Retrieve last session info sector index from infoflash
 *****************************************************************************/
unsigned int infoGetLastSessSector(void);

/**************************************************************************//**
 * \brief Retrieve last session serial from infoflash
 *****************************************************************************/
unsigned int infoGetLastEpoch(void);

/**************************************************************************//**
 * \brief Retrieve card ID from infoflash
 *
 * \param[out]	cid		Card ID string (of length #CARD_ID_LEN)
 *****************************************************************************/
void infoGetCardID(unsigned char * cid);

/**************************************************************************//**
 * \brief Retrieve node ID from infoflash
 *****************************************************************************/
unsigned int infoGetNodeID(void);

/**************************************************************************//**
 * \brief Set card ID and re-init serials
 *
 * \note	Intended for use when doing a card re-init or using new card
 *
 * \retval	0 success
 * \retval	-1 Verification check failed
 *
 * \sideeffect	The calibration values will be cleared
 * \sideeffect	Sets #infoRO if validity check fails
 *****************************************************************************/
int infoCardInit(unsigned char* cid, unsigned int nodeID, unsigned int lastSector, unsigned int lastSessSector, unsigned int timeEpoch);

#endif /* INFOFLASH_H_ */
