/****************************************************************//**
 * \file comm.h
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
#ifndef COMM_H_
#define COMM_H_
#include "util.h"			// Includes bitwise access structure and macros (used in CS logic)
#include "comm_hal_5342.h"
#include "clocks.h"

#define MAX_DEVS	16		///< Maximum number of devices to be registered

// USCI Library Conditional Compilation Macros
// NOTE: Only define at most 1 config for each USCI module, otherwise a Multiple Serial Endpoint error will be created on compilation
#define USE_UCA0_UART			///< USCI A0 UART Mode Conditional Compilation Flag
//#define USE_UCA0_SPI			///< USCI A0 SPI Mode Conditional Compilation Flag
//#define USE_UCA1_UART			///< USCI A1 UART Mode Conditional Compilation Flag
//#define USE_UCA1_SPI			///< USCI A1 SPI Mode Conditional Compilation Flag
//#define USE_UCB0_SPI			///< USCI B0 SPI Mode Conditional Compilation Flag
//#define USE_UCB0_I2C			///< USCI B0 I2C Mode Conditional Compilation Flag
//#define USE_UCB1_SPI			///< USCI B1 SPI Mode Conditional Compilation Flag
#define USE_UCB1_I2C			///< USCI B1 I2C Mode Conditional Compilation Flag

typedef struct uconf			/// USCI Configuration Data Structure
{
	unsigned int rAddr;			///< 16-Bit Resource Code [ USCI # (2 bits) ] [  USCI mode (2 bits) ] [ CS or I2C Address (12 bits) ]
	unsigned int usciCtlW0;		///< 16-Bit USCI Control Word0 (see TI User Guide)
	unsigned int usciCtlW1;		///< 16-Bit USCI Control Word1 (see TI User Guide)
	unsigned int baudDiv;		///< Sourced clock rate divisor (can use FREQ_2_BAUDDIV(x) macro included below)
	unsigned char *rxPtr;		///< Data write back pointer
} usciConfig;

typedef struct i2cDataPacket	/// Packet for I2C transmit/receive
{
	unsigned int commID;		///< Communications ID for the transfer
	unsigned char regAddr;		///< Internal chip register address for write/read
	unsigned char len;			///< Length of the desired transfer (in bytes)
	unsigned char* data;		///< Data pointer to write from/read to
}i2cPacket;

// USCI Status Codes
typedef enum usciStatusCode		/// Enumerated type for USCI status
{
	 OPEN	= 0,				///< Module open and available for transfer
	 TX		= 1,				///< Module currently completing a transmission
	 RX		= 2,				///< Module currently completing a receive
	 SWAP	= 3					///< Module currently compelting a byte swap (SPI)
} usciStatus;

/*********************************************************
 * Resource address control codes
 ********************************************************/
// Resource Address Masking
#define	USCI_MASK		0xC000	///< USCI device name (UCXX) mask for resource address code
#define MODE_MASK		0x3000	///< USCI mode (UART/SPI/I2C) name mask for resource address code
#define	UMODE_MASK		0xF000	///< USCI name/mode mask for resource address code
#define	ADDR_MASK		0x03FF	///< USCI address mask for resource address code (max 10 bits)
// Resource Codes
#define UCA0_RCODE		0x0000	///< USCI A0 resource code
#define	UCA1_RCODE		0x4000	///< USCI A1 resource code
#define UCB0_RCODE		0x8000	///< USCI B0 resource code
#define UCB1_RCODE		0xC000	///< USCI B1 resource code
#define UART_MODE		0x1000	///< UART mode code
#define SPI_MODE		0x2000	///< SPI mode code
#define I2C_MODE		0x3000	///< I2C mode code
// Resource and Mode combo codes
#define UCA0_UART		UCA0_RCODE + UART_MODE	///< Combined UCA0 UART Mode Resource Code
#define UCA0_SPI		UCA0_RCODE + SPI_MODE	///< Combined UCA0 SPI Mode Resource Code
#define UCA1_UART		UCA1_RCODE + UART_MODE	///< Combined UCA1 UART Mode Resource Code
#define UCA1_SPI		UCA1_RCODE + SPI_MODE	///< Combined UCA1 SPI Mode Resource Code
#define UCB0_SPI		UCB0_RCODE + SPI_MODE	///< Combined UCB0 SPI Mode Resource Code
#define UCB0_I2C		UCB0_RCODE + I2C_MODE	///< Combined UCB0 I2C Mode Resource Code
#define UCB1_SPI		UCB1_RCODE + SPI_MODE	///< Combined UCB1 SPI Mode Resource Code
#define UCB1_I2C		UCB1_RCODE + I2C_MODE	///< Combined UCB1 I2C Mode Resource Code

