/*
 * ftdi.h
 *
 *  Created on: Aug 8, 2013
 *      Author: bb3jd
 */

#ifndef FTDI_H_
#define FTDI_H_

#define BAUD_RATE 115200		///< Baud rate to run the UART communications at

typedef struct ftdi_packet_data{
	unsigned char *data;
	unsigned int len;
} ftdiPacket;

/**************************************************************************//**
 * \brief Initializes the UCA0 UART for use with an FT232 USB to UART bridge
 *
 * Creates a USCI "socket" for the FTDI chip and resets the module for use
 *
 * \return	commID 	FTDI communication ID
 * \retval 	-1		Registration has failed
 ******************************************************************************/
int ftdiInit(void);
/**************************************************************************//**
 * \brief Reads a specified number of bytes from the FTDI chip
 *
 * Spoofed "read" routine for FTDI chip, returns the number of bytes currently
 * available in the #ftdiBuff up to the amount of bytes requested.
 *
 * \param	len	The number of bytes requested to be read
 * \return	An FTDI receive packet (length and data)
 ******************************************************************************/
ftdiPacket ftdiRead(unsigned int len);
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
void ftdiWrite(ftdiPacket packet);
/**************************************************************************//**
 * \brief Reads a single byte from the FTDI chip RX buffer
 *
 * Spoofed "read" routine for FTDI chip, returns the number of bytes currently
 * available in the #ftdiBuff up to the amount of bytes requested.
 *
 * \return	The most recent single character from the FTDI buffer
 ******************************************************************************/
unsigned char ftdiGetch(void);
/**************************************************************************//**
 * \brief	Gets the number of bytes available from the FTDI chip
 *
 * Returns the number of bytes which have been written
 * to the RX pointer (since the last read performed).
 *
 * \return			The number of valid bytes in the receive buffer
 ******************************************************************************/
unsigned int ftdiGetBuffSize(void);
/**************************************************************************//**
 * \brief	Gets the status of the FTDI communications interface
 *
 * Returns the current USCI status of the FTDI UART communication interface
 *
 * \return			The current USCI status code
 ******************************************************************************/
unsigned char ftdiGetStatus(void);
/**************************************************************************//**
 * \brief	Clears buffer management for the FTDI UART channel
 ******************************************************************************/
void ftdiFlush(void);


#endif /* FTDI_H_ */
