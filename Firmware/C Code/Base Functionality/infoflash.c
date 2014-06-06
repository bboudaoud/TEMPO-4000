/**************************************************************************//**
 * \file 	infoflash.c
 * \author	Ben Boudaoud (bb3jd@virginia.edu)
 * \date	Dec 20, 2013
 *
 * \brief	This file contains the TEMPO 4000 info flash management code
 *
 * This library provides a simple interface for reading and writing the MSP430
 * info flash on 5xxx series devices. The A and B segments are used for storage
 * of critical non-volatile data including system state and critical flags.
 *****************************************************************************/

#include <msp430.h>
#include <string.h>
#include "infoflash.h"
#include "hal.h"
#include "util.h"


unsigned char infoRO = 0;		///< Info flash read only flag

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
int infoInit(void)
{
	unsigned int checksum;

	memcpy((void *)&_tempoInfoA, (void *)INFO_A_ADDR, sizeof(_tempoInfoA));
	memcpy(&_tempoInfoB, (void *)INFO_B_ADDR, sizeof(_tempoInfoB));

	if(_tempoInfoA.validityCode != INFO_VALID_CODE){
		infoRO = 1;
		return INFO_A_INVALID;
	}

	// Test validity of the stored info
	checksum = fletcherChecksum((unsigned char *)&_tempoInfoB,
			sizeof(_tempoInfoB) - sizeof(_tempoInfoB.checksum), 0);

	if(_tempoInfoB.validityCode != INFO_VALID_CODE || checksum != _tempoInfoB.checksum) {
		infoRO = 1;
		return INFO_B_INVALID;
	}

	return INFO_VALID;
}

/**************************************************************************//**
 * \brief Write RAM copy of critical flags back to info flash
 *****************************************************************************/
static void _writeTempoInfoA()
{
	unsigned int status;

	_tempoInfoA.validityCode = INFO_VALID_CODE;

	/// \todo decide whether to put a watchdog reset here

	// MSP430 User guide stipulates that this entire process be protected
	enter_critical(status);
		if(FCTL3 | LOCKA) FCTL3 = (FWKEY + LOCKA);
		else FCTL3 = FWKEY;				// Clear Lock bit
		FCTL1 = (FWKEY + ERASE);			// Set Erase bit
		*(unsigned char *)INFO_A_ADDR = 0;	// Dummy write to erase flash segment

		FCTL1 = (FWKEY + WRT);			// Set WRT bit for write operation
		memcpy((void *)INFO_A_ADDR, (const void *)&_tempoInfoA, sizeof(_tempoInfoA));

		FCTL1 = FWKEY;					// Clear WRT bit
		FCTL3 = FWKEY + LOCK;			// Set LOCK bit
	exit_critical(status);
}

/**************************************************************************//**
 * \brief Write RAM copy of info B structure back to info flash
 *****************************************************************************/
static int _writeTempoInfoB()
{
	unsigned int status;

	_tempoInfoB.validityCode = INFO_VALID_CODE;
	_tempoInfoB.checksum = fletcherChecksum((unsigned char *)&_tempoInfoB,(sizeof(_tempoInfoB) - sizeof(_tempoInfoB.checksum)), 0);


	// MSP430 User guide stipulates that this entire process be protected
	enter_critical(status);
		FCTL3 = FWKEY;					// Clear Lock bit
		FCTL1 = FWKEY + ERASE;			// Set Erase bit
		*(unsigned char *)INFO_B_ADDR = 0;	// Dummy write to erase flash segment

		FCTL1 = (FWKEY + WRT);			// Set WRT bit for write operation
		memcpy((void *)INFO_B_ADDR, &_tempoInfoB, sizeof(_tempoInfoB));
		FCTL1 = FWKEY;					// Clear WRT bit
		FCTL3 = FWKEY + LOCK;			// Set LOCK bit
	exit_critical(status);

	if(memcmp(&_tempoInfoB, (void *)INFO_B_ADDR, sizeof(_tempoInfoB)) != 0) {
		infoRO = 1;
		infoSetInfoBFail();
		return -1;
	}

	return 0;
}

/**************************************************************************//**
 * \brief Check whether any critical condition flags are set in info flash
 *
 * \retval	0 	Info valid
 * \retval	-1	Info A fails validity check (see #INFO_A_INVALID)
 * \retval	-3	Info A contians critical flags (see #INFO_CRITICAL)
 *****************************************************************************/