/**********************************************************
 * USCI Register Values
 **********************************************************/
// USCI CTL Word 0 Defaults
// UART MODE
#define UART_8N1		UCSSEL__SMCLK														///< UCTLW0: 8 bit UART (no parity, 1 stop bit) w/ baud from SMCLK
#define	UART_7N1		(UC7BIT << 8) + UCSSEL__SMCLK										///< UCTLW0: 7 bit UART (no parity, 1 stop bit) w/ baud from SMCLK
// SPI MODE
#define SPI_8M0_LE		((UCSYNC + UCMST) << 8) + UCSSEL__SMCLK								///< UCTLW0: 8 bit Mode 0 SPI Master LSB first w/ baud from SMCLK
#define SPI_8M0_BE		((UCSYNC + UCMST + UCMSB) << 8) + UCSSEL__SMCLK						///< UCTLW0: 8 bit Mode 0 SPI Master MSB first w/ baud from SMCLK
#define SPI_8M1_LE		((UCSYNC + UCMST + UCCKPH) << 8) + UCSSEL__SMCLK					///< UCTLW0: 8 bit Mode 1 SPI Master LSB first w/ baud from SMCLK
#define SPI_8M1_BE		((UCSYNC + UCMST + UCCKPH + UCMSB) << 8) + UCSSEL__SMCLK			///< UCTLW0: 8 bit Mode 1 SPI Master MSB first w/ baud from SMCLK
#define SPI_8M2_LE		((UCSYNC + UCMST + UCCKPL) << 8) + UCSSEL__SMCLK					///< UCTLW0: 8 bit Mode 2 SPI Master LSB first w/ baud from SMCLK
#define SPI_8M2_BE		((UCSYNC + UCMST + UCCKPL + UCMSB) << 8) + UCSSEL__SMCLK			///< UCTLw0: 8 bit Mode 2 SPI Master MSB first w/ baud from SMCLK
#define SPI_8M3_LE		((UCSYNC + UCMST + UCCKPH + UCCKPL) << 8) + UCSSEL__SMCLK			///< UCTLW0: 8 Bit Mode 3 SPI Master LSB first w/ baud from SMCLK
#define SPI_8M3_BE		((UCSYNC + UCMST + UCCKPH + UCCKPL + UCMSB) << 8) + UCSSEL__SMCLK	///< UCTLW0: 8 bit Mode 3 SPI Master MSB first w/ baud from SMCLK
#define	SPI_S8M0_LE		UCSYNC << 8															///< UCTLW0: 8 bit Mode 0 SPI Slave LSB first
#define SPI_S8M0_BE		((UCSYNC + UCMSB) << 8 ) + UCSSEL__SMCLK							///< UCTLW0: 8 bit Mode 0 SPI Slave MSB first
#define SPI_S8M1_LE		((UCSYNC + UCCKPH) << 8) + UCSSEL__SMCLK							///< UCTLW0: 8 bit Mode 1 SPI Slave LSB first
#define SPI_S8M1_BE		((UCSYNC + UCCKPH + UCMSB) << 8) + UCSSEL__SMCLK					///< UCTLW0: 8 bit Mode 1 SPI Slave MSB first
#define SPI_S8M2_LE		((UCSYNC + UCCKPL) << 8) + UCSSEL__SMCLK							///< UCTLW0: 8 bit Mode 2 SPI Slave LSB first
#define SPI_28M2_BE		((UCSYNC + UCCKPL + UCMSB) << 8) + UCSSEL__SMCLK					///< UCTLw0: 8 bit Mode 2 SPI Slave MSB first
#define SPI_S8M3_LE		((UCSYNC + UCCKPH + UCCKPL) << 8) + UCSSEL__SMCLK					///< UCTLW0: 8 bit Mode 3 SPI Slave LSB first
#define SPI_S8M3_BE		((UCSYNC + UCCKPH + UCCKPL + UCMSB) << 8 ) + UCSSEL__SMCLK			///< UCTLW0: 8 bit Mode 3 SPI Slave MSB first
// I2C MODE
// THESE MACROS NEED UPDATING/TESTING
#define I2C_10SM		((UCA10 + UCSLA10 + UCMST + UCMODE_3 + UCSYNC) << 8) + UCSSEL__SMCLK 		///< UCTLW0: 10 bit addressed I2C (master and slave), single master mode, transmitter w/ baud from SMCLK
#define I2C_7SM			((UCMST + UCMODE_3 + UCSYNC) << 8) + UCSSEL__SMCLK				///< UCTLW0: 7 bit addressed I2C (master and slave), single master mode, receiver w/ baud from SMCLK

