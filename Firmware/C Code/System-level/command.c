/*
 * command.c
 *
 *  Created on: Dec 16, 2013
 *      Author: bb3jd
 */
#include "command.h"
#include "system.h"
#include "hal.h"
#include "ftdi.h"
#include "rtc.h"
#include "flash.h"
#include "mpu.h"

unsigned char handshakeFlag = 0;		///< Handshake received flag
unsigned char processingCmd = 0;		///< Processing command flag

int runCommand(cmdPkt* cmd)
{
	cmdReply reply = {0};
	unsigned int payloadChecksum;
	unsigned char payload[MAX_PAYLOAD_SIZE];
	ftdiPacket outPacket;
	unsigned char startofReply = 'R';

	reply.echoCmd = cmd->command;		// Copy incoming command into the echo field in reply

	// Validate checksum for command
	if(fletcherChecksum((unsigned char *)cmd, CMD_CRC_LEN, 0) != cmd->checksum){
		reply.response = CMD_CORRUPT_REQUEST;
		reply.len = 0;
	}
	// Check for handshake flag or incoming handshake command
	else if((handshakeFlag == 0) && (cmd->command != CMD_HANDSHAKE)){
		reply.response = CMD_NEED_HANDSHAKE;
		reply.len = 0;
	}
	else{
		// Process the command
		switch(cmd->command){
			case CMD_HANDSHAKE:						// Handshake for initialization of the command interface
				handshake(cmd, &reply, payload);
				break;
			case CMD_GET_STATUS:					// Status polling
				//getStatus(cmd, &reply, payload);
				break;
			case CMD_SET_TIME:						// Set system time in RTC request
				sysSetTime(cmd, &reply, payload);
				break;
			case CMD_GET_TIME:						// Get system time from RTC request
				sysGetTime(cmd,  &reply, payload);
				break;
			case CMD_GET_VER:						// Get the system firmware version
				sysGetVersion(cmd, &reply, payload);
				break;
			case CMD_GET_NID:						// Get the system node ID
				sysGetNodeID(cmd, &reply, payload);
				break;
			case CMD_GET_CID:						// Get the flash card ID
				sysGetCardID(cmd, &reply, payload);
				break;
			case CMD_GET_SECTOR:					// Fetch a sector from the flash card
				sysGetSector(cmd, &reply, payload);
				break;
			case CMD_GET_ACCEL:						// Get the current accelerometer value triplet
				sysGetAccel(cmd, &reply, payload);
				break;
			case CMD_GET_GYRO:						// Get the current gyro value triplet
				sysGetGyro(cmd, &reply, payload);
				break;
			case CMD_SET_SR:						// Set the system sampling rate
				sysSetSamplingRate(cmd, &reply, payload);
				break;
			case CMD_SET_DCE:
				sysSetDCE(cmd, &reply, payload);
				break;
				// TODO: Add commands here...
			case CMD_LED_ON:						// Dummy command: turn on the LED
				LED1_CONFIG();
				LED1_ON();
				reply.response = CMD_SUCCESS;
				reply.len = 0;
				break;
			case CMD_LED_OFF:						// Dummy command: turn off the LED
				LED1_CONFIG();
				LED1_OFF();
				reply.response = CMD_SUCCESS;
				reply.len = 0;
				break;
			default:
				reply.response = CMD_INVALID_COMMAND;
				reply.len = 0;
				break;
		}

	}

	if(reply.len > 0){								// If reply payload is populated
		payloadChecksum = fletcherChecksum(payload, reply.len, 0);	// Perform checksum
		reply.len += sizeof(payloadChecksum);						// Pack the checksum length into the reply header
	}
	reply.checksum = fletcherChecksum((unsigned char *)&reply, sizeof(reply)-sizeof(reply.checksum), 0);	// Compute reply header chekcsum

	// Send the start of reply
	outPacket.data = &startofReply;
	outPacket.len = 1;
	ftdiWrite(outPacket);

	// Send the reply header
	outPacket.data = (unsigned char *)&reply;
	outPacket.len = sizeof(reply);
	ftdiWrite(outPacket);

	// Send the reply payload (if one exists)
	if(reply.len > 0){
		outPacket.data = payload;
		outPacket.len = reply.len - sizeof(payloadChecksum);
		ftdiWrite(outPacket);
		// Send the reply payload checksum at the end of the payload
		outPacket.data = (unsigned char *)(&payloadChecksum);
		outPacket.len = sizeof(payloadChecksum);
		ftdiWrite(outPacket);
	}

	processingCmd = 0;	// Clear processing command flag (for incoming parser management)
	return CMD_SUCCESS;
}

