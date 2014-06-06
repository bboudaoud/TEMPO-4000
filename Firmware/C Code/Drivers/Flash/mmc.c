#ifndef _MMCLIB_C
#define _MMCLIB_C

#include "mmc.h"
#include "comm.h"
#include "hal.h"
#include <msp430.h>

// Function Prototypes
char mmcGetResponse(void);
char mmcGetXXResponse(const char resp);
char mmcCheckBusy(void);
char mmcGoIdle();

// Variables
int mmcID;																			///< MMC Comm Registration Number
unsigned char mmcRxBuff[MMC_RX_BUFF_SIZE];											///< Rx Buffer for communications
usciConfig mmcConf = {UCB0_SPI, SPI_8M2_BE, 0, MMC_INIT_BAUD, mmcRxBuff};			///< MMC USCI Config Structure

// Replacement macros for original SPI management in hal_SPI.c
#define spiSendByte(x)		spiB0Swap(x, mmcID)
void spiSendFrame(unsigned char *data, unsigned int len){
	while(spiB0Write(data, len, mmcID) == -1);		// Initialize transfer
	while(getUCB0Stat() != OPEN);					// Block until transfer complete
}
void spiReadFrame(unsigned char *data, unsigned int len){
	while(spiB0Read(len, mmcID) == -1);
	memcpy(data, mmcRxBuff, 512);
}

// Initialize MMC card
char mmcInit(void){
  //raise CS and MOSI for 80 clock cycles
  //SendByte(0xff) 10 times with CS high
  //RAISE CS
  int i;

  mmcID = registerComm(&mmcConf);
  resetUCB0(mmcID);
  MMC_CS_CONFIG();
  
  //initialization sequence on PowerUp
  MMC_CS_HIGH();
  for(i=0;i<=10;i++)
	  spiSendByte(DUMMY_CHAR);

  return (mmcGoIdle());
}


// set MMC in Idle mode
char mmcGoIdle() {
  int i;
  char response = 0x01;

  //Send Command 0 to put MMC in SPI mode
  MMC_CS_LOW();
  mmcSendCmd(MMC_GO_IDLE_STATE,0,0x95);

  //Now wait for READY RESPONSE
  if(mmcGetResponse()!=0x01){
	MMC_CS_HIGH();
    return MMC_INIT_ERROR;
  }
  // This timeout has been extended (transcend cards have i = 250-350 on init)
  for(i = 0; response == 0x01 && i<=1000; i++)
  {
    MMC_CS_HIGH();
    spiSendByte(DUMMY_CHAR);
    MMC_CS_LOW();
    mmcSendCmd(MMC_SEND_OP_COND,0x00,0xff);
    response = mmcGetResponse();
  }
  MMC_CS_HIGH();

  spiSendByte(DUMMY_CHAR);
  setUCB0Baud(MMC_DEF_BAUD, mmcID);

  switch(response){
    case 0:
	  return MMC_SUCCESS;
    case 1:
	  return MMC_TIMEOUT_ERROR;
    default:
	  return MMC_OTHER_ERROR;
  }
}

// MMC Get Response
char mmcGetResponse(void) {
  //Response comes 1-8bytes after command
  //the first bit will be a 0
  //followed by an error code
  //data will be 0xff until response
  int i;
  char response;

  for(i = 0; i<=64; i++) {
    response=spiSendByte(DUMMY_CHAR);
    if(response==0x00) break;
    if(response==0x01) break;
  }
  return response;
}

char mmcGetXXResponse(const char resp) {
  //Response comes 1-8bytes after command
  //the first bit will be a 0
  //followed by an error code
  //data will be 0xff until response
  int i;
  char response;

  for(i=0; i<=1000; i++) {
    response=spiSendByte(DUMMY_CHAR);
    if(response==resp)break;
  }
  return response;
}

