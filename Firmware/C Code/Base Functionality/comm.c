/****************************************************************//**
 * \file comm.c
 *
 * \author 	Ben Boudaoud
 * \date 	January 2013
 *
 * \brief 	This library provides functions for interfacing MSP430
 * 			USCI modules in a relatively intuitive way.
 *
 * When a module is to be used, first the application must register
 * a communications ID. The #registerComm function is used to pass in
 * a configuration for the USCI module (see #usciConfig). In return
 * #registerComm passes back a comm ID which can be used to access the
 * USCI. Currently the library supports UART, SPI, and I2C single master
 * modes.
 *******************************************************************/
#include "comm.h"

usciConfig *dev[MAX_DEVS];							///< Device config buffer (indexed by comm ID always non-zero)
unsigned int devIndex = 0;							///< Device config buffer index
unsigned int devConf[4] = {0,0,0,0};				///< Currently applied configs buffer [A0, A1, B0, B1]
usciStatus usciStat[4] = {OPEN, OPEN, OPEN, OPEN};	///< Store status (OPEN, TX, or RX) for [A0, A1, B0, B1]

/**************************************************************************//**
 * \fn int registerComm(usciConfig *conf)
 * \brief Registers an application for use of a USCI module.
 *
 * Create a USCI "socket" by affiliating a unique comm ID with an
 * endpoint configuration for TI's eUSCI module.
 *
 * \param	conf	The USCI configuration structure to be used (see comm.h)
 * \return	commID 	A positive (> 0) value representing the registered
 * 					app
 * \retval 	-1	The maximum number of apps (MAX_DEVS) has been registered
 ******************************************************************************/
int registerComm(usciConfig *conf)
{
	if(devIndex >= MAX_DEVS) return -1;	// Check device list not full
	dev[++devIndex] = conf;			// Copy config pointer into device list
	return devIndex;
}

/****************************************************************
 * USCI A0 Variable Declarations
 ***************************************************************/
#ifdef USE_UCA0
unsigned char *uca0TxPtr;				///< USCI A0 TX Data Pointer
unsigned char *uca0RxPtr;				///< USCI A0 RX Data Pointer
unsigned int uca0TxSize = 0;			///< USCI A0 TX Size
unsigned int uca0RxSize = 0;			///< USCI A0 RX Size
// Conditional SPI Receive size
#ifdef USE_UCA0_SPI
unsigned int spiA0RxSize = 0;			///< USCI A0 To-RX Size (used for SPI RX)
#endif //USE_UCA0_SPI

/**************************************************************
 * General Purpose USCI A0 Functions
 *************************************************************/
// NOTE: This configuration is safe for use with single end-point UART and SPI config
/**************************************************************************//**
 * \brief	Configures USCI A0 for operation
 *
 * Write control registers and clear system variables for the
 * USCI A0 module, which can be used as either a UART or SPI.
 *
 * NOTE: This config function is already called before any read/write function call
 * and therefore should (in almost all cases) never be called by the user.
 *
 * \param	commID	The communication ID for the registered app
 ******************************************************************************/
void confUCA0(unsigned int commID)
{
	unsigned int status;

	if(devConf[UCA0_INDEX] == commID) return;		// Check if device is already configured
	enter_critical(status);							// Perform config in critical section
	UCA0CTL1 |= UCSWRST;							// Pause operation
	UCA0_IO_CLEAR();								// Clear I/O for configuration

	// Configure key control words
	UCA0CTLW0 = dev[commID]->usciCtlW0 | UCSWRST;
#ifdef UCA0CTLW1	// Check for control word 1 define (eUSCI vs USCI) future patch
	UCA0CTLW1 = dev[commID]->usciCtlW1;
#endif // UCA0CTLW1
	UCA0BRW = dev[commID]->baudDiv;
	uca0RxPtr = dev[commID]->rxPtr;

	// Clear buffer sizes
	uca0RxSize = 0;
	uca0TxSize = 0;
#ifdef USE_UCA0_SPI
	spiA0RxSize = 0;
#endif //USE_UCA0_SPI

	UCA0_IO_CONF(dev[commID]->rAddr & ADDR_MASK);	// Port set up
	UCA0CTL1 &= ~UCSWRST;							// Resume operation (clear software reset)
	UCA0IFG = 0;									// Clear any previously existing interrupt flags
	UCA0IE |= UCRXIE + UCTXIE;						// Enable Interrupts

	devConf[UCA0_INDEX] = commID;					// Store config
	exit_critical(status);							// End critical section
}
/**************************************************************************//**
 * \brief	Resets USCI A0 without writing over control regs
 *
 * This function is included to soft-reset the USCI A0 module
 * management variables without clearing the current config.
 * 
 * \param	commID	The comm ID of the registered app
 * \sideeffect	Sets the RX pointer to that registered w/ commID
 ******************************************************************************/
void resetUCA0(unsigned int commID){
	uca0RxPtr = dev[commID]->rxPtr;
	uca0RxSize = 0;
	uca0TxSize = 0;
#ifdef USE_UCA0_SPI
	spiA0RxSize = 0;
#endif //USE_UCA0_SPI
	usciStat[UCA0_INDEX] = OPEN;
	return;
}
/**************************************************************************//**
 * \brief	Get method for USCI A0 RX buffer size
 *
 * Returns the number of bytes which have been written
 * to the RX pointer (since the last read performed).
 *
 * \return			The number of valid bytes following the rxPtr.
 ******************************************************************************/
unsigned int getUCA0RxSize(void){
	return uca0RxSize;
}
/**************************************************************************//**
 * \brief	Get method for USCI A0 status
 *
 * Returns the status of the USCI module, either OPEN, TX, or RX
 *
 * \retval	0	Indicates the OPEN status
 * \retval 	1	Indicates the TX Status
 * \retval 	2	Indicates the RX Status
 ******************************************************************************/
unsigned char getUCA0Stat(void){
	return usciStat[UCA0_INDEX];
}

/*************************************************************************//**
 * \brief	Set method for USCI A0 Baud Rate Divisor
 *
 * Sets the baud rate divisor of the USCI module, this divisor is generally
 * performed relative to the SMCLK rate of the system.
 *
 * \param 	baudDiv	The new divisor to apply
 * \param	commID	The communications ID number of the application
 *****************************************************************************/
void setUCA0Baud(unsigned int baudDiv, unsigned int commID){
	dev[commID]->baudDiv = baudDiv;		// Replace the baud divisor in memory
	devConf[UCA0_INDEX] = 0;		// Reset the device config storage (config will be performed on next read/write)
	return;
}
/***************************************************************
* UCA0 UART HANDLERS
 **************************************************************/
#ifdef USE_UCA0_UART
/**************************************************************************//**
 * \brief	Transmit method for USCI A0 UART operation
 *
 * This method initializes the transmission of len bytes from
 * the base of the *data pointer. The actual transmission itself
 * is finished a variable length of time from the write (based
 * upon len's value) in the TX ISR. Thus calling uartA0Write() twice in quick
 * succession will likely result in partial transmission of the first data.
 *
 * \param	*data	Pointer to data to be written
 * \param	len		Length (in bytes) of data to be written
 * \param	commID	Communication ID number of application
 *
 * \retval	-2		Incorrect resource code
 * \retval	-1		USCI A0 module busy
 * \retval	1		Transmit successfully started
 ******************************************************************************/
