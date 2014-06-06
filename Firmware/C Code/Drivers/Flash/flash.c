#include "flash.h"
#include "mmc.h"
#include "hal.h"

/**************************************************************************//**
 * \brief Initialize flash card for communication.
 *
 * Attempt
 *
 * \retval	0	Success
 * \retval	-1	Failure (timeout)
 * \sideeffect	Sets #readOnly if not successful
 *****************************************************************************/
int flashInit()
{
	unsigned int timeout = 0;

	MMC_CD_CONFIG();
	if(!MMC_CARD_PRESENT) return FLASH_NO_CARD;

	for(timeout = 0; timeout < INIT_TIMEOUT; timeout++){ 	// Try initialization up to 50 times
		if(mmcInit() == MMC_SUCCESS) break;		// If it occurs successfully we are done here
	}

	if(timeout >= INIT_TIMEOUT) return FLASH_TIMEOUT;
	else return FLASH_SUCCESS;
}

/**************************************************************************//**
 * \brief Read a single sector from the flash card
 *
 * Attempt to read a full sector (including checksum)
 *
 * Provides a clean read wrapper separating the rest of the codebase from
 * the MMC library.
 *
 * \param		sectorNum	The sector number to read in (0 to 4194303 for a
 * 							2GB MMC card with 512B sector sizes).
 * \param[out]	sector		Pointer to a sector structure to fill. Most
 * 							likely you will be casting a more specific sector
 * 							type or a simple array to this interface
 * \retval		0 success
 * \retval		-1 failure
 *****************************************************************************/
int flashRead(unsigned long sectorNum, struct Sector *sector){
	if(mmcReadSector(sectorNum, (unsigned char *) sector) == MMC_SUCCESS) {
		return FLASH_SUCCESS;
	}
	else {
		return FLASH_FAIL;
	}
}

/**************************************************************************//**
 * \brief Read a sector and verify the checksum
 *
 * Read a sector and, if successful, verify its checksum as well. After
 * #READ_RETRIES failed read-verify attempts exit
 *
 * \param		sectorNum	The sector number to read in (0 to 4194303 for a
 * 							2GB MMC card with 512B sector sizes).
 * \param[out]	sector		Pointer to a sector structure to fill. Most
 * 							likely you will be casting a more specific sector
 * 							type or a simple array to this interface
 * \retval		0 success
 * \retval		-1 failure
 *****************************************************************************/
int secureFlashRead(unsigned long sectorNum, struct Sector *sector){
	int calcChecksum = 0;
	int i = 0;

	for( i = 0; i < READ_RETRIES; i++)				// Attempt to read the card multiple times
	{
		if(flashRead(sectorNum, sector) == 0) {
			calcChecksum = fletcherChecksum(sector->data, SECTOR_CRC_SIZE, 0);
			if(sector->checksum == calcChecksum){
				return FLASH_SUCCESS;
			}
		}
	}

	if(i >= READ_RETRIES) return FLASH_TIMEOUT;	// If checksums do not agree return fail
	return FLASH_SUCCESS;
}

/**************************************************************************//**
 * \brief Write a single sector to the flash card
 *
 * Attempt to write a full sector (including computed checksum)
 *
 * Provides a clean read wrapper separating the rest of the codebase from
 * the MMC library.
 *
 * \param			sectorNum	The sector number to read in (0 to 4194303
 * 								for a 2GB MMC card with 512B sector sizes).
 * \param[in,out]	sector		Pointer to sector to write. Will have a newly
 * 								computed checksum written into it. Most
 * 								likely you will be casting a more specific
 * 								sector type or a simple array to this
 * 								interface
 * \retval		0 success
 * \retval		-1 failure (MMC library returned an error)
 * \see			secureFlashWrite() for more thorough write validation
 *****************************************************************************/
int flashWrite(unsigned long sectorNum, struct Sector *sector)
{
	sector->checksum = fletcherChecksum(sector->data, SECTOR_CRC_SIZE, 0);

	if(mmcWriteSector(sectorNum, (unsigned char *) sector) == MMC_SUCCESS) {
		return FLASH_SUCCESS;
	}
	else {
		return FLASH_FAIL;
	}
}

/**************************************************************************//**
 * \brief Write a sector and read it back to verify correctness
 *
 * Write a sector and, if successful, read it back to verify that it passes
 * a checksum check and has the same checksum as before After #WRITE_RETRIES
 * failed write-verify attempts exit
 *
 * \param			sectorNum	The sector number to read in (0 to 4194303
 * 								for a 2GB MMC card with 512B sector sizes).
 * \param[in,out]	sector		Pointer to sector to write. Will have a newly
 * 								computed checksum written into it. Most
 * 								likely you will be casting a more specific
 * 								sector type or a simple array to this
 * 								interface
 * \retval		0 success
 * \retval		-1 failure
 *****************************************************************************/
int secureFlashWrite(unsigned long sectorNum, struct Sector *sector){
	int postChecksum = 0;
	struct Sector tmpSector;
	int i = 0;

	sector->checksum = fletcherChecksum(sector->data, SECTOR_CRC_SIZE, 0);

	for( i = 0; i < WRITE_RETRIES; i++)
	{
		if(mmcWriteSector(sectorNum, (unsigned char *) sector)==MMC_SUCCESS) {
			mmcReadSector(sectorNum, (unsigned char *) &tmpSector);
			postChecksum = fletcherChecksum(tmpSector.data, SECTOR_CRC_SIZE, 0);

			if(tmpSector.checksum == postChecksum &&		// Read-back data passes checksum
					sector->checksum == postChecksum) {		// && Read-back matches original
					return FLASH_SUCCESS;
			}
		}
	}
	// If checksums do not agree and we have tried multiple times, then..
	return FLASH_TIMEOUT;						// Return failure

}

/**************************************************************************//**
 * \brief Read the card ID directly from the MMC card
 *
 *	This function reads the flash card ID directly from the MMC register inside
 *	the card. It includes some simple all 0/1 detection for faulty values.
 *
 * \param[out]	cardID		16-byte buffer to store the card ID into
 * \retval	0	Card ID read successfully
 * \retval	-1	Card ID could not be read
 * \retval	-2	Card ID read as all 0's
 * \retval  -3	Card ID read as all 1's
 *****************************************************************************/
int flashReadCardID(unsigned char *cardID)
{
	unsigned int i;

	if(mmcReadRegister(MMC_SEND_CID, CARD_ID_LEN, cardID) == MMC_SUCCESS) {
		for(i = 0; i < CARD_ID_LEN; i++) {
			if(cardID[i] != 0) break;
		}
		if(i == CARD_ID_LEN) return -2;

		for(i = 0; i < CARD_ID_LEN; i++) {
			if(cardID[i] != 0xFF) break;
		}
		if(i == CARD_ID_LEN) return -3;

		return 0;
	}
	else return -1;
}