// I2C Interrupt Vector Definitions
#define I2CIV_NO_INT		0			///< I2C no interrupt source
#define I2CIV_AL_INT		2			///< I2C arbitration lost interrupt flag
#define I2CIV_NACK_INT		4			///< I2C NACK interrupt flag
#define I2CIV_STT_INT		6			///< I2C start condition flag
#define I2CIV_STP_INT		8			///< I2C stop condition flag
#define I2CIV_RX_INT		10			///< I2C receive interrupt flag
#define I2CIV_TX_INT		12			///< I2C transmit interrupt flag

// USCI CTL Work 1 Defaults
#define DEF_CTLW1		0x0003			///< CTLW1: 200ns deglitch time
// USCI Baud Rate Defaults
#define UCLK_FREQ		SMCLK_FREQ		///< USCI Clock Rate [use SMCLK to source our UART (from timing.h)]
#define UBR_DIV(x)		UCLK_FREQ/x		///< Baud rate frequency to divisor macro (uses timing.h)

// Resource config buffer index
#define UCA0_INDEX		0				///< USCI A0 shared buffer index
#define UCA1_INDEX 		1				///< USCI A1 shared buffer index
#define UCB0_INDEX		2				///< USCI B0 shared buffer index
#define UCB1_INDEX		3				///< USCI B1 shared buffer index

// Read/Write Routine Return Codes
#define USCI_CONF_ERROR		-2			///< USCI configuration error return code
#define	USCI_BUSY_ERROR		-1			///< USCI busy error return code
#define	USCI_SUCCESS		1			///< TX/RX success return code

// Time to start condition (in cycles) macro
#define MAX_STT_WAIT 		10000		///< Maximum time to start condition wait period

// App. registration function prototype
int registerComm(usciConfig *conf);
/*************************************************************************
 * UCA0 Macro Logic
 ************************************************************************/
// Basic function prototypes
void confUCA0(unsigned int commID);
void resetUCA0(unsigned int commID);
unsigned int getUCA0RxSize(void);
unsigned char getUCA0Stat(void);
void setUCA0Baud(unsigned int baudDiv, unsigned int commID);
/************************* UCA0 UART MODE ********************************/
#ifdef USE_UCA0_UART
// Function prototypes
int uartA0Write(unsigned char* data, unsigned int len, unsigned int commID);
int uartA0Read(unsigned int len, unsigned int commID);
// Other useful macros
#define USE_UCA0	///< UCA0 Active Definition
// Multiple endpoint config detection
#ifdef USE_UCA0_SPI
#error Multiple Serial Endpoint Configuration on USCI A0
#endif // USE_UCA0_UART and USE_UCA0_SPI
#endif
/************************* UCA0 SPI MODE ********************************/
#ifdef USE_UCA0_SPI
// Function prototypes
int spiA0Write(unsigned char* data, unsigned int len, unsigned int commID);
int spiA0Read(unsigned int len, unsigned int commID);
unsigned char spiA0Swap(unsigned char byte, unsigned int commID);
// Multiple Endpoint Config Compiler Error
#define USE_UCA0	///< USCI A0 Active Definition
#ifdef USE_UCA0_UART
#error Multiple Serial Endpoint Configuration on USCI A0
#endif // USE_UCA0_UART AND USE_UCA0_SPI
#endif // USE_UCA0_SPI

/**************************************************************************
 * UCA1 Macro Logic
 *************************************************************************/