int infoCheckCritical(void)
{
	// Check info flash for critical previous system error flags
	if(_tempoInfoA.validityCode == INFO_VALID_CODE) {
		if(_tempoInfoA.evtQueueOvf == 1 || _tempoInfoA.highTemp == 1
			|| _tempoInfoA.infoBFail == 1) {
				return INFO_CRITICAL;
		}
	}
	else return INFO_A_INVALID;

	return INFO_VALID;
}

/**************************************************************************//**
 * \brief Write the low voltage critical flag to info flash
 *****************************************************************************/
void infoSetLowVoltage(void)
{
	_tempoInfoA.lowVoltage = 1;
	_writeTempoInfoA();
}

/**************************************************************************//**
 * \brief Write the high temperature critical flag to info flash
 *****************************************************************************/
void infoSetHighTemp(void)
{
	_tempoInfoA.highTemp = 1;
	_writeTempoInfoA();
}

/**************************************************************************//**
 * \brief Write the event queue overflow critical flag to info flash
 *****************************************************************************/
void infoSetEvtQueueOvf(void)
{
	_tempoInfoA.evtQueueOvf = 1;
	_writeTempoInfoA();
}

/**************************************************************************//**
 * \brief Write the info flash SegmentB failure critical flag to info flash
 *****************************************************************************/
void infoSetInfoBFail(void)
{
	_tempoInfoA.infoBFail = 1;
	_writeTempoInfoA();
}

/**************************************************************************//**
 * \brief Clear all critical flags from the info flash
 *****************************************************************************/
void infoClearCriticalFlags(void)
{
	_tempoInfoA.lowVoltage = 0;
	_tempoInfoA.highTemp = 0;
	_tempoInfoA.evtQueueOvf = 0;
	_tempoInfoA.infoBFail = 0;
	_writeTempoInfoA();
}


/**************************************************************************//**
 * \brief Update last session serial and time epoch to info flash
 *
 * \retval	0 success
 * \retval	-1 #infoRO and/or verification check failed
 *
 * \sideeffect	Sets #infoRO if validity check fails
 *****************************************************************************/
int infoUpdateLastSector(unsigned int lastSector, unsigned int lastSessSector, unsigned int epoch)
{
	_tempoInfoB.lastSector = lastSector;
	_tempoInfoB.lastSessSector = lastSessSector;
	_tempoInfoB.currEpoch = epoch;
	return _writeTempoInfoB();
}

/**************************************************************************//**
 * \brief Retrieve last sector index from infoflash
 *****************************************************************************/
unsigned int infoGetLastSector(void)
{
	return _tempoInfoB.lastSector;
}

/**************************************************************************//**
 * \brief Retrieve last session info sector index from infoflash
 *****************************************************************************/
unsigned int infoGetLastSessSector(void)
{
	return _tempoInfoB.lastSessSector;
}

/**************************************************************************//**
 * \brief Retrieve last session serial from infoflash
 *****************************************************************************/
unsigned int infoGetLastEpoch(void)
{
	return _tempoInfoB.currEpoch;
}

/**************************************************************************//**
 * \brief Retrieve card ID from infoflash
 *
 * \param[out]	cid		Card ID string (of length #CARD_ID_LEN)
 *****************************************************************************/
void infoGetCardID(unsigned char * cid)
{
	memcpy(cid, _tempoInfoB.cid, CARD_ID_LEN);
}

/**************************************************************************//**
 * \brief Retrieve node ID from infoflash
 *****************************************************************************/
unsigned int infoGetNodeID(void)
{
	return _tempoInfoB.nodeID;
}

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
int infoCardInit(unsigned char* cid, unsigned int nodeID, unsigned int lastSector, unsigned int lastSessSector, unsigned int timeEpoch)
{
	memcpy(_tempoInfoB.cid, cid, CARD_ID_LEN);
	_tempoInfoB.nodeID = nodeID;
	_tempoInfoB.lastSector = lastSector;
	_tempoInfoB.lastSessSector = lastSessSector;
	_tempoInfoB.currEpoch = timeEpoch;
	_tempoInfoB.validityCode = INFO_VALID_CODE;

	return _writeTempoInfoB();
}