int uartA0Write(unsigned char *data, unsigned int len, unsigned int commID)
{
	if(usciStat[UCA0_INDEX] != OPEN) return -1;		// Check that the USCI is available

	confUCA0(commID);

	// Copy over pointer and length
	uca0TxPtr = data;
	uca0TxSize = len-1;
	// Write TXBUF (start of transmit) and set status
	usciStat[UCA0_INDEX] = TX;
	UCA0TXBUF = *uca0TxPtr;

	return 1;
}
/**************************************************************************//**
 * \brief	Receive method for USCI A0 UART operation
 *
 * This method spoofs an asynchronous read by providing the min of the
 * bytes available and the requested length. It decrements the buffer size
 * appropriately and returns bytes "read".
 *
 * \param		len	The number of bytes to be read from the buffer
 * \param		commID	The comm ID of the application
 * \return		The number of bytes available to read in the buffer. If the buffer
 * 				is empty this value will be 0.
 * \sideeffect	The uca0RxSize variable is decremented by the min of itself and
 * 				the requested amount of bytes (len).
 * \note		This function does not make a call to #confUCA0 or check it the
 * 				state of the USCI module as it does  not interact with the
 * 				hardware module.
 ******************************************************************************/
int uartA0Read(unsigned int len, unsigned int commID)
{
	// Read length determination = max(requested, available)
	if(len > uca0RxSize) {
		len = uca0RxSize;
	}
	uca0RxSize -= len;
	uca0RxPtr -= len;

	return len;
}
#endif // USE_UCA0_UART
/***********************************************************
 * UCA0 SPI HANDLERS
 ***********************************************************/
#ifdef USE_UCA0_SPI
/**************************************************************************//**
 * \brief	Transmit method for USCI A0 SPI operation
 *
 * This method initializes a transmission of len bytes from the base of the
 * *data pointer. Similarly to uartA0Write(), the transmission uses the USCI A0
 * TX ISR to complete, so 2 sequential calls may result in partial transmission.
 *
 * \param	*data	Pointer to data to be written
 * \param	len		Length (in bytes) of data to be written
 * \param 	commID	Communication ID number of application
 *
 * \retval	-1		USCI A0 Module busy
 * \retval	1		Transmit successfully started
 *******************************************************************************/
int spiA0Write(unsigned char *data, unsigned int len, unsigned int commID)
{
	if(usciStat[UCA0_INDEX] != OPEN) return -1;	// Check that USCI is available

	confUCA0(commID);
	
	// Copy over pointer and length
	uca0TxPtr = data;
	uca0TxSize = len-1;
	// Start of TX
	usciStat[UCA0_INDEX] = TX;
	UCA0TXBUF = *uca0TxPtr;

	return 1;
}
/**************************************************************************//**
 * \brief	Receive method for USCI A0 SPI operation
 *
 * This method performs a synchronous read by storing the bytes to be read in
 * spiA0RxSize and then performing len dummy write to the bus to fetch the data
 * from a slave device. The RX size is cleared on this function call.
 *
 * \param	len	The number of bytes to be read from the bus
 * \param	commID	Communication ID number of the application
 *
 * \retval	-1	USCI A0 Module Busy
 * \retval	1	Receive successfully started
 *
 * \sideeffect	Reset the UCA0 RX size and data pointer
 ******************************************************************************/
int spiA0Read(unsigned int len, unsigned int commID)
{
	if(usciStat[UCA0_INDEX] != OPEN) return -1;	// Check that USCI is available

	confUCA0(commID);
	
	// Clear RX Size/Buff and copy length
	uca0RxSize = 0;					// Reset the rx size
	uca0RxPtr = dev[commID]->rxPtr;			// Reset the rx pointer
	spiA0RxSize = len-1;
	// Start of RX
	usciStat[UCA0_INDEX] = RX;
	UCA0TXBUF = 0xFF;				// Start TX
	return 1;
}
/**************************************************************************//**
 * \brief	Byte Swap method for USCI A0 SPI operation
 *
 * This blocking method allows the user to transmit a byte and receive the
 * response simultaneously clocked back in. TX and RX buffer sizes/content
 * are unaffected.
 *
 * \param	byte	The byte to be sent via SPI
 * \param 	commID	Communication ID number of the application
 *
 * \return	The byte shifted in from the SPI
 *************************************************************************/
unsigned char spiA0Swap(unsigned char byte, unsigned int commID)
{
	if(usciStat[UCA0_INDEX] != OPEN) return -1;	// Check that USCI is available

	confUCA0(commID);
	
	usciStat[UCA0_INDEX] = SWAP;		// Set USCI status to swap (prevent other operations)
	UCA0TXBUF = byte;
	while(UCA0STAT & UCBUSY);			// Wait for TX complete
	usciStat[UCA0_INDEX] = OPEN;		// Set USCI status to open (swap complete)
	return UCA0RXBUF;					// Return RX contents
}
#endif //USE_UCA0_SPI
/**********************************************************************//**
 * \brief	USCI A0 RX/TX Interrupt Service Routine
 *
 * This ISR manages all TX/RX proceedures with the exception of transfer
 * initialization. Once a transfer (read or write) is underway, this method
 * assures the correct amount of bytes are written to the correct location.
 *************************************************************************/
#pragma vector=USCI_A0_VECTOR
__interrupt void usciA0Isr(void)
{
	unsigned int dummy = 0xFF;
	// Transmit Interrupt Flag Set
	if(UCA0IFG & UCTXIFG){
#ifdef USE_UCA0_SPI
		if(usciStat[UCA0_INDEX] == TX){
#endif
		if(uca0TxSize > 0){
			UCA0TXBUF = *(++uca0TxPtr);		// Transmit the next outgoing byte
			uca0TxSize--;
		}
		else{
			UCA0IFG &= ~UCTXIFG;			// Clear TX interrupt flag from vector on end of TX
			usciStat[UCA0_INDEX] = OPEN; 		// Set status open if done with transmit
		}
#ifdef USE_UCA0_SPI
		}
#endif
	}

	// Receive Interrupt Flag Set
	if(UCA0IFG & UCRXIFG){
#ifdef USE_UCA0_SPI
		if(usciStat[UCA0_INDEX] == RX){			// Check we are in RX mode for SPI
#endif // USE_UCA0_SPI
		if(UCA0STAT & UCRXERR) dummy = UCA0RXBUF;	// RX ERROR: Do a dummy read to clear interrupt flag
		else {						// Otherwise write the value to the RX pointer
			*(uca0RxPtr++) = UCA0RXBUF;
			uca0RxSize++;				// RX Size decrement in read function
#ifdef USE_UCA0_SPI
			if(uca0RxSize < spiA0RxSize) UCA0TXBUF = dummy; // Perform another dummy write
			else
#endif
			usciStat[UCA0_INDEX] = OPEN;
		}
#ifdef USE_UCA0_SPI
		}
#endif
	}
	UCA0IFG &= ~UCRXIFG;	// Clear RX interrupt flag from vector on end of RX
}
#endif // USE_UCA0


/****************************************************************
 * USCI A1 Variable Declarations
 ***************************************************************/
#ifdef USE_UCA1
unsigned char *uca1TxPtr;		///< USCI A1 TX Data Pointer
unsigned char *uca1RxPtr;		///< USCI A1 RX Data Pointer
unsigned int uca1TxSize = 0;		///< USCI A1 TX Size
unsigned int uca1RxSize = 0;		///< USCI A1 RX Size
// Conditional SPI Receive size
#ifdef USE_UCA1_SPI
unsigned int spiA1RxSize = 0;		///< USCI A1 To-RX Size (used for SPI RX)
#endif //USE_UCA1_SPI

/**************************************************************
 * General Purpose USCI A1 Functions
 *************************************************************/