unsigned int mmcGetR2Response(void) {
  //Response comes 1-8bytes after command
  //the first bit will be a 0
  //followed by an error code
  //data will be 0xff until response
  int i;
  unsigned char responseHi, responseLo;

  for(i=0; i<=64; i++) {
    responseHi=spiSendByte(DUMMY_CHAR);
    if(responseHi!=0xFF)
    	break;
  }
  responseLo=spiSendByte(DUMMY_CHAR);
  return ((unsigned int)(responseHi << 8) | responseLo);
}

// Check if MMC card is still busy
char mmcCheckBusy(void) {
  //Response comes 1-8bytes after command
  //the first bit will be a 0
  //followed by an error code
  //data will be 0xff until response
  int i;
  char response, rvalue;

  for(i=0; i<=64; i++) {
    response=spiSendByte(DUMMY_CHAR);
    response &= 0x1f;
    switch(response) {
      case 0x05:
    	  rvalue=MMC_SUCCESS;
    	  break;
      case 0x0b:
    	  return(MMC_CRC_ERROR);
      case 0x0d:
    	  return(MMC_WRITE_ERROR);
      default:
    	  rvalue = MMC_OTHER_ERROR;
    	  break;
    }
    if(rvalue==MMC_SUCCESS)
    	break;
  }

  i=0;
  do
  { // ADDRESS THIS ISSUE, WE GET HUNG HERE AND WATCHDOG AS A RESULT!!!!!!!!!!!!!!!!!
    response = spiSendByte(DUMMY_CHAR);
    i++;
  }while(response==0 && i<=10000);	// I HAVE NO IDEA WHAT TO MAKE THIS TIMEOUT ACTUALLY BE!!!
  
  if(response==0) {
  	rvalue = MMC_TIMEOUT_ERROR;
  }
  
  return rvalue;
}
// The card will respond with a standard response token followed by a data
// block suffixed with a 16 bit CRC.

// read a size Byte big block beginning at the address.
char mmcReadBlock(const unsigned long address, const unsigned long count, unsigned char *pBuffer) {
  char rvalue = MMC_RESPONSE_ERROR;

  // Set the block length to read
  if (mmcSetBlockLength (count) == MMC_SUCCESS) {   // Attempt to set block length
    MMC_CS_LOW ();
    // send read command MMC_READ_SINGLE_BLOCK=CMD17
    mmcSendCmd (MMC_READ_SINGLE_BLOCK,address, 0xFF);
    // Send 8 Clock pulses of delay, check if the MMC acknowledged the read block command
    // it will do this by sending an affirmative response
    // in the R1 format (0x00 is no errors)
    if (mmcGetResponse() == 0x00) {
      // Look for the data token to signify the start of data
      if (mmcGetXXResponse(MMC_START_DATA_BLOCK_TOKEN) == MMC_START_DATA_BLOCK_TOKEN) {
        // Clock the actual data transfer and receive the bytes; spi_read automatically finds the Data Block
        spiReadFrame(pBuffer, count);
        // Get CRC bytes (not really needed by us, but required by MMC)
        spiSendByte(DUMMY_CHAR);
        spiSendByte(DUMMY_CHAR);
        rvalue = MMC_SUCCESS;
      }
      else { // The data token was never received
        rvalue = MMC_DATA_TOKEN_ERROR;      // 3
      }
    }
    else {   // The MMC never acknowledge the read command
      rvalue = MMC_RESPONSE_ERROR;          // 2
    }
  }
  else {	// The block length was not set correctly
    rvalue = MMC_BLOCK_SET_ERROR;           // 1
  }
  MMC_CS_HIGH ();
  spiSendByte(DUMMY_CHAR);
  return rvalue;
}// mmc_read_block



