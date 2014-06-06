#ifndef FLASH_H_
#define FLASH_H_

// Read, Write, and Init Retry Counts (for flashInit() and secure read/write)
#define	INIT_TIMEOUT		50										///< Number of retries before flash init timeout
#define	WRITE_RETRIES		3										///< Write retry limit for secureFlashWrite()
#define	READ_RETRIES		3										///< Read retry limit for secureFlashRead()

// Flash Info and Locations
#define	CARD_SIZE			2000000000								///< Card size in bytes (NOTE: 2*10^9 not 2^31)
#define	SECTOR_SIZE			512										///< True sector size
#define	SECTOR_CRC_SIZE		SECTOR_SIZE-sizeof(unsigned int) 		///< Size of space in sector to CRC
#define SECTOR_DATA_SIZE	SECTOR_SIZE-(2*sizeof(unsigned int))	///< Available sector data
#define CARD_BUFFER_SIZE	2*SECTOR_DATA_SIZE						///< Declared size of flash write buffer
#define	CARD_ID_LEN			16										///< Card ID length in bytes (always 16 for MMC)
#define	TOTAL_SECTORS		(CARD_SIZE/SECTOR_SIZE)					///< Last sector on card

/// Flash return codes
#define FLASH_NO_CARD		-1					///< Flash card failure: no card present (via CD pin state)
#define FLASH_TIMEOUT		-2					///< Flash card failure: communications timeout (via MMC libs)
#define FLASH_FAIL			-3					///< Flash card single failure
#define FLASH_SUCCESS		0					///< Flash card operation succeeded

/// Generic flash sector data structure
typedef enum sectorTypeDefine					/// Enumerated type for defining sector parsing
{
	sect_nodeInfo = 1,							///< Node info sector (contains global info)
	sect_cardInfo = 2,							///< Card info sector (contains global info)
	sect_sessInfo = 3,							///< Session info sector (contains session info)
	sect_data = 4								///< Data sector (contains session data)
}sectorType;

struct Sector									/// Generic sector structure
{
	sectorType type;							///< Sector type code
	unsigned char data[SECTOR_DATA_SIZE];		///< Raw contents of the sector
	unsigned int checksum;						///< Checksum tacked on the end
};

int flashInit();
int flashRead(unsigned long sectorNum, struct Sector *sector);
int secureFlashRead(unsigned long sectorNum, struct Sector *sector);
int flashWrite(unsigned long sectorNum, struct Sector *sector);
int secureFlashWrite(unsigned long sectorNum, struct Sector *sector);
int flashReadCardID(unsigned char *cardID);
unsigned int fletcherChecksum(unsigned char *Buffer, int numBytes, unsigned int checksum);

#endif /* FLASH_H_ */