/**************************************************************************//**
 * \brief	Configures USCI A1 for operation
 *
 * Write control registers and clear system variables for the
 * USCI A1 module, which can be used as either a UART or SPI.
 *
 * NOTE: This config function is already called before any read/write function call
 * and therefore should (in almost all cases) never be called by the user.
 *
 * \param	commID	The communication ID for the registered app
 ******************************************************************************/
void confUCA1(unsigned int commID)
{
	unsigned int status;
	if(devConf[UCA1_INDEX] == commID) return;		// Check if device is already configured
	enter_critical(status);					// Perform config in critical section
	UCA1CTL1 |= UCSWRST;					// Pause operation
	UCA1_IO_CLEAR();					// Clear I/O for config

	// Configure key control words
	UCA1CTLW0 = dev[commID]->usciCtlW0 | UCSWRST;
#ifdef UCA1CTLW1	// Check for UCA1CTLW1 defined
	UCA1CTLW1 = dev[commID]->usciCtlW1;
#endif // UCA1CTLW1
	UCA1BRW = dev[commID]->baudDiv;
	uca1RxPtr = dev[commID]->rxPtr;

	// Clear buffer sizes
	uca1RxSize = 0;
	uca1TxSize = 0;
#ifdef USE_UCA1_SPI
	spiA1RxSize = 0;
#endif //USE_UCA1_SPI

	UCA1_IO_CONF(dev[commID]->rAddr & ADDR_MASK);		// Port set up
	UCA1CTL1 &= ~UCSWRST;					// Resume operation
	UCA1IFG = 0;							// Clear any previously existing interrupt flags
	UCA1IE |= UCRXIE + UCTXIE;				// Enable Interrupts	

	devConf[UCA1_INDEX] = commID;				// Store config
	exit_critical(status);					// End critical section
}

/**************************************************************************//**
 * \brief	Resets USCI A1 without writing over control regs
 *
 * This function is included to sof-reset the USCI A1 module
 * management variables without clearing the current config.
 *
 * \param	commID	The comm ID of the registered app
 * \sideeffect	Sets the RX pointer to that registered w/ commID
 ******************************************************************************/
void resetUCA1(unsigned int commID){
	uca1RxPtr = dev[commID]->rxPtr;
	uca1RxSize = 0;
	uca1TxSize = 0;
#ifdef USE_UCA1_SPI
	spiA1RxSize = 0;
#endif //USE_UCA1_SPI
	usciStat[UCA1_INDEX] = OPEN;
	return;
}
/**************************************************************************//**
 * \brief	Get method for USCI A1 RX buffer size
 *
 * Returns the number of bytes which have been written
 * to the RX pointer (since the last read performed).
 *
 * \return	The number of valid bytes following the rxPtr.
 ******************************************************************************/
unsigned int getUCA1RxSize(void){
	return uca1RxSize;
}
/**************************************************************************//**
 * \brief	Get method for USCI A1 status
 *
 * Returns the status of the USCI module, either OPEN, TX, or RX
 *
 * \retval	0	Indicates the OPEN status
 * \retval 	1	Indicates the TX Status
 * \retval 	2	Indicates the RX Status
 ******************************************************************************/
unsigned char getUCA1Stat(void){
	return usciStat[UCA1_INDEX];
}

/*************************************************************************//**
 * \brief	Set method for USCI A1 Baud Rate Divisor
 *
 * Sets the baud rate divisor of the USCI module, this divisor is generally
 * performed relative to the SMCLK rate of the system.
 *
 * \param 	baudDiv	The new divisor to apply
 * \param	commID	The communications ID number of the application
 *****************************************************************************/
void setUCA1Baud(unsigned int baudDiv, unsigned int commID){
	dev[commID]->baudDiv = baudDiv;		// Replace the baud divisor in memory
	devConf[UCA1_INDEX] = 0;		// Reset the device config storage (config will be performed on next read/write)
	return;
}
/***************************************************************
* UCA1 UART HANDLERS
***************************************************************/
#ifdef USE_UCA1_UART
/**************************************************************************//**
 * \brief	Transmit method for USCI A1 UART operation
 *
 * This method initializes the transmission of len bytes from
 * the base of the *data pointer. The actual transmission itself
 * is finished a variable length of time from the write (based
 * upon len's value) in the TX ISR. Thus calling uartA1Write() twice in quick
 * succession will likely result in partial transmission of the first data.
 *
 * \param	*data	Pointer to data to be written
 * \param	len		Length (in bytes) of data to be written
 * \param	commID	Communication ID number of application
 *
 * \retval	-2		Incorrect resource code
 * \retval	-1		USCI A1 module busy
 * \retval	1		Transmit successfully started
 ******************************************************************************/
int uartA1Write(unsigned char *data, unsigned int len, unsigned int commID)
{
	if(usciStat[UCA1_INDEX] != OPEN) return -1;	// Check that the USCI is available

	confUCA1(commID);

	// Copy over pointer and length
	uca1TxPtr = data;
	uca1TxSize = len-1;
	// Write TXBUF (start of transmit) and set status
	usciStat[UCA1_INDEX] = TX;
	UCA1TXBUF = *uca1TxPtr;

	return 1;
}
/**************************************************************************//**
 * \brief	Receive method for USCI A1 UART operation
 *
 * This method spoofs an asynchronous read by providing the min of the
 * bytes available and the requested length. It decrements the buffer size
 * appropriately and returns bytes "read".
 *
 * \param	len		The number of bytes to be read from the buffer
 * \param	commID		The comm ID of the application
 * \return	The number of bytes available to read in the buffer. If the buffer
 * 		is empty this value will be 0.
 * \sideeffect	The ucA1RxSize variable is decremented by the min of itself and
 * 		the requested amount of bytes (len).
 * \sa		This function does not make a call to #confUCA0 or check it the
 * 		state of the USCI module as it does  not interact with the
 * 		hardware module.
 ******************************************************************************/
int uartA1Read(unsigned int len, unsigned int commID)
{
	if(len > uca1RxSize) {
		len = uca1RxSize;
	}
	uca1RxSize -= len;
	return len;
}
#endif // USE_UCA1_UART
/***********************************************************
 * UCA1 SPI HANDLERS
 ***********************************************************/
#ifdef USE_UCA1_SPI
/**************************************************************************//**
 * \brief	Transmit method for USCI A1 SPI operation
 *
 * This method initializes a transmission of len bytes from the base of the
 * *data pointer. Similarly to uartA1Write(), the transmission uses the USCI A1
 * TX ISR to complete, so 2 sequential calls may result in partial transmission.
 *
 * \param	*data	Pointer to data to be written
 * \param	len		Length (in bytes) of data to be written
 * \param 	commID	Communication ID number of application
 *
 * \retval	-1		USCI A1 Module busy
 * \retval	1		Transmit successfully started
 *******************************************************************************/
int spiA1Write(unsigned char *data, unsigned int len, unsigned int commID)
{
	if(usciStat[UCA1_INDEX] != OPEN) return -1;	// Check that the USCI is available

	confUCA1(commID);
	
	// Copy over pointer and length
	uca1TxPtr = data;
	uca1TxSize = len-1;
	// Start of TX
	usciStat[UCA1_INDEX] = TX;
	UCA1TXBUF = *uca1TxPtr;

	return 1;
}
/**************************************************************************//**
 * \brief	Receive method for USCI A1 SPI operation
 *
 * This method performs a synchronous read by storing the bytes to be read in
 * spiA1RxSize and then performing len dummy write to the bus to fetch the data
 * from a slave device. The RX size is cleared on this function call.
 *
 * \param	len	The number of bytes to be read from the bus
 * \param	commID	Communication ID number of the application
 *
 * \retval	-1	USCI A1 Module Busy
 * \retval	1	Receive successfully started
 *
 * \sideeffect		Reset the UCA1 RX size and data pointer
 ******************************************************************************/