//char mmcWriteBlock (const unsigned long address)
char mmcWriteBlock (const unsigned long address, const unsigned long count, unsigned char *pBuffer)
{
  char rvalue = MMC_RESPONSE_ERROR;         // MMC_SUCCESS;

  if (mmcSetBlockLength (count) == MMC_SUCCESS) {   // Set the block length to read
    MMC_CS_LOW ();
    mmcSendCmd (MMC_WRITE_BLOCK,address, 0xFF); // Send write command
    // Check if the MMC acknowledged the write block command
    // it will do this by sending an affirmative response
    // in the R1 format (0x00 is no errors)
    if (mmcGetXXResponse(MMC_R1_RESPONSE) == MMC_R1_RESPONSE) {
      spiSendByte(DUMMY_CHAR);
      spiSendByte(0xfe); // Send the data token to signify the start of the data
      spiSendFrame(pBuffer, count); // Clock the actual data transfer and transmit the bytes

      // Put CRC bytes (not really needed by us, but required by MMC)
      spiSendByte(DUMMY_CHAR);
      spiSendByte(DUMMY_CHAR);

      // Read the data response xxx0<status>1
      	  // Status = 010: Data accepted
      	  // Status 101: Data rejected due to a CRC error
      	  // Status 110: Data rejected due to a write error.
      rvalue = mmcCheckBusy();
      if(rvalue==MMC_SUCCESS) {
        // check status after write for any possible errors during write (see sandisk.pdf, p.63)
        MMC_CS_HIGH();
        spiSendByte(DUMMY_CHAR);
        MMC_CS_LOW();
        mmcSendCmd(MMC_SEND_STATUS, 0, 0xFF);
        if (mmcGetR2Response() != 0x0000) {
          rvalue = MMC_WRITE_ERROR;
        }
      }
    }
    else { // The MMC never acknowledge the write command
      rvalue = MMC_RESPONSE_ERROR;   // 2
    }
  }
  else {
    rvalue = MMC_BLOCK_SET_ERROR;   // 1
  }

  MMC_CS_HIGH();
  spiSendByte(DUMMY_CHAR); // Send 8 Clock pulses of delay.
  return rvalue;
} // mmc_write_block


// send command to MMC
void mmcSendCmd (const char cmd, unsigned long data, const char crc)
{
  unsigned char frame[6];
  char temp;
  int i;
  frame[0]=(cmd|0x40);
  for(i=3;i>=0;i--){
    temp=(char)(data>>(8*i));
    frame[4-i]=(temp);
  }
  frame[5]=(crc);
  spiSendFrame(frame,6);
}


// set blocklength 2^n
char mmcSetBlockLength (const unsigned long blocklength)
{
  char response;

  MMC_CS_LOW ();
  mmcSendCmd(MMC_SET_BLOCKLEN, blocklength, 0xFF);	  // Set the block length to read
  response=mmcGetResponse(); // Test response =  0x00 (R1 OK format)
  MMC_CS_HIGH ();
  spiSendByte(DUMMY_CHAR); 	// Send 8 Clock pulses of delay.
  return response;
} // Set block_length


// Reading the contents of the CSD and CID registers in SPI mode is a simple
// read-block transaction.
char mmcReadRegister (const char cmd_register, const unsigned char length, unsigned char *pBuffer)
{
  char uc;
  char rvalue = MMC_TIMEOUT_ERROR;

  if (mmcSetBlockLength (length) == MMC_SUCCESS)
  {
    MMC_CS_LOW ();
    mmcSendCmd(cmd_register, 0x000000, 0xff);	// CRC not used: 0xff as last byte
    if (mmcGetResponse() == 0x00) {	// Wait for R1 response (0x00 = OK)
      if (mmcGetXXResponse(0xfe)== 0xfe)
        for (uc = 0; uc < length; uc++)
          pBuffer[uc] = spiSendByte(DUMMY_CHAR);
      // get CRC bytes (not really needed by us, but required by MMC)
      spiSendByte(DUMMY_CHAR);
      spiSendByte(DUMMY_CHAR);
      rvalue = MMC_SUCCESS;
    }
    else
      rvalue = MMC_RESPONSE_ERROR;
    MMC_CS_HIGH ();

    spiSendByte(DUMMY_CHAR);	 // Send 8 Clock pulses of delay.
  }
  MMC_CS_HIGH ();
  return rvalue;
} // mmc_read_register


