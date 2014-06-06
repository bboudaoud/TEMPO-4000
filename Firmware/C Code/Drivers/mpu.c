/****************************************************************//**
 * \file MPU6000.c
 *
 * \author 	Bill Devine, Ben Boudaoud
 * \date 	August 2013
 *
 * \brief 	This library provides base-level functionality for the
 * 			MPU6050 series IMU from Invensense
 *
 * \note 	This I2C-based driver runs on top of the TEMPO 4000
 * 			communications library and makes all its hardware calls
 * 			to the USCI module through this interface.
 *******************************************************************/

#include <msp430f5342.h>
#include "mpu.h"
#include "comm.h"
#include "hal.h"
#include "clocks.h"

static unsigned int mpuID = 0;				// Storage for mpu comm ID
static unsigned char rxdat[512] = {0};		// Buffer for MPU recieves

static usciConfig mpuConf = {(UCB1_I2C + MPU_CHIP_ADDR), I2C_7SM, DEF_CTLW1, UBR_DIV(MPU_BAUD), rxdat};
//TODO: add a default status here
const mpuInfo mpuDefault = { 0 };
static mpuInfo mpuData = {
		0x07,			// Sampling rate divisor 	(1024/(7+1) = 128Hz)
		0x01,			// Config DLPF 				(BW = 180Hz)
		FSR_2000dps,	// Gyro config 				(1kHz, 2000deg/s)
		FSR_8G,			// Accel config 			(1kHz, 8G)
		0x00,			// Motion threshold 		(0 - not used)
		0x00,			// FIFO enable 				(0 - not used)
		0x00,			// I2C Master Control		(0 - not used)
		0x30,			// Int Pin Config			(Int active high, push-pull, latched)
		DATA_RDY_INT,	// Int Enable				(Data ready interrupts)
		delay_4ms,		// Motion detection			(0 - not used)
		0x00,			// User control				(Reset signal conditioning)
		0x00,			// Power control 1			(Device awake)
		0x00			// Power control 2 			(All sensors awake)
};

/**************************************************************************//**
 * \fn 		int mpuRegWrite(unsigned char regAddr, unsigned char toWrite)
 * \brief	This function writes a single byte to a register in the MPU6050
 *
 * This function attempts to write a single byte into a provided register
 * address on the MPU6050 over I2C. Once the transmission has been started it
 * waits until the USCI module is listed as #OPEN before returning, so multiple
 * calls can be safely written in line in user code
 *
 * \param	regAddr		The internal register address of the desired register
 * 						(see #MPU6000.h for register map defines)
 * \param	toWrite		The 8-bit character to write to the register
 *
 * \retval	1 			Success
 * \retval	-1 			USCI module busy
 *****************************************************************************/
int mpuRegWrite(unsigned char regAddr, unsigned char toWrite)
{
	i2cPacket packet;					// Packet for i2c transmission
	int ret = 0;						// Return value

	packet.commID = mpuID;				// Set up the commID for the packet transfer
	packet.regAddr = regAddr;			// Set up the internal chip register address
	packet.len = 1;						// Set the packet length to a single byte
	packet.data = &toWrite;				// Pack the single byte to write into the packet

	ret = i2cB1Write(&packet);			// Write the packet to USCI B1
	while(getUCB1Stat() != OPEN);		// Wait for the resource to open before return

	return ret;
}

/**************************************************************************//**
 * \fn 		unsigned char mpuRegRead(unsigned char regAddr)
 * \brief	This function reads a single byte from a register in the MPU6050
 *
 * This function attempts to read a single byte from the MPU6050 over I2C. Once
 * the transmission has been started it waits until the USCI module is listed as
 * #OPEN before returning, so multiple calls can be safely written in line in user
 * code
 *
 * \param	regAddr		The internal register address of the desired register
 * 						(see #MPU6000.h for register map defines)
 *
 * \returns	The character read from the address requested
 * \retval	0xFF 	If the module is busy
 *****************************************************************************/
unsigned char mpuRegRead(unsigned char regAddr)
{
	i2cPacket packet;					// Packet for i2c reception
	unsigned char ret = 0;

	packet.commID = mpuID;				// Set up the commID for the packet transfer
	packet.regAddr = regAddr;			// Set up the internal chip register address
	packet.len = 1;						// Set the packet length to a single byte
	packet.data = &ret;					// Aim the read pointer at the rx buffer

	if(i2cB1Read(&packet) == -1)			// Attempt to read the packet from USCI B1
		return 0xFF;
	while(getUCB1Stat() != OPEN);		// Wait for the resource to open before return

	return ret;							// Get the received character from RX buffer and return it
}