int spiA1Read(unsigned int len, unsigned int commID)
{
	if(usciStat[UCA1_INDEX] != OPEN) return -1;	// Check that the USCI is available
	
	confUCA1(commID);
	
	// Clear RX Size and copy length
	uca1RxSize = 0;					// Reset RX size
	uca1RxPtr = dev[commID]->rxPtr;			// Reset RX pointer
	spiA1RxSize = len-1;
	// Start of RX
	usciStat[UCA1_INDEX] = RX;
	UCA1TXBUF = 0xFF;				// Start TX
	return 1;
}

/**************************************************************************//**
 * \brief	Byte Swap method for USCI A1 SPI operation
 *
 * This blocking method allows the user to transmit a byte and receive the
 * response simultaneously clocked back in. TX and RX buffer sizes/content
 * are unaffected.
 *
 * \param	byte	The byte to be sent via SPI
 * \param 	commID	Communication ID number of the application
 *
 * \return	The byte shifted in from the SPI
 *************************************************************************/
unsigned char spiA1Swap(unsigned char byte, unsigned int commID)
{
	if(usciStat[UCA1_INDEX] != OPEN) return -1;	// Check that the USCI is available

	confUCA1(commID);
	
	usciStat[UCA1_INDEX] = SWAP;		// Set USCI status to swap (prevent other operations)
	UCA1TXBUF = byte;
	while(UCA1STAT & UCBUSY);			// Wait for TX complete
	usciStat[UCA1_INDEX] = OPEN;		// Set USCI status to open (swap complete)
	return UCA1RXBUF;					// Return RX contents
}
#endif //USE_UCA1_SPI

/**********************************************************************//**
 * \brief	USCI A1 RX/TX Interrupt Service Routine
 *
 * This ISR manages all TX/RX proceedures with the exception of transfer
 * initialization. Once a transfer (read or write) is underway, this method
 * assures the correct amount of bytes are written to the correct location.
 *************************************************************************/
#pragma vector=USCI_A1_VECTOR
__interrupt void usciA1Isr(void)
{
	unsigned int dummy = 0xFF;
	// Transmit Interrupt Flag Set
	if(UCA1IFG & UCTXIFG){
#ifdef USE_UCA1_SPI
		if(usciStat[UCA1_INDEX] == TX) {
#endif
		if(uca1TxSize > 0){
			UCA1TXBUF = *(++uca1TxPtr);		// Transmit the next outgoing byte
			uca1TxSize--;
		}
		else{
			UCA1IFG &= ~UCTXIFG;			// Clear TX interrupt flag from vector on end of TX
			usciStat[UCA1_INDEX] = OPEN; 		// Set status open if done with transmit
		}
#ifdef USE_UCA1_SPI
		}
#endif
	}

	// Receive Interrupt Flag Set
	if(UCA1IFG & UCRXIFG){
#ifdef USE_UCA1_SPI
		if(usciStat[UCA1_INDEX] == RX){			// Check we are in RX mode for SPI
#endif // USE_UCA1_SPI
		if(UCA1STAT & UCRXERR) dummy = UCA1RXBUF;	// RX ERROR: Do a dummy read to clear interrupt flag
		else {						// Otherwise write the value to the RX pointer
			*(uca1RxPtr++) = UCA1RXBUF;
			uca1RxSize++;				// RX Size decrement in read function
#ifdef USE_UCA1_SPI
			if(uca1RxSize < spiA1RxSize) UCA1TXBUF = dummy; // Perform another dummy write
			else
#endif
			usciStat[UCA1_INDEX] = OPEN;
		}
#ifdef USE_UCA1_SPI
		}
#endif
	}
	UCA1IFG &= ~UCRXIFG;					// Clear RX interrupt flag from vector on end of RX
}
#endif // USE_UCA1

/****************************************************************
 * USCI B0 Variable Declarations
 ***************************************************************/
#ifdef USE_UCB0
unsigned char *ucb0TxPtr;			///< USCI B0 TX Data Pointer
unsigned char *ucb0RxPtr;			///< USCI B0 RX Data Pointer
unsigned int ucb0TxSize = 0;		///< USCI B0 TX Size
unsigned int ucb0RxSize = 0;		///< USCI B0 RX Size
unsigned int ucb0ToRxSize = 0;		///< USCI B0 to-RX Size
unsigned char i2cb0RegAddr = 0;		///< Register address storage for I2C operation

/**************************************************************
 * General Purpose USCI B0 Functions
 *************************************************************/
/**************************************************************************//**
 * \brief	Configures USCI B0 for operation
 *
 * Write control registers and clear system variables for the
 * USCI B0 module, which can be used as either a UART or SPI.
 *
 * NOTE: This config function is already called before any read/write function call
 * and therefore should (in almost all cases) never be called by the user.
 *
 * \param	commID	The communication ID for the registered app
 ******************************************************************************/
void confUCB0(unsigned int commID)
{
	unsigned int status;
	if(devConf[UCB0_INDEX] == commID) return;	// Check if device is already configured
	enter_critical(status);				// Perform config in critical section
	UCB0CTL1 |= UCSWRST;				// Assert USCI software reset
	UCB0_IO_CLEAR();				// Clear I/O for configuration

	// Configure key control words
	UCB0CTLW0 = dev[commID]->usciCtlW0 | UCSWRST;
#ifdef UCB0CTLW1	// Check for UCB0CTLW1 defined
	UCB0CTLW1 = dev[commID]->usciCtlW1;
#endif //UCB0CTLW1
	UCB0BRW = dev[commID]->baudDiv;
	ucb0RxPtr = dev[commID]->rxPtr;

	// Clear buffer sizes
	ucb0RxSize = 0;
	ucb0TxSize = 0;
	ucb0ToRxSize = 0;

#ifdef USE_UCB0_I2C
	UCB0I2CSA = (dev[commID]->rAddr) & ADDR_MASK;	// Set up the slave address
#endif //USE_UCB0_I2C

	UCB0_IO_CONF(dev[commID]->rAddr & ADDR_MASK);	// Port set up
	UCB0CTL1 &= ~UCSWRST;				// Resume operation
	UCB0IFG = 0;						// Clear any previously existing interrupt flags
	UCB0IE |= UCRXIE + UCTXIE;			// Enable Interrupts	
#ifdef USE_UCB0_I2C
	UCB0IE |= UCNACKIE;					// Set up slave NACK interrupt
#endif

	devConf[UCB0_INDEX] = commID;			// Store config
	exit_critical(status);				// End critical section
}
/**************************************************************************//**
 * \brief	Resets USCI B0 without writing over control regs
 *
 * This function is included to soft-reset the USCI B0 module
 * management variables without clearing the current config.
 *
 * \param	commID	The comm ID of the registered app
 * \sideeffect	Sets the RX pointer to that registered w/ commID
 ******************************************************************************/
void resetUCB0(unsigned int commID){
	ucb0RxPtr = dev[commID]->rxPtr;
	ucb0RxSize = 0;
	ucb0TxSize = 0;
	ucb0ToRxSize = 0;
	usciStat[UCB0_INDEX] = OPEN;
	return;
}
/**************************************************************************//**
 * \brief	Get method for USCI B0 RX buffer size
 *
 * Returns the number of bytes which have been written
 * to the RX pointer (since the last read performed).
 *
 * \return	The number of valid bytes following the rxPtr.
 ******************************************************************************/