/**************************************************************************//**
 * \brief "Handshake" with the node to unlock access to the other commands
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void handshake(cmdPkt* cmd, cmdReply* reply, unsigned char* payload){
	handshakeFlag = 1;
	reply->len = 0;
	reply->response = CMD_SUCCESS;
}

/**************************************************************************//**
 * \brief Set the system RTC time to a specific value
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysGetTime(cmdPkt* cmd, cmdReply* reply, unsigned char* payload){
	time* t = rtcGetTime();
	memcpy((void*)payload, (void*)t, sizeof(time));
	reply->len = sizeof(time);
	reply->response = CMD_SUCCESS;
}

/**************************************************************************//**
 * \brief Get the system RTC time and return it in the payload
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysSetTime(cmdPkt* cmd, cmdReply* reply, unsigned char* payload){
	time* t = (time *)(cmd->arg);
	rtcSetTime(t);
	reply->len = 0;
	reply->response = CMD_SUCCESS;
}

/**************************************************************************//**
 * \brief Get the system firmware version
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysGetVersion(cmdPkt* cmd, cmdReply* reply, unsigned char* payload) {
	extern Version v;
	memcpy(payload, (unsigned char *)&v, sizeof(Version));
	reply->len = sizeof(Version);
	reply->response = CMD_SUCCESS;
}

/**************************************************************************//**
 * \brief Get the node identification number
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysGetNodeID(cmdPkt* cmd, cmdReply* reply, unsigned char* payload) {
	extern unsigned int nodeID; // From main.c
	memcpy(payload, (unsigned char *)&nodeID, sizeof(nodeID));
	reply->len = sizeof(nodeID);
	reply->response = CMD_SUCCESS;
}

/**************************************************************************//**
 * \brief Get the card identification number
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysGetCardID(cmdPkt* cmd, cmdReply* reply, unsigned char* payload) {
	flashReadCardID(payload);
	reply->len = CARD_ID_LEN;
	reply->response = CMD_SUCCESS;
}

/**************************************************************************//**
 * \brief Get the most recent accelerometer values
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysGetAccel(cmdPkt* cmd, cmdReply* reply, unsigned char* payload){
	axisData accel = mpuGetAccel();
	memcpy(payload, (unsigned char *)&accel, sizeof(axisData));
	reply->len = sizeof(axisData);
	reply->response = CMD_SUCCESS;
}

/**************************************************************************//**
 * \brief Get the most recent gyro values
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysGetGyro(cmdPkt* cmd, cmdReply* reply, unsigned char* payload){
	axisData accel = mpuGetGyro();
	memcpy(payload, (unsigned char *)&accel, sizeof(axisData));
	reply->len = sizeof(axisData);
	reply->response = CMD_SUCCESS;
}

/**************************************************************************//**
 * \brief Fetch the desired sector from the card and return it
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysGetSector(cmdPkt* cmd, cmdReply* reply, unsigned char* payload){
	unsigned long index = (unsigned long)(cmd->arg);
	unsigned long len = (unsigned long)(&cmd->arg[4]);
	struct Sector sect;

	if(secureFlashRead(index, &sect) == FLASH_TIMEOUT){	// Attempt a flash read
		reply->len = 0;									// On timeout set length to 0
		reply->response = CMD_FAIL_GENERAL;				// and provide general failure
	}
	else{												// On successful read
		memcpy(payload, (unsigned char *)&sect, sizeof(struct Sector));	// Copy over the payload
		reply->len = sizeof(struct Sector);				// Set the response length
		reply->response = CMD_SUCCESS;					// Set the response code (SUCCESS)
	}
}

/**************************************************************************//**
 * \brief Set the system sampling rate to the desired value
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysSetSamplingRate(cmdPkt* cmd, cmdReply* reply, unsigned char* payload) {
	unsigned int reqSR = (unsigned int)cmd->arg;
	unsigned int setSR = 0;
	setSR = mpuSetSampRate(reqSR);		// Call the MPU set sampling rate routine
	memcpy(payload, (unsigned char *)&setSR, sizeof(unsigned int));	// Copy over set rate
	reply->len = sizeof(unsigned int);	// Set the reply length
	reply->response = CMD_SUCCESS;		// Set the command code (SUCCESS)
}

/**************************************************************************//**
 * \brief Set the system sampling rate to the desired value
 *
 * \param[in]	cmd		Pointer to the command passed into the run function
 * \param[out]	reply	Pointer to the reply packet to be sent in response
 * \param[out]  payload Pointer to the data payload to be sent with the reply
 ******************************************************************************/
void sysSetDCE(cmdPkt* cmd, cmdReply* reply, unsigned char* payload){
	extern unsigned char dataCollectionEn;	// From system.c

	dataCollectionEn = (unsigned int)cmd->arg;
	reply->len = 0;
	reply->response = CMD_SUCCESS;
}