// Basic function prototypes
void confUCA1(unsigned int commID);
void resetUCA1(unsigned int commID);
unsigned int getUCA1RxSize(void);
unsigned char getUCA1Stat(void);
void setUCA1Baud(unsigned int baudDiv, unsigned int commID);
/************************* UCA1 UART MODE ********************************/
#ifdef USE_UCA1_UART
// Function prototypes
int uartA1Write(unsigned char* data, unsigned int len, unsigned int commID);
int uartA1Read(unsigned int len, unsigned int commID);
// Other useful macros
#define USE_UCA1	///< USCI A1 Active Definition
// Multiple endpoint config detection
#ifdef USE_UCA1_SPI
#error Multiple Serial Endpoint Configuration on USCI A1
#endif // USE_UCA1_UART and USE_UCA1_SPI
#endif // USE_UCA1_UART
/*************************** UCA1 SPI MODE *******************************/
#ifdef USE_UCA1_SPI
// Function prototypes
int spiA1Write(unsigned char* data, unsigned int len, unsigned int commID);
int spiA1Read(unsigned int len, unsigned int commID);
unsigned char spiA1Swap(unsigned char byte, unsigned int commID);
// Other useful macros
#define USE_UCA1	///< USCI A1 Active Definition
// Multiple endpoint config detection
#ifdef USE_UCA1_UART
#error Multiple Serial Endpoint Configuration on USCI A1
#endif // USE_UCA1_UART and USE_UCA1_SPI
#endif // USE_UCA1_SPI

/**************************************************************************
 * UCB0 Macro Logic
 *************************************************************************/
// Basic Function Prototypes
void confUCB0(unsigned int commID);
void resetUCB0(unsigned int commID);
unsigned int getUCB0RxSize(void);
unsigned char getUCB0Stat(void);
void setUCB0Baud(unsigned int baudDiv, unsigned int commID);
/************************* UCB0 SPI MODE *********************************/
#ifdef USE_UCB0_SPI
// Function prototypes
int spiB0Write(unsigned char* data, unsigned int len, unsigned int commID);
int spiB0Read(unsigned int len, unsigned int commID);
unsigned char spiB0Swap(unsigned char byte, unsigned int commID);
// Other useful macros
#define USE_UCB0	///< USCI B0 Active Definition
// Multiple endpoint config detection
#ifdef USE_UCB0_I2C
#error Mulitple Serial Endpoint Configuration on USCI B0
#endif // USE_UCB0_SPI and USE_UCB0_I2C
#endif // USE_UCB0_SPI
/************************* UCB0 I2C MODE *********************************/
#ifdef USE_UCB0_I2C
// Function prototypes
int i2cB0Write(i2cPacket packet);
int i2cB0Read(i2cPacket packet);
int i2cB0SlavePresent(unsigned int commID);
// Other useful macros
#define USE_UCB0	///< USCI B0 Active Definition
/// Muleiple endpoint config detection
#ifdef USE_UCB0_SPI
#error Multiple Serial Endpoint Configuration on USCI B0
#endif // USE_UCB0_SPI and USE_UCB0_I2C
#endif // USE_UCB0_I2C

/**************************************************************************
 * UCB1 Macro Logic
 *************************************************************************/
// Basic Function Prototypes
void confUCB1(unsigned int commID);
void resetUCB1(unsigned int commID);
unsigned int getUCB1RxSize(void);
unsigned char getUCB1Stat(void);
void setUCB1Baud(unsigned int baudDiv, unsigned int commID);
/************************* UCB1 SPI MODE *********************************/
#ifdef USE_UCB1_SPI
// Function prototypes
int spiB1Write(unsigned char* data, unsigned int len, unsigned int commID);
int spiB1Read(unsigned int len, unsigned int commID);
unsigned char spiB1Swap(unsigned char byte, unsigned int commID);
// Other useful macros
#define USE_UCB1	///< USCI B1 Active Definition
// Multiple endpoint config detection
#ifdef USE_UCB1_I2C
#error Mulitple Serial Endpoint Configuration on USCI B1
#endif // USE_UCB1_SPI and USE_UCB1_I2C
#endif // USE_UCB1_SPI
/************************* UCB0 I2C MODE *********************************/
#ifdef USE_UCB1_I2C
// Function prototypes
int i2cB1Write(i2cPacket* packet);
int i2cB1Read(i2cPacket* packet);
int i2cB1SlavePresent(unsigned int commID);
// Other useful macros
#define USE_UCB1	///< USCI B1 Active Definition
/// Multiple endpoint config detection
#ifdef USE_UCB1_SPI
#error Multiple Serial Endpoint Configuration on USCI B1
#endif // USE_UCB1_SPI and USE_UCB1_I2C
#endif // USE_UCB1_I2C
#endif /* COMM_H_ */