unsigned int getUCB0RxSize(void){
	return ucb0RxSize;
}
/**************************************************************************//**
 * \brief	Get method for USCI B0 status
 *
 * Returns the status of the USCI module, either OPEN, TX, or RX
 *
 * \retval	0	Indicates the OPEN status
 * \retval 	1	Indicates the TX Status
 * \retval 	2	Indicates the RX Status
 ******************************************************************************/
unsigned char getUCB0Stat(void){
	return usciStat[UCB0_INDEX];
}

/*************************************************************************//**
 * \brief	Set method for USCI B0 Baud Rate Divisor
 *
 * Sets the baud rate divisor of the USCI module, this divisor is generally
 * performed relative to the SMCLK rate of the system.
 *
 * \param 	baudDiv	The new divisor to apply
 * \param	commID	The communications ID number of the application
 *****************************************************************************/
void setUCB0Baud(unsigned int baudDiv, unsigned int commID){
	dev[commID]->baudDiv = baudDiv;		// Replace the baud divisor in memory
	devConf[UCB0_INDEX] = 0;		// Reset the device config storage (config will be performed on next read/write)
	return;
}

/***********************************************************
 * UCB0 SPI HANDLERS
 ***********************************************************/
#ifdef USE_UCB0_SPI
/**************************************************************************//**
 * \brief	Transmit method for USCI B0 SPI operation
 *
 * This method initializes a transmission of len bytes from the base of the
 * *data pointer. The transmission uses the USCI B0 TX ISR, so 2 sequential calls 
 * may result in partial transmission.
 *
 * \param	*data	Pointer to data to be written
 * \param	len		Length (in bytes) of data to be written
 * \param 	commID	Communication ID number of application
 *
 * \retval	-1		USCI B0 Module busy
 * \retval	1		Transmit successfully started
 *******************************************************************************/
int spiB0Write(unsigned char *data, unsigned int len, unsigned int commID)
{
	if(usciStat[UCB0_INDEX] != OPEN) return -1;	// Check that the USCI is available

	confUCB0(commID);
	
	// Copy over pointer and length
	ucb0TxPtr = data;
	ucb0TxSize = len-1;
	// Start of TX
	usciStat[UCB0_INDEX] = TX;
	UCB0TXBUF = *ucb0TxPtr;

	return 1;
}
/**************************************************************************//**
 * \brief	Receive method for USCI B0 SPI operation
 *
 * This method performs a synchronous read by storing the bytes to be read in
 * ucbB0ToRxSize and then performing len dummy write to the bus to fetch the data
 * from a slave device. The RX size is cleared on this function call.
 *
 * \param	len	The number of bytes to be read from the bus
 * \param	commID	Communication ID number of the application
 *
 * \retval	-1	USCI B0 Module Busy
 * \retval	1	Receive successfully started
 *
 * \sideeffect	Reset the UCB0 RX size and data pointer
 ******************************************************************************/
int spiB0Read(unsigned int len, unsigned int commID)
{
	if(usciStat[UCB0_INDEX] != OPEN) return -1;	// Check that the USCI is available

	confUCB0(commID);

	// Clear RX Size and copy length
	ucb0RxSize = 0;							// Reset the rx size
	ucb0RxPtr = dev[commID]->rxPtr;			// Reset the rx pointer
	ucb0ToRxSize = len;
	// Start of RX
	usciStat[UCB0_INDEX] = RX;
	UCB0TXBUF = 0xFF;				// Start TX
	
	return 1;
}

/**************************************************************************//**
 * \brief	Byte Swap method for USCI B0 SPI operation
 *
 * This blocking method allows the user to transmit a byte and receive the
 * response simultaneously clocked back in. TX and RX buffer sizes/content
 * are unaffected.
 *
 * \param	byte	The byte to be sent via SPI
 * \param 	commID	Communication ID number of the application
 *
 * \return	The byte shifted in from the SPI
 *************************************************************************/
unsigned char spiB0Swap(unsigned char byte, unsigned int commID)
{
	if(usciStat[UCB0_INDEX] != OPEN) return -1;	// Check that the USCI is available
	
	confUCB0(commID);
	
	usciStat[UCB0_INDEX] = SWAP;		// Set USCI status to swap (prevent other operations)
	UCB0TXBUF = byte;
	while(UCB0STAT & UCBUSY);			// Wait for TX complete
	usciStat[UCB0_INDEX] = OPEN;		// Set USCI status to open (swap complete)
	return UCB0RXBUF;					// Return RX contents
}
#endif //USE_UCB0_SPI
/***********************************************************
 * UCB0 I2C HANDLERS
 * \todo: NOT YET TESTED!!!!!!!!!
 ***********************************************************/
#ifdef USE_UCB0_I2C
/**************************************************************************//**
 * \brief	Transmit method for USCI B0 I2C operation
 *
 * This method initializes a transmission of len bytes from the base of the
 * *data pointer. Similarly to spiB0Write(), the transmission uses the USCI B0
 * TX ISR to complete, so 2 sequential calls may result in partial transmission.
 *
 * \param	*data	Pointer to data to be written
 * \param	len	Length (in bytes) of data to be written
 * \param 	commID	Communication ID number of application
 *
 * \retval	-1	USCI B0 Module busy
 * \retval	1	Transmit successfully started
 *******************************************************************************/
int i2cB0Write(i2cPacket packet)
{
	if(usciStat[UCB0_INDEX] != OPEN) return -1; 	// Check that the USCI is available

	confUCB0(packet.commID);

	i2cb0RegAddr = packet.regAddr;
	ucb0TxPtr = packet.data;
	ucb0TxSize = packet.len;
	// Start of TX
	usciStat[UCB0_INDEX] = TX;
	UCB0CTL1 |= UCTR + UCTXSTT;	// Generate start condition

	return 1;
}
/**************************************************************************//**
 * \brief	Receive method for USCI B0 I2C operation
 *
 * This method performs a synchronous read by storing the bytes to be read in
 * ucbB0ToRxSize and then performing len dummy write to the bus to fetch the data
 * from a slave device. The RX size is cleared on this function call.
 *
 * \param	len	The number of bytes to be read from the bus
 * \param	commID	Communication ID number of the application
 *
 * \retval	-1	USCI B0 Module Busy
 * \retval	1	Receive successfully started
 *
 * \sideeffect	Reset the UCB0 RX size and data pointer
 ******************************************************************************/
int i2cB0Read(i2cPacket packet)
{
	unsigned char ier = UCB0IE;					// Save the interrupt enable register value
	unsigned int timeout = 0;					// Timeout counter

	if(usciStat[UCB0_INDEX] != OPEN) return -1;	// Check that the USCI is available

	confUCB0(packet.commID);

	i2cb0RegAddr = packet.regAddr;
	ucb0RxSize = 0;
	ucb0ToRxSize = packet.len;
	// Start of RX
	usciStat[UCB0_INDEX] = RX;

	UCB0IE = 0x00;								// Clear the interrupt enables
	UCB0CTL1 |= UCTR + UCTXSTT;					// Generate start condition
	UCB0TXBUF = packet.regAddr;					// Write the desired start register to the chip
	//while(UCB0CTL1 & UCTXSTT);				// Wait for stop condition to be lowered
	for(timeout = 0; UCB0CTL1 & UCTXSTT; timeout++){
		if (timeout > MAX_STT_WAIT) {
			timeout = -1;
			break;
		}
	}
	UCB0IFG &= ~UCTXIFG;						// Clear the TX interrupt flag
	UCB0CTL1 &= ~(UCTR + UCTXSTT);				// Clear the transmit and start control bits
	UCB0IE = ier;								// Restore the interrupt enable register
	if(timeout == -1)
		return -1;
	UCB0CTL1 |= UCTXSTT;						// Set the start command, initializing read
	return 0;
}
/**************************************************************************//**
 * \brief	Ping (slave present) Method for USCI B0 I2C Operation
 *
 * This method tests for the presence of a slave at the address affiliated with
 * the registered commID. This is accomplished by sending a start, then stop
 * condition sequenitally on the bus, then reading the UCB0STAT register for
 * a NACK condition.
 *
 * \param	commID	Communication ID number of the application
 *
 * \retval	-1	USCI B0 Module Busy
 * \retval	0	Slave not present
 * \retval	1	Slave present
 *
 * \sideeffect	The USCI module will need to be reconfigured for the next
 * 		operation (even if it uses the same slave address or commID)
 ******************************************************************************/