/**************************************************************************//**
 * \fn 		int mpuInit(void)
 * \brief	This function registers the I2C port for communication and sets up
 * 			the MPU6050
 *
 * This function attempts to register the communication with the MPU6050 as an
 * I2C on USCI B1. It sets up the I/O, resets the device, disables sleep then
 * polls the MPU6050 for its WHOAMI value. If the register address matches the
 * expected value, it continues to initialize the registers in the MPU to their
 * default RAM copy values.
 *
 * \retval	-1		WHOAMI check failed (the MPU is not fully initialized)
 * \retval	1		MPU successfully initialized
 *****************************************************************************/
int mpuInit(void)
{
	mpuID = registerComm(&mpuConf);		// Set up comm handler for the MPU6050 on I2C B1
	MPU_IO_CONFIG();						// Set up the digital I/O for MPU6050

	mpuReset();							// Reset the device
	mpuSleepEn(False);					// Disable the sleep state

	if(mpuWhoAmI() != WHOAMI_VAL) 		// Assure device has correct address
			return 0;

	return mpuSetup(&mpuData);			// Run the i2c setup routine
}

/**************************************************************************//**
 * \fn 		int mpuSetup(mpuInfo* info)
 * \brief	This function writes a RAM copy of MPU settings into the MPU6050
 *
 * This function writes almost all registers of interest on the MPU6050 to a
 * set of values provided by the user in an #mpuInfo structure. Once a value
 * has been successfully written into the device its RAM buffer copy is updated.
 *
 * \param	mpuInfo* info	Pointer to a RAM copy of the MPU configuration
 *
 * \retval	-1		Sample Rate Divisor Config Fail
 * \retval	-2		DLPF Config Fail
 * \retval	-3		Gyro Config Fail
 * \retval	-4		Accel Config Fail
 * \retval	-5		Motion Threshold Config Fail
 * \retval	-6		FIFO Config Fail
 * \retval	-7		Master I2C Config Fail
 * \retval	-8		Interrupt Pin Config Fail
 * \retval	-9		Interrupt Enable Config Fail
 * \retval	-10		Motion Control Config Fail
 * \retval	-11		User Control Config Fail
 * \retval	-12		Power Management 1 Config Fail
 * \retval	-13		Power Management 2 Config Fail
 *****************************************************************************/
int mpuSetup(mpuInfo* info)
{
	if(mpuRegWrite(PWR_MGMT_1, info->pwr_mgmt_1) == 1){			// Configure power management 1
		mpuData.pwr_mgmt_1 = info->pwr_mgmt_1;
	}
	else return PWR_MGMT_1;

	if(mpuRegWrite(PWR_MGMT_2, info->pwr_mgmt_2) == 1){			// Configure power management 2
		mpuData.pwr_mgmt_2 = info->pwr_mgmt_2;
	}
	else return PWR_MGMT_2;


	if(mpuRegWrite(USER_CTRL, info->user_ctrl) == 1){			// Configure user control
		mpuData.user_ctrl = info->user_ctrl;
	}
	else return USER_CTRL;

	if(mpuRegWrite(SMPLRT_DIV, info->smplrt_div) == 1){			// Configure the sampling rate divisor [ SR = gyro rate/(1+smplrt_div) ]
		mpuData.smplrt_div = info->smplrt_div;
	}
	else return SMPLRT_DIV;

	if(mpuRegWrite(CONFIG, info->config) == 1){					// Configure digital low-pass filter and ext sync settings
		mpuData.config = info->config;
	}
	else return CONFIG;

	if(mpuRegWrite(GYRO_CONFIG, info->gyro_config) == 1){		// Configure gyro self test and range select
		mpuData.gyro_config = info->gyro_config;
	}
	else return GYRO_CONFIG;

	if(mpuRegWrite(ACCEL_CONFIG, info->accel_config) == 1){		// Configure accel self test and range select
		mpuData.accel_config = info->accel_config;
	}
	else return ACCEL_CONFIG;

	if(mpuRegWrite(MOT_THR, info->mot_thr) == 1){				// Set the motion threshold for interrupt	 (should this be elsewhere?)
		mpuData.mot_thr = info->mot_thr;
	}
	else return MOT_THR;

	if(mpuRegWrite(FIFO_EN_REG, info->fifo_en) == 1){			// Configure the FIFO
		mpuData.fifo_en = info->fifo_en;
	}
	else return FIFO_EN_REG;

	if(mpuRegWrite(I2C_MST_CTRL, info->i2c_mst_ctrl) == 1){		// Configure the on-chip master I2C interface
		mpuData.i2c_mst_ctrl = info->i2c_mst_ctrl;
	}
	else return I2C_MST_CTRL;

	if(mpuRegWrite(INT_PIN_CFG, info->int_pin_cfg) == 1){		// Configure the interrupt pin
		mpuData.int_pin_cfg = info->int_pin_cfg;
	}
	else return INT_PIN_CFG;

	if(mpuRegWrite(INT_ENABLE, info->int_enable) == 1){			// Configure interrupt enable
		mpuData.int_enable = info->int_enable;
	}
	else return INT_ENABLE;

	if(mpuRegWrite(MOT_DETECT_CTRL, info->mot_detect_ctrl) == 1){// Configure motion detection control
		mpuData.mot_detect_ctrl = info->mot_detect_ctrl;
	}
	else return MOT_DETECT_CTRL;

	return 1;
}

