/*
 * command.h
 *
 *  Created on: Dec 16, 2013
 *      Author: bb3jd
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#define MAX_PAYLOAD_SIZE	516
#define CMD_MASK			0xFF00

// TEMPO 4 Command Set
#define 	CMD_HANDSHAKE	'H'			///< Handshake command for initializing communications
#define		CMD_GET_STATUS	'?'			///< Poll for status
#define 	CMD_GET_TIME	'T'			///< Poll for node time (RTC-based)
#define		CMD_SET_TIME	'C'			///< Set the node time (RTC-based)
#define		CMD_GET_VER		'V'			///< Get firmware version number command
#define 	CMD_GET_NID		'N'			///< Get the node ID
#define 	CMD_GET_CID		'*'			///< Get the flash card
#define		CMD_SET_DCE		'$'			///< Enable data collection
#define 	CMD_SET_DCD		'W'			///< Disable data collection
#define		CMD_GET_SECTOR	'S'			///< Get data sector
#define 	CMD_GET_BLOCK	'B'			///< Get data block (set of sectors)
#define		CMD_GET_ACCEL	'A'			///< Get the most recent accelerometer value triplet
#define 	CMD_GET_GYRO	'G'			///< Get the most recent gyro value triplet
#define		CMD_SET_SR		'R'			///< Set the node sampling rate
#define		CMD_CARD_INIT	'I'			///< Flash card initialization command
#define		CMD_CARD_OVWT	'O'			///< Flash card overwrite command
#define		CMD_LED_ON		'1'			///< Turn the LED on
#define		CMD_LED_OFF		'0'			///< Turn the LED off

// Other Command Related Values
#define 	CMD_ARG_LEN			16		///< Command argument length
#define 	CMD_LEN				2 + CMD_ARG_LEN + 2 	///< Command header (2B) plus arg (16B) plus CRC (2B)
#define 	CMD_CRC_LEN			CMD_ARG_LEN + 2			///< Command arg (16B) plus CRC (2B)
#define 	CMD_START_OF_SEQ	'S'
#define		CMD_START_OF_REPLY	'R'

typedef struct commandPacket {			/// TEMPO 4 Incoming Command Structure
	unsigned int command;				///< The command to be processed
	unsigned char arg[CMD_ARG_LEN];		///< The argument to the command
	unsigned int checksum;				///< Checksum for validity purposes
} cmdPkt;

// Command Response Codes
#define CMD_CORRUPT_REQUEST		1		///< The command received did not pass checksum
#define CMD_NEED_HANDSHAKE		2		///< No handshake has been passed to this node
#define	CMD_INVALID_COMMAND		3		///< No such command exists in the current command set
#define CMD_FAIL_READ_ONLY		4		///< The device is in read-only mode
#define CMD_FAIL_GENERAL		5		///< General failure case (catch all)
#define CMD_BAD_ARG				6		///< Invalid argument failure
#define CMD_SUCCESS				0		///< Command processed successfully

typedef struct replyHeader {			/// TEMPO 4 Outgoing Response Structure
	unsigned int echoCmd;				///< Command echo field
	unsigned int response;				///< Command response code field
	unsigned int len;					///< Command response length field
	unsigned int checksum;				///< Command response checksum field
} cmdReply;

// Function Prototypes
int runCommand(cmdPkt *cmd);
/**************************************************************************//**
 * \brief "Handshake" with the node to unlock access to the other commands
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void handshake(cmdPkt* cmd, cmdReply* reply, unsigned char* payload);
/**************************************************************************//**
 * \brief Set the system RTC time to a specific value
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysGetTime(cmdPkt* cmd, cmdReply* reply, unsigned char* payload);
/**************************************************************************//**
 * \brief Get the system RTC time and return it in the payload
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysSetTime(cmdPkt* cmd, cmdReply* reply, unsigned char* payload);
/**************************************************************************//**
 * \brief Get the system firmware version
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysGetVersion(cmdPkt* cmd, cmdReply* reply, unsigned char* payload);
/**************************************************************************//**
 * \brief Get the node identification number
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysGetNodeID(cmdPkt* cmd, cmdReply* reply, unsigned char* payload);
/**************************************************************************//**
 * \brief Get the card identification number
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysGetCardID(cmdPkt* cmd, cmdReply* reply, unsigned char* payload);
/**************************************************************************//**
 * \brief Get the most recent accelerometer values
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysGetAccel(cmdPkt* cmd, cmdReply* reply, unsigned char* payload);
/**************************************************************************//**
 * \brief Get the most recent gyro values
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysGetGyro(cmdPkt* cmd, cmdReply* reply, unsigned char* payload);
/**************************************************************************//**
 * \brief Fetch the desired sector from the card and return it
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysGetSector(cmdPkt* cmd, cmdReply* reply, unsigned char* payload);
/**************************************************************************//**
 * \brief Set the system sampling rate to the desired value
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysSetSamplingRate(cmdPkt* cmd, cmdReply* reply, unsigned char* payload);

/**************************************************************************//**
 * \brief Set the system sampling rate to the desired value
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysSetDCE(cmdPkt* cmd, cmdReply* reply, unsigned char* payload);

#endif /* COMMAND_H_ */