unsigned char i2cB0SlavePresent(unsigned int commID)
{
	unsigned char retval;

	if(usciStat[UCB0_INDEX] != OPEN) return -1;	// Check that USCI is available

	UCB0IE = 0;									// Clear NACK, RX, and TX interrupt conditions
	UCB0I2CSA = dev[commID]->raddr & ADDR_MASK;	// Set slave address
	UCB0CTL1 |= UCTR + UCTXSTT + UCTXSTP;		// TX w/ start and stop condition

	while(UCB0CTL1 & UCTXSTP);					// Wait for stop condition
	retval = !(UCB0STAT & UCNACKIFG);

	devConf[UCB0_INDEX] = 0;					// Clear device config slot for UCB0 (reconfigure next use)
	return retval;
}
#endif //USE_UCB0_I2C
/**********************************************************************//**
 * \brief	USCI B0 RX/TX Interrupt Service Routine
 *
 * This ISR manages all TX/RX proceedures with the exception of transfer
 * initialization. Once a transfer (read or write) is underway, this method
 * assures the correct amount of bytes are written to the correct location.
 *************************************************************************/
#pragma vector=USCI_B0_VECTOR
__interrupt void usciB0Isr(void)
{
#ifdef USE_UCB0_SPI
	unsigned int dummy = 0xFF;
	// Transmit Interrupt Flag Set
	if(UCB0IFG & UCTXIFG){
		if(usciStat[UCB0_INDEX] == TX) {
			if(ucb0TxSize > 0){
				UCB0TXBUF = *(++ucb0TxPtr);	// Transmit the next outgoing byte
				ucb0TxSize--;
			}
			else{
				usciStat[UCB0_INDEX] = OPEN; 	// Set status open if done with transmit
				UCB0IFG &= ~UCTXIFG;		// Clear TX interrupt flag from vector on end of TX
			}
		}
		else UCB0IFG &= ~UCTXIFG;
	}

	// Receive Interrupt Flag Set
	if(UCB0IFG & UCRXIFG){	// Check for interrupt flag and RX mode
		if(usciStat[UCB0_INDEX] == RX){				// Check we are in RX mode for SPI
			if(UCB0STAT & UCRXERR) dummy = UCB0RXBUF;	// RX ERROR: Do a dummy read to clear interrupt flag
			else {	// Otherwise write the value to the RX pointer
				*(ucb0RxPtr++) = UCB0RXBUF;
				ucb0RxSize++;	// RX Size decrement in read function
				if(ucb0RxSize < ucb0ToRxSize) UCB0TXBUF = dummy; // Perform another dummy write
				else usciStat[UCB0_INDEX] = OPEN;
			}
		}
		else UCB0IFG &= ~UCRXIFG; // Clear RX interrupt flag from vector on end of RX
	}
#endif // USE_UCB0_SPI
#ifdef USE_UCB0_I2C
	switch(__even_in_range(UCB0IV, 12))
	{
		case I2CIV_NO_INT: break;					// Vector 0 (no interrupt)
		case I2CIV_AL_INT: break;					// Arbitration lost IFG
		case I2CIV_NACK_INT: 						// NACK Flag
			UCB0CTL1 |= UCTXSTP;					// Send stop bit
			UCB0STAT &= ~UCNACKIFG; 				// Clear NACK flag
			usciStat[UCB0_INDEX] = OPEN;			// Set module to open status
			break;
		case I2CIV_STT_INT: break;					// Start flag
		case I2CIV_STP_INT: break;  				// Stop flag
		case I2CIV_RX_INT:							// RX flag
			if(usciStat[UCB0_INDEX] == RX){			// Check we are performing an RX
				*(ucb0RxPtr++) = UCB0RXBUF;			// Read character_UCB0_I2C
				if(ucb0RxSize == ucb0ToRxSize){		// Is this the final RX?
					UCB0CTL1 |= UCTXSTP;			// Send a stop bit
					usciStat[UCB0_INDEX] = OPEN; 	// Set the resource to open
				}
				else {
					ucb0RxSize++;
				}
			}
			else UCB0IFG &= ~UCRXIFG;
			break;
		case I2CIV_TX_INT:							// TX flag
			if(usciStat[UCB0_INDEX] == TX){
				if(i2cb0RegAddr != 0){				// Are we writing the first byte (register address)?
					UCB0TXBUF = i2cb0RegAddr;		// Write the register address
					i2cb0RegAddr = 0;				// Zero the register address to indicate transferred
				}
				else if(ucb0TxSize > 0){			// Normal data transfer
					UCB0TXBUF = *(++ucb0TxPtr);		// Write the next character
					ucb0TxSize--;					// Decrement the transmit count
				}
				else {								// This is the final TX
					UCB0CTL1 |= UCTXSTP;			// Send stop bit
					UCB0IFG &= ~UCTXIFG;			// Clear TX flag
					usciStat[UCB0_INDEX] = OPEN;
				}
			}
			else UCB0IFG &= ~UCTXIFG;
			break;
		default: break;
	}
#endif // USE_UCB0_I2C
}
#endif // USE_UCB0

/****************************************************************
 * USCI B1 Variable Declarations
 ***************************************************************/
#ifdef USE_UCB1
unsigned char *ucb1TxPtr;			///< USCI B1 TX Data Pointer
unsigned char *ucb1RxPtr;			///< USCI B1 RX Data Pointer
unsigned int ucb1TxSize = 0;		///< USCI B1 TX Size
unsigned int ucb1RxSize = 0;		///< USCI B1 RX Size
unsigned int ucb1ToRxSize = 0;		///< USCI B1 to-RX Size
unsigned int i2cb1RegAddr = 0;		///< Register address storage for I2C operation

/**************************************************************
 * General Purpose USCI B1 Functions
 *************************************************************/
/**************************************************************************//**
 * \brief	Configures USCI B1 for operation
 *
 * Write control registers and clear system variables for the
 * USCI B0 module, which can be used as either a UART or SPI.
 *
 * NOTE: This config function is already called before any read/write function call
 * and therefore should (in almost all cases) never be called by the user.
 *
 * \param	commID	The communication ID for the registered app
 ******************************************************************************/