/**************************************************************************//**
 * \fn 		inline void mpuReset(void)
 * \brief	This function resets the MPU6050
 *
 * This function attempts to reset the MPU6050 by writing the reset bit in the
 * power management 1 register. This should clear all registers to their default state.
 *
 * TODO: Do we want to add clear for RAM copy here???????
 *****************************************************************************/
inline void mpuReset(void)
{
	mpuRegWrite(PWR_MGMT_1, DEVICE_RESET);			// Write a reset to the device
	mpuData.pwr_mgmt_1 &= ~DEVICE_RESET;			// Assure device reset bit isn't set in local copy
	delay_ms(MPU_RESET_DELAY_MS);					// Wait for reset
}

/**************************************************************************//**
 * \fn 		unsigned char mpuSleepEn(unsigned char en)
 * \brief	This function sets or clears the MPU6050 sleep mode
 *
 * This function attempts to set or clear the sleep enable bit inside the MPU6050
 * power mangement 1 register.
 *
 * \param	unsigned char en	The desired state of sleep (en=True => in sleep mode)
 *
 * \returns	The current value of the power management 1 register
 *****************************************************************************/
unsigned char mpuSleepEn(bool en)
{
	unsigned char temp = mpuData.pwr_mgmt_1;

	if(en)
		temp |= SLEEP;								// Set the sleep bit
	else
		temp &= ~SLEEP;								// Clear the sleep bit

	if(temp != mpuData.pwr_mgmt_1){					// Check for changes to RAM copy
		if(mpuRegWrite(PWR_MGMT_1, temp) == 1){ 	// Write the control register
			mpuData.pwr_mgmt_1 = temp;
		}
	}

	return mpuData.pwr_mgmt_1;
}

/**************************************************************************//**
 * \fn 		unsigned int mpuSetSampRate(unsigned int SR)
 * \brief	This function sets the sampling rate of MPU6050
 *
 * This function takes a sampling rate (integer < 1 or 8kHz depending on DLPF) and
 * attempts to set the MPU6050 sampling divisor to produce a sampling rate as close
 * as possible to this rate.
 *
 * \param	unsigned int SR		The desired sampling rate
 *
 * \returns	The actual sampling rate set in the device
 *****************************************************************************/
unsigned int mpuSetSampRate(unsigned int SR)
{
	unsigned char srDiv;

	if(SR > GYRO_OUT_RATE){							// Check for out of bounds
		srDiv = 1;									// If so choose maximum sampling rate
	}
	else {
		srDiv = (unsigned char)(GYRO_OUT_RATE/SR);	// Find divisor
		if((GYRO_OUT_RATE % SR) < SR/2) srDiv--;	// Round divisor down if necessary
	}

	if(mpuRegWrite(SMPLRT_DIV, srDiv) == 1){		// Write the divisor to the register
				mpuData.smplrt_div = srDiv;			// Update RAM copy
			}

	return (GYRO_OUT_RATE/mpuData.smplrt_div);		// Return actual value that sample rate is set to
}

/**************************************************************************//**
 * \fn 		unsigned char mpuAccelRangeConfig(accelFSR range)
 * \brief	This function sets the full scale range of the accelerometer
 *
 * This function takes an enumerated type containing the various accel full-scale
 * range values. Valid options are 2, 4, 8, and 16 g's.
 *
 * \param	accelFSR range	The desired full scale range
 *
 * \returns	The current state of the accel config register
 *****************************************************************************/