#include "math.h"
unsigned long mmcReadCardSize(void)
{
  // Read contents of Card Specific Data (CSD)
  int timeout = 0;
  unsigned long MMC_CardSize;
  unsigned short i,      // index
                 j,      // index
                 b,      // temporary variable
                 response,   // MMC response to command
                 mmc_C_SIZE;

  unsigned char mmc_READ_BL_LEN,  // Read block length
                mmc_C_SIZE_MULT;

  MMC_CS_LOW ();

  spiSendByte(MMC_READ_CSD);   // CMD 9
  for(i=4; i>0; i--) spiSendByte(0);  // Send four dummy byte
  spiSendByte(DUMMY_CHAR);   // Send CRC byte (why not just add one more byte above????)

  response = mmcGetResponse();
  b = spiSendByte(DUMMY_CHAR);   // data transmission always starts with 0xFE
  if(!response) {	// Response != 0xFF
    while (b != 0xFE ){
    	 b = spiSendByte(DUMMY_CHAR);
    	 if (timeout++ >= 150)
    	 	return 0;
    }
    // bits 127:87
    for(j=5; j>0; j--)          // Host must keep the clock running for at ?????
      b = spiSendByte(DUMMY_CHAR);

    // 4 bits of READ_BL_LEN
    // bits 84:80
    b =spiSendByte(DUMMY_CHAR);  // lower 4 bits of CCC and
    mmc_READ_BL_LEN = b & 0x0F;
    b = spiSendByte(DUMMY_CHAR);
    // bits 73:62  C_Size
    // xxCC CCCC CCCC CC
    mmc_C_SIZE = (b & 0x03) << 10;
    b = spiSendByte(DUMMY_CHAR);
    mmc_C_SIZE += b << 2;
    b = spiSendByte(DUMMY_CHAR);
    mmc_C_SIZE += b >> 6;
    // bits 55:53
    b = spiSendByte(DUMMY_CHAR);
    // bits 49:47
    mmc_C_SIZE_MULT = (b & 0x03) << 1;
    b = spiSendByte(DUMMY_CHAR);
    mmc_C_SIZE_MULT += b >> 7;
    // bits 41:37
    b = spiSendByte(DUMMY_CHAR);
    b = spiSendByte(DUMMY_CHAR);
    b = spiSendByte(DUMMY_CHAR);
    b = spiSendByte(DUMMY_CHAR);
    b = spiSendByte(DUMMY_CHAR);
  }

  for(j=4; j>0; j--)          // Host must keep the clock running for at
    b = spiSendByte(DUMMY_CHAR);  // least Ncr (max = 4 bytes) cycles after
                               // the card response is received
  b = spiSendByte(DUMMY_CHAR);
  MMC_CS_LOW ();

  MMC_CardSize = (mmc_C_SIZE + 1);
  // power function with base 2 is better with a loop
  // i = (pow(2,mmc_C_SIZE_MULT+2)+0.5);
  for(i = 2,j=mmc_C_SIZE_MULT+2; j>1; j--)
    i <<= 1;
  MMC_CardSize *= i;
  // power function with base 2 is better with a loop
  //i = (pow(2,mmc_READ_BL_LEN)+0.5);
  for(i = 2,j=mmc_READ_BL_LEN; j>1; j--)
    i <<= 1;
  MMC_CardSize *= i;

  return (MMC_CardSize);
}

/* This function does not communicate with MMC, simply tests card detect pin which is unused
char mmcPing(void)
{
  if (!(MMC_CD_PxIN & MMC_CD))
    return (MMC_SUCCESS);
  else
    return (MMC_INIT_ERROR);
}*/

//---------------------------------------------------------------------
#endif /* _MMCLIB_C */