void confUCB1(unsigned int commID)
{
	unsigned int status;
	if(devConf[UCB1_INDEX] == commID) return;	// Check if device is already configured
	enter_critical(status);				// Perform config in critical section
	UCB1CTL1 |= UCSWRST;				// Assert USCI software reset
	//UCB1_IO_CLEAR();					// Clear I/O for configuration

	// Configure key control words
	UCB1CTLW0 = dev[commID]->usciCtlW0 | UCSWRST;
#ifdef UCB1CTLW1	// Check for UCB1CTLW1 defined
	UCB1CTLW1 = dev[commID]->usciCtlW1;
#endif //UCB1CTLW1
	UCB1BRW = dev[commID]->baudDiv;
	ucb1RxPtr = dev[commID]->rxPtr;

	// Clear buffer sizes
	ucb1RxSize = 0;
	ucb1TxSize = 0;
	ucb1ToRxSize = 0;

#ifdef USE_UCB1_I2C
	UCB1I2CSA = (dev[commID]->rAddr) & ADDR_MASK;	// Set up the slave address
#endif //USE_UCB1_I2C

	UCB1_IO_CONF(dev[commID]->rAddr & ADDR_MASK);	// Port set up
	UCB1CTL1 &= ~UCSWRST;				// Resume operation
	UCB1IFG = 0;						// Clear any previously existing interrupt flags
	UCB1IE |= UCRXIE + UCTXIE;			// Enable Interrupts
#ifdef USE_UCB1_I2C
	UCB1IE |= UCNACKIE;					// Set up slave NACK interrupt
#endif

	devConf[UCB1_INDEX] = commID;			// Store config
	exit_critical(status);				// End critical section
}
/**************************************************************************//**
 * \brief	Resets USCI B1 without writing over control regs
 *
 * This function is included to soft-reset the USCI B1 module
 * management variables without clearing the current config.
 *
 * \param	commID	The comm ID of the registered app
 * \sideeffect	Sets the RX pointer to that registered w/ commID
 ******************************************************************************/
void resetUCB1(unsigned int commID){
	ucb1RxPtr = dev[commID]->rxPtr;
	ucb1RxSize = 0;
	ucb1TxSize = 0;
	ucb1ToRxSize = 0;
	usciStat[UCB1_INDEX] = OPEN;
	return;
}
/**************************************************************************//**
 * \brief	Get method for USCI B1 RX buffer size
 *
 * Returns the number of bytes which have been written
 * to the RX pointer (since the last read performed).
 *
 * \return			The number of valid bytes following the rxPtr.
 ******************************************************************************/
unsigned int getUCB1RxSize(void){
	return ucb1RxSize;
}
/**************************************************************************//**
 * \brief	Get method for USCI B1 status
 *
 * Returns the status of the USCI module, either OPEN, TX, or RX
 *
 * \retval	0	Indicates the OPEN status
 * \retval 	1	Indicates the TX Status
 * \retval 	2	Indicates the RX Status
 ******************************************************************************/
unsigned char getUCB1Stat(void){
	return usciStat[UCB1_INDEX];
}

/*************************************************************************//**
 * \brief	Set method for USCI B1 Baud Rate Divisor
 *
 * Sets the baud rate divisor of the USCI module, this divisor is generally
 * performed relative to the SMCLK rate of the system.
 *
 * \param 	baudDiv	The new divisor to apply
 * \param	commID	The communications ID number of the application
 *****************************************************************************/
void setUCB1Baud(unsigned int baudDiv, unsigned int commID){
	dev[commID]->baudDiv = baudDiv;		// Replace the baud divisor in memory
	devConf[UCB1_INDEX] = 0;		// Reset the device config storage (config will be performed on next read/write)
	return;
}

/***********************************************************
 * UCB0 SPI HANDLERS
 ***********************************************************/
#ifdef USE_UCB1_SPI
/**************************************************************************//**
 * \brief	Transmit method for USCI B1 SPI operation
 *
 * This method initializes a transmission of len bytes from the base of the
 * *data pointer. Similarly to uartB0Write(), the transmission uses the USCI B1
 * TX ISR to complete, so 2 sequential calls may result in partial transmission.
 *
 * \param	*data	Pointer to data to be written
 * \param	len		Length (in bytes) of data to be written
 * \param 	commID	Communication ID number of application
 *
 * \retval	-1		USCI B1 Module busy
 * \retval	1		Transmit successfully started
 *******************************************************************************/
int spiB1Write(unsigned char *data, unsigned int len, unsigned int commID)
{
	if(usciStat[UCB1_INDEX] != OPEN) return -1;	// Check that the USCI is available

	confUCB1(commID);
	
	// Copy over pointer and length
	ucb1TxPtr = data;
	ucb1TxSize = len-1;
	// Start of TX
	usciStat[UCB1_INDEX] = TX;
	UCB1TXBUF = *ucb1TxPtr;

	return 1;
}
/**************************************************************************//**
 * \brief	Receive method for USCI B1 SPI operation
 *
 * This method performs a synchronous read by storing the bytes to be read in
 * ucb1ToRxSize and then performing len dummy write to the bus to fetch the data
 * from a slave device. The RX size is cleared on this function call.
 *
 * \param	len		The number of bytes to be read from the bus
 * \param	commID	Communication ID number of the application
 *
 * \retval	-1		USCI B1 Module Busy
 * \retval	1		Receive successfully started
 * \sideeffect		Reset the UCB0 RX size and data pointer
 ******************************************************************************/
int spiB1Read(unsigned int len, unsigned int commID)
{
	if(usciStat[UCB1_INDEX] != OPEN) return -1;	// Check that the USCI is available

	confUCB1(commID);

	// Clear RX Size and copy length
	ucb1RxPtr = dev[commID]->rxPtr;			// Reset the rx pointer
	ucb1RxSize = 0;					// Reset the rx size
	ucb1ToRxSize = len-1;
	// Start of RX
	usciStat[UCB1_INDEX] = RX;
	UCB1TXBUF = 0xFF;				// Start TX
	
	return 1;
}

/**************************************************************************//**
 * \brief	Byte Swap method for USCI B1 SPI operation
 *
 * This blocking method allows the user to transmit a byte and receive the
 * response simultaneously clocked back in. TX and RX buffer sizes/content
 * are unaffected.
 *
 * \param	byte	The byte to be sent via SPI
 * \param 	commID	Communication ID number of the application
 *
 * \return	The byte shifted in from the SPI
 *************************************************************************/
unsigned char spiB1Swap(unsigned char byte, unsigned int commID)
{
	if(usciStat[UCB1_INDEX] != OPEN) return -1;	// Check that the USCI is available
	
	confUCB1(commID);
	
	usciStat[UCB1_INDEX] = SWAP;		// Set status to swap (prevent other operations)
	UCB1TXBUF = byte;
	while(UCB1STAT & UCBUSY);			// Wait for TX complete
	usciStat[UCB1_INDEX] = OPEN;		// Set status to open (swap complete)
	return UCB1RXBUF;					// Return RX contents
}
#endif //USE_UCB1_SPI
/***********************************************************
 * UCB1 I2C HANDLERS
 ***********************************************************/
#ifdef USE_UCB1_I2C
/**************************************************************************//**
 * \brief	Transmit method for USCI B1 I2C operation
 *
 * This method initializes a transmission of len bytes from the base of the
 * *data pointer. Similarly to spiB1Write(), the transmission uses the USCI B1
 * TX ISR to complete, so 2 sequential calls may result in partial transmission.
 *
 * \param	*data	Pointer to data to be written
 * \param	len	Length (in bytes) of data to be written
 * \param 	commID	Communication ID number of application
 *
 * \retval	-1	USCI B1 Module busy
 * \retval	1	Transmit successfully started
 *******************************************************************************/