unsigned char mpuAccelRangeConfig(accelFSR range)
{
	unsigned char temp = mpuData.accel_config;

	temp &= ~(AFS_SEL0 + AFS_SEL1);					// Clear range bits
	temp |= (unsigned char)range;					// Set new range bits

	if(temp != mpuData.accel_config){				// Check for change
		if(mpuRegWrite(ACCEL_CONFIG, temp) == 1){
			mpuData.accel_config = temp;
		}
	}
	return mpuData.accel_config;
}

/**************************************************************************//**
 * \fn 		axisData mpuGetAccel(void)
 * \brief	This function gets the current accelerometer value from the MPU6050
 *
 * This function returns the current X, Y, Z accelerometer values as an #axisData
 * structure.
 *
 * \returns	The X, Y, Z triple output from the accelerometer
 *****************************************************************************/
axisData mpuGetAccel(void)
{
	axisData data;
	i2cPacket accelReq;
	unsigned char* temp = (unsigned char*)&data;

	accelReq.commID = mpuID;
	accelReq.regAddr = ACCEL_XOUT_H;
	accelReq.len =  6;
	accelReq.data = temp;

	while(i2cB1Read(&accelReq) == -1);
	while(getUCB1Stat() != OPEN);

	data.x = temp[1] | (temp[0] << 8);
	data.y = temp[3] | (temp[2] << 8);
	data.z = temp[5] | (temp[4] << 8);

	return data;
}

/**************************************************************************//**
 * \fn 		unsigned char mpuGyroRangeConfig(gyroFSR range)
 * \brief	This function sets the full scale range of the gyro
 *
 * This function takes an enumerated type containing the various gyro full-scale
 * range values. Valid options are 250, 500, 1000, and 2000 degrees per second.
 *
 * \param	gyroFSR range	The desired full-scale range
 *
 * \returns	The current state of the gyro config register
 *****************************************************************************/
unsigned char mpuGyroRangeConfig(gyroFSR range)
{
	unsigned char temp = mpuData.gyro_config;

	temp &= ~(FS_SEL0 + FS_SEL1);
	temp |= (unsigned char)range;

	if(temp != mpuData.gyro_config){
		if(mpuRegWrite(GYRO_CONFIG, temp) == 1){
			mpuData.gyro_config = temp;
		}
	}

	return mpuData.gyro_config;
}

/**************************************************************************//**
 * \fn 		axisData mpuGetGyro(void)
 * \brief	This function gets the current gyro value from the MPU6050
 *
 * This function returns the current X, Y, Z gyro values as an #axisData
 * structure.
 *
 * \returns	The X, Y, Z triple output from the gyro
 *****************************************************************************/
axisData mpuGetGyro(void)
{
	axisData data = { 0 };
	i2cPacket gyroReq;
	unsigned char* temp = (unsigned char*)&data;

	gyroReq.commID = mpuID;
	gyroReq.regAddr = GYRO_XOUT_H;
	gyroReq.len = 6;
	gyroReq.data = temp;

	while(i2cB1Read(&gyroReq) == -1);
	while(getUCB1Stat() != OPEN);

	data.x = temp[1] | (temp[0] << 8);
	data.y = temp[3] | (temp[2] << 8);
	data.z = temp[5] | (temp[4] << 8);

	return data;
}

/**************************************************************************//**
 * \fn 		int mpuGetTemp(void)
 * \brief	This function gets the current temperature value from the MPU6050
 *
 * This function returns the current temperature value as a signed integer
 *
 * \returns	The current MPU6050 temperature
 *****************************************************************************/
int mpuGetTemp(void)
{
	i2cPacket tempReq;
	unsigned char temp[2];

	tempReq.commID = mpuID;
	tempReq.regAddr = TEMP_OUT_H;
	tempReq.len = 2;
	tempReq.data = temp;

	while(i2cB1Read(&tempReq) == -1);
	while(getUCB1Stat() != OPEN);

	return (unsigned int)(temp[1]) + ((unsigned int)(temp[0]) << 8);
}


