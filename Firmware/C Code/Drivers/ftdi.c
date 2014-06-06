/*
 * ftdi.c
 *
 *  Created on: Aug 8, 2013
 *      Author: bb3jd
 */
#include "ftdi.h"
#include "comm.h"

unsigned int ftdiID;			///< Comm ID for the FTDI chip
unsigned char ftdiBuff[256];	///< Storage buffer for UART receive values
static usciConfig ftdiConf = {UCA0_UART, UART_8N1, DEF_CTLW1, UBR_DIV(BAUD_RATE), ftdiBuff};


/**************************************************************************//**
 * \brief Initializes the UCA0 UART for use with an FT232 USB to UART bridge
 *
 * Creates a USCI "socket" for the FTDI chip and resets the module for use
 *
 * \return	commID 	FTDI communication ID
 * \retval 	-1		Registration has failed
 ******************************************************************************/
int ftdiInit(void)
{
	ftdiID = registerComm(&ftdiConf);
	resetUCA0(ftdiID);						// Configure port
	return ftdiID;
}

/**************************************************************************//**
 * \brief Reads a specified number of bytes from the FTDI chip
 *
 * Spoofed "read" routine for FTDI chip, returns the number of bytes currently
 * available in the #ftdiBuff up to the amount of bytes requested.
 *
 * \param	len	The number of bytes requested to be read
 * \return	An FTDI receive packet (length and data)
 ******************************************************************************/
ftdiPacket ftdiRead(unsigned int len)
{
	ftdiPacket rxPack = {ftdiBuff,0};
	rxPack.len = uartA0Read(len, ftdiID);
	return rxPack;
}

/**************************************************************************//**
 * \brief Reads a single byte from the FTDI chip RX buffer
 *
 * Spoofed "read" routine for FTDI chip, returns the number of bytes currently
 * available in the #ftdiBuff up to the amount of bytes requested.
 *
 * \return	The most recent single character from the FTDI buffer
 ******************************************************************************/
unsigned char ftdiGetch(void)
{
	unsigned int i = uartA0Read(1, ftdiID);
	return ftdiBuff[i];
}

/**************************************************************************//**
 * \brief Writes a specified number of bytes to the FTDI chip
 *
 * Write routine for the FTDI chip. This function writes #len bytes following the
 * #*data pointer to the FTDI chip via UART
 *
 * \param 	*data	A pointer to the start of data to TX
 * \param	len		The number of bytes to transmit beginning at #*data
 * \return	The number of bytes available in the read buffer (#ftdiBuff)
 ******************************************************************************/
void ftdiWrite(ftdiPacket packet)
{
	while(uartA0Write(packet.data, packet.len, ftdiID) != 1);
}

/**************************************************************************//**
 * \brief	Gets the number of bytes available from the FTDI chip
 *
 * Returns the number of bytes which have been written
 * to the RX pointer (since the last read performed).
 *
 * \return			The number of valid bytes in the receive buffer
 ******************************************************************************/
unsigned int ftdiGetBuffSize(void)
{
	return getUCA0RxSize();
}

/**************************************************************************//**
 * \brief	Gets the status of the FTDI communications interface
 *
 * Returns the current USCI status of the FTDI UART communication interface
 *
 * \return			The current USCI status code
 ******************************************************************************/
unsigned char ftdiGetStatus(void){
	return getUCA0Stat();
}

/**************************************************************************//**
 * \brief	Clears buffer management for the FTDI UART channel
 ******************************************************************************/
void ftdiFlush(void)
{
	resetUCA0(ftdiID);						// Clear buffer management
	return;
}