int i2cB1Write(i2cPacket* packet)
{
	if(usciStat[UCB1_INDEX] != OPEN) return -1; 	// Check that the USCI is available

	confUCB1(packet->commID);

	i2cb1RegAddr = packet->regAddr;
	ucb1TxPtr = packet->data;
	ucb1TxSize = packet->len;

	// Start of TX
	usciStat[UCB1_INDEX] = TX;
	UCB1CTL1 |= UCTR + UCTXSTT;						// Generate start condition

	return 1;
}
/**************************************************************************//**
 * \brief	Receive method for USCI B1 I2C operation
 *
 * This method performs a synchronous read by storing the bytes to be read in
 * ucbB1ToRxSize and then performing len dummy write to the bus to fetch the data
 * from a slave device. The RX size is cleared on this function call.
 *
 * \param	len	The number of bytes to be read from the bus
 * \param	commID	Communication ID number of the application
 *
 * \retval	-1	USCI B1 Module Busy
 * \retval	1	Receive successfully started
 *
 * \sideeffect	Reset the UCB1 RX size and data pointer
 ******************************************************************************/
int i2cB1Read(i2cPacket* packet)
{
	if(usciStat[UCB1_INDEX] != OPEN) return -1;	// Check that the USCI is available

	confUCB1(packet->commID);

	i2cb1RegAddr = packet->regAddr;
	ucb1RxSize = 0;
	ucb1ToRxSize = packet->len;
	ucb1RxPtr = packet->data;					// THIS IS OPTIONAL (DUNNO IF WE WANT TO REDIRECT THIS WRITEBACK)

	// Start of RX
	usciStat[UCB1_INDEX] = RX;

	UCB1CTL1 |= UCTR + UCTXSTT;					// Generate start condition
	devConf[UCB1_INDEX] = 0;					// Clear config

	return 0;
}
/**************************************************************************//**
 * \brief	Ping (slave present) Method for USCI B1 I2C Operation
 *
 * This method tests for the presence of a slave at the address affiliated with
 * the registered commID. This is accomplished by sending a start, then stop
 * condition sequentially on the bus, then reading the UCB1STAT register for
 * a NACK condition.
 *
 * \param	commID	Communication ID number of the application
 *
 * \retval	-1	USCI B1 Module Busy
 * \retval	0	Slave not present
 * \retval	1	Slave present
 *
 * \sideeffect	The USCI module will need to be reconfigured for the next
 * 		operation (even if it uses the same slave address or commID)
 ******************************************************************************/
int i2cB1SlavePresent(unsigned int commID)
{
	unsigned char retval;

	if(usciStat[UCB1_INDEX] != OPEN) return -1;	// Check that USCI is available

	UCB1IE = 0;									// Clear NACK, RX, and TX interrupt conditions
	UCB1I2CSA = dev[commID]->rAddr & ADDR_MASK;	// Set slave address
	UCB1CTL1 |= UCTR + UCTXSTT + UCTXSTP;		// TX w/ start and stop condition

	while(UCB1CTL1 & UCTXSTP);					// Wait for stop condition
	retval = !(UCB1STAT & UCNACKIFG);

	devConf[UCB1_INDEX] = 0;					// Clear device config slot for UCB0 (reconfigure next use)
	return retval;
}
#endif //USE_UCB0_I2C
/**********************************************************************//**
 * \brief	USCI B1 RX/TX Interrupt Service Routine
 *
 * This ISR manages all TX/RX proceedures with the exception of transfer
 * initialization. Once a transfer (read or write) is underway, this method
 * assures the correct amount of bytes are written to the correct location.
 *************************************************************************/
#pragma vector=USCI_B1_VECTOR
__interrupt void usciB1Isr(void)
{
	unsigned int dummy = 0xFF;
#ifdef USE_UCB1_SPI
	// Transmit Interrupt Flag Set
	if(UCB1IFG & UCTXIFG){
		if(usciStat[UCB1_INDEX] == TX) {
			if(ucb1TxSize > 0){
				UCB1TXBUF = *(++ucb1TxPtr);	// Transmit the next outgoing byte
				ucb1TxSize--;
			}
			else{
				usciStat[UCB1_INDEX] = OPEN; 	// Set status open if done with transmit
				UCB1IFG &= ~UCTXIFG;		// Clear TX interrupt flag from vector on end of TX
			}
		}
	}

	// Receive Interrupt Flag Set
	if(UCB1IFG & UCRXIFG){	// Check for interrupt flag and RX mode
		if(usciStat[UCB1_INDEX] == RX)	{			// Check we are in RX mode for SPI
			if(UCB1STAT & UCRXERR) dummy = UCB1RXBUF;	// RX ERROR: Do a dummy read to clear interrupt flag
			else {	// Otherwise write the value to the RX pointer
			*(ucb1RxPtr++) = UCB1RXBUF;
			ucb1RxSize++;	// RX Size decrement in read function
			if(ucb1RxSize < ucb1ToRxSize) UCB1TXBUF = dummy;	// Perform another dummy write
			else
			usciStat[UCB1_INDEX] = OPEN;
			}
		}
	}
	UCB1IFG &= ~UCRXIFG;	// Clear RX interrupt flag from vector on end of RX
#endif // USE_UCB1_SPI
#ifdef USE_UCB1_I2C
	switch(__even_in_range(UCB1IV, 12))
	{
		case I2CIV_NO_INT: break;					// Vector 0 (no interrupt)
		case I2CIV_AL_INT: break;					// Arbitration lost IFG
		case I2CIV_NACK_INT:						// NACK Flag
			UCB1CTL1 |= UCTXSTP;					// Send stop bit
			UCB1STAT &= ~UCNACKIFG; 				// Clear NACK flag
			usciStat[UCB1_INDEX] = OPEN;
			break;
		case I2CIV_STT_INT: break;					// Start flag
		case I2CIV_STP_INT:	break;					// Stop flag
		case I2CIV_RX_INT:							// RX flag
			if(usciStat[UCB1_INDEX] == RX){			// Check we are performing an RX
				*(ucb1RxPtr++) = UCB1RXBUF;			// Read character
				ucb1RxSize++;
				if(ucb1RxSize == ucb1ToRxSize){	// Is this the final RX?
					dummy = UCB1RXBUF;				// Perform a dummy read
					UCB1CTL1 |= UCTXSTP;			// Send a stop bit
					usciStat[UCB1_INDEX] = OPEN; 	// Set the resource to open
				}
			}
			else UCB1IFG &= ~UCRXIFG;
			break;
		case I2CIV_TX_INT:							// TX flag
			if(usciStat[UCB1_INDEX] == TX){
				if(i2cb1RegAddr != 0){				// Are we writing the first byte (register address)?
					UCB1TXBUF = i2cb1RegAddr;		// Write the register address
					i2cb1RegAddr = 0;				// Zero the register address to indicate transferred
				}
				else if(ucb1TxSize > 0){			// Normal data transfer
					UCB1TXBUF = *(ucb1TxPtr++);		// Write the next character
					ucb1TxSize--;					// Decrement the transmit count
				}
				else {								// This is the final TX
					UCB1CTL1 |= UCTXSTP;			// Send stop bit
					UCB1IFG &= ~UCTXIFG;			// Clear TX flag
					usciStat[UCB1_INDEX] = OPEN;
				}
			}
			else if(usciStat[UCB1_INDEX] == RX){
				if(i2cb1RegAddr != 0){				// Are we writing the first byte (register address)?
					UCB1TXBUF = i2cb1RegAddr;		// Write the register address
					i2cb1RegAddr = 0;				// Zero the register address to indicate transferred
				}
				else {
					UCB1CTL1 &= ~UCTR;				// Clear the transmit and start control bits
					UCB1CTL1 |= UCTXSTT;			// Set the start command, initializing read
					//UCB1IFG &= ~UCTXIFG;			// Clear the TX interrupt flag
					//UCB1IFG |= UCRXIFG;
				}
			}
			else UCB1IFG &= ~UCTXIFG;
			break;
		default: break;
	}
#endif // USE_UCB1_I2C
}
#endif // USE_UCB1