/**************************************************************************//**
 * \fn 		unsigned int mpuMotionConfig(unsigned char thresh, bool intEn, accelDelay onDelay)
 * \brief	This function sets up the MPU6050 parameters related to motion detection
 *
 * This function takes a motion threshold, interrupt enable, and turn-on delay
 * and configures the MPU6050 appropriately. All changes made to the MPU6050 are
 * updated in the RAM copy.
 *
 * \param	unsigned char thresh	The motion threshold for the MPU6050
 * \param	bool	intEn			The state of the interrupt enable for motion
 * \param	accelDelay	onDelay		The on delay for the accelerometer
 *
 * \retval	0		Success
 * \retval	-1		Set motion threshold fail
 * \retval	-2		Set motion interrupt enable fail
 * \retval	-3		Set on delay fail
 *****************************************************************************/
unsigned int mpuMotionConfig(unsigned char thresh, bool intEn, accelDelay onDelay)
{
	unsigned char temp = mpuData.int_enable;
	unsigned int retval = 0;

	if(thresh != mpuData.mot_thr){
		if(mpuRegWrite(MOT_THR, thresh) == 1){
			mpuData.mot_thr = thresh;
		}
		else retval = -1;
	}

	if((temp & MOT_INT) && !intEn){				// Motion interrupt needs to be cleared
		temp &= ~MOT_INT;
		if(mpuRegWrite(INT_ENABLE, temp) == 1){
			mpuData.int_enable = temp;
		}
		else retval = -2;
	}
	else if(!(temp & MOT_INT) && intEn){			// Motion interrupt needs to be set
		temp |= MOT_INT;
		if(mpuRegWrite(INT_ENABLE, temp) == 1){
			mpuData.int_enable = temp;
		}
		else retval = -2;
	}

	if(onDelay != (mpuData.mot_detect_ctrl & AOD_MASK)){
		if(mpuRegWrite(MOT_DETECT_CTRL, onDelay) == 1){
			mpuData.mot_detect_ctrl = onDelay & AOD_MASK;
		}
		else retval = -3;
	}

	return retval;
}

/**************************************************************************//**
 * \fn 		int mpuIntConfig(unsigned char intEnable, unsigned char intPinCfg
 * \brief	This function sets up the MPU6050 interrupt enable and
 * 			interrupt pin configuration registers
 *
 * This function takes the desired values of the interrupt enable and interrupt
 * pin configuration registers, checks them against the RAM copy and writes if
 * the status has changed.
 *
 * \param	unsigned char	intEn			The state of the interrupt enable for motion
 * \param	unsigned char 	intPinCfg		The desired fuctionality of the interrupt pin
 *
 * \retval	0		Success
 * \retval	-1		Interrupt enable fail
 * \retval	-2		Interrupt pin configuration fail
 *****************************************************************************/
int mpuIntConfig(unsigned char intEnable, unsigned char intPinCfg)
{
	if(intEnable ^ mpuData.int_enable){
		if(mpuRegWrite(INT_ENABLE, intEnable) == 1){
			mpuData.int_enable = intEnable;
		}
		else return -1;
	}

	if(intPinCfg ^ mpuData.int_pin_cfg){
		if(mpuRegWrite(INT_PIN_CFG, intPinCfg) == 1){
			mpuData.int_pin_cfg = intPinCfg;
		}
		else return -2;
	}

	return 1;
}

/**************************************************************************//**
 * \fn 		inline void mpuClearBuff(void)
 * \brief	This function clears the MPU data buffer
 *
 *	Clears all data fetched from the MPU6050 stored in RAM
 *****************************************************************************/
void mpuClearBuff(void)
{
	resetUCB1(mpuID);
}

/**************************************************************************//**
 * \fn 		inline unsigned char mpuGetIntStatus(void)
 * \brief	This function returns the status of the MPU interrupt enable
 *
 *	Gets the MPU6050 interrupt status register and returns it to the user
 *
 * \returns	The current state of the interrupt status register
 * \retval	-1	The USCI module is currently busy
 *****************************************************************************/
inline unsigned char mpuGetIntStatus(void)
{
	return mpuRegRead(INT_STATUS);
}

/**************************************************************************//**
 * \fn 		inline unsigned char mpuWhoAmI(void)
 * \brief	This function returns the value from the MPU WHOAMI register
 *
 *	Gets the MPU6050 WHOAMI register value (should always be 0x68)
 *	and returns it to the user
 *
 * \returns	The WHOAMI value
 * \retval	-1	The USCI module is currently busy
 *****************************************************************************/
inline unsigned char mpuWhoAmI(void)
{
	unsigned char retval = 0;
	retval = mpuRegRead(WHOAMI);
	return (retval & WHOAMI_MASK);
}
