/****************************************************************//**
 * \file MPU6000.h
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

#ifndef MPU_H_
#define MPU_H_
#include "util.h"

/***********************************************************************
 *  NOTE: All vales in this register description file can be found in
 *  the Invensense Register Map Document located at:
 * [http://invensense.com/mems/gyro/documents/RM-MPU-6000A.pdf]
 **********************************************************************/
#define AD0					0					///< Value of the AD0 pin on MPU-6050 (can only be 0 or 1)
#define MPU_CHIP_ADDR		0x68+(AD0 & 0x01)	///< I2C Chip Address of MPU-6050 (AD0, LSB of address is selectable)
#define MPU_RESET_DELAY_MS	10					///< Delay for next command after device reset
#define GYRO_OUT_RATE		1000				///< Gyro output rate in Hz (8k w/o DLPF, 1k w/ DLPF)
#define MPU_BAUD			40000				///< Baud rate for communications w/ the MPU6050 via I2C

// Command Formatting
#define MPU_ADDR_MASK		0x7F				///< 7-bit Chip Address Mask for MPU6050
//#define READ_CMD(x)		(x)|0x80			///< Read command (set bit 8)
//#define WRITE_CMD(x)		(x)&0x7F			///< Write command (clear bit 8)

/*******************************************************************************************
 * Register Map
 ******************************************************************************************/
//#define MPUREG_AUX_VDDIO  0x01
#define SELF_TEST_X			0x0D	///< X Axis Test Value Register
// { XA Test (bits 4-2) [7-5] } { XG Test [4-0] }
#define SELF_TEST_Y			0x0E	///< Y Axis Test Value Register
//{ YA Test (bits 4-2) [7-5] } { YG Test [4-0] }
#define SELF_TEST_Z			0x0F	///< Z Axis Test Value Register
//{ ZA Test (bits 4-2) [7-5] } { ZG Test [4-0] }
#define SELF_TEST_A			0x10	///< 3 Axis Accel. Test Values
//{ Reserved [7-6] } { XA Test (bits 1-0) [5-4] } { YA Test (bits 1-0) [3-2] } { ZA Test (bits 1-0) [1-0] }
#define SMPLRT_DIV			0x19	///< Sample rate divisor:
//{ Sample Rate Div [7-0] } NOTE: this divides the gyro output rate (either 1 or 8 kHz depending on DLPF_CFG)
#define	CONFIG				0x1A	///< Configuration Register
//{ Not used [7-6] } { EXT_SYNC_SET [5-3] } { DLPF_CFG [2-0]}
#define GYRO_CONFIG			0x1B	///< Gyro Configuration Register
//{ XG Self Test [7] } { YG Self Test [6] } { ZG Self Test [5] } { FS_SEL [4-3] }  {Not used [2-0] } NOTE: FS_SEL selects sensitivity
#define ACCEL_CONFIG		0x1C	///< Accel. Configuration Register
//{ XA Self Test [7] } { YA Self Test [6] } { ZA Self Test [5] } { AFS_SEL [4-3] }
#define MOT_THR				0x1F	///< Motion Threshold Register
//{ Motion Threshold [7-0] }
#define FIFO_EN_REG			0x23	///< FIFO Enable Register
//{ Temperature En. [7] } { X Gyro En. [6] } { Y Gyro En. [5] } { Z Gyro En. [4] } { Accel En. [3] } { I2C SLV2 En. [2] } { I2C SLV1 En. [1] } { I2C SLV0 En. [0] }
#define	I2C_MST_CTRL		0x24	///< I2C Master Control Register
//{ Multi-master En. [7] } { Wait for ext. sensor [6] } { I2C SLV3 FIFO En. [5] } { I2C Master NSR [4] } { I2C Master Clock Control [3-0] }
#define I2C_SLV0_ADDR		0x25	///< I2C Slave 0 Address Register
//{ I2C SLV0 Read/Write [7] } { I2C SLV0 Addr. [6-0] }
#define I2C_SLV0_REG		0x26	///< I2C Slave 0 Data Register
//{ I2C SLV0 Data [7-0] }
#define I2C_SLV0_CTRL		0x27	///< I2C SLave 0 Control Register
//{ I2C SLV0 En. [7] } { I2C SLV0 Byte Swapping [6] } { I2C SLV0 DIS [5] } { I2C SLV0 Word Grouping [4] } { I2C SLV0 TX/RX Length [3-0] }
#define I2C_SLV1_ADDR		0x28	///< I2C Slave 1 Address Register
//{ I2C SLV1 Read/Write [7] } { I2C SLV1 Addr. [6-0] }
#define I2C_SLV1_REG		0x29	///< I2C Slave 1 Data Register
//{ I2C SLV1 Data [7-0] }
#define I2C_SLV1_CTRL		0x2A	///< I2C Slave 1 Control Register
//{ I2C SLV1 En. [7] } { I2C SLV1 Byte Swapping [6] } { I2C SLV1 DIS [5] } { I2C SLV1 Word Grouping [4] } { I2C SLV1 TX/RX Length [3-0] }
#define I2C_SLV2_ADDR		0x2B	///< I2C Slave 2 Address Register
//{ I2C SLV2 Read/Write [7] } { I2C SLV2 Addr. [6-0] }
#define I2C_SLV2_REG		0x2C	///< I2C Slave 2 Data Register
//{ I2C SLV2 Data [7-0] }
#define I2C_SLV2_CTRL		0x2D	///< I2C Slave 2 Control Register
//{ I2C SLV2 En. [7] } { I2C SLV2 Byte Swapping [6] } { I2C SLV2 DIS [5] } { I2C SLV2 Word Grouping [4] } { I2C SLV2 TX/RX Length [3-0] }
#define I2C_SLV3_ADDR		0x2E	///< I2C Slave 3 Address Register
//{ I2C SLV3 Read/Write [7] } { I2C SLV3 Addr. [6-0] }
#define I2C_SLV3_REG		0x2F	///< I2C Slave 3 Data Register
//{ I2C SLV3 Data [7-0] }
#define I2C_SLV3_CTRL		0x30	///< I2C Slave 3 Control Register
//{ I2C SLV3 En. [7] } { I2C SLV3 Byte Swapping [6] } { I2C SLV3 DIS [5] } { I2C SLV3 Word Grouping [4] } { I2C SLV3 TX/RX Length [3-0] }
#define I2C_SLV4_ADDR		0x31	///< I2C Slave 4 Address Register
//{ I2C SLV4 Read/Write [7] } { I2C SLV4 Addr [6-0] }
#define I2C_SLV4_REG		0x32	///< I2C Slave 4 Data Pointer
//{ I2C SLV4 Data Transfer Address [7-0] }
#define I2C_SLV4_DO			0x33	///< I2C Slave 4 Data Out Register
//{ I2C SLV4 Data to Write to Slave 4 [7-0] }
#define I2C_SLV4_CTRL		0x34	///< I2C Slave 4 Control Register
//{ I2C SLV4 En. [7] } { I2C SLV4 Byte Swapping [6] } { I2C SLV4 DIS [5] } { I2C SLV4 Word Grouping [4] } { I2C SLV4 TX/RX Length [3-0] }
#define I2C_SLV4_DI			0x35	///< I2C Slave 4 Data In Register
//{ I2C SLV4 Data Read from SLV4 [7-0] }
#define I2C_MST_STATUS		0x36	///< I2C Master Status Register
//{ Pass Through [7] } { SLV4 Done [6] } { Lost Arbitration [5] } { SLV4 NACK [4] } { SLV3 NACK [3] } { SLV2 NACK [2] } { SLV1 NACK [1] } { SLV0 NACK [0] }
#define INT_PIN_CFG			0x37	///< Interrupt Pin Config Register
//{ Int. Level [7] } { Int. Open [6] } { Latch Int. En. [5] } { Int. Read Clear [4] } { FSYNC Int. Level [3] } { FSYNC Int. En. [2] } { I2C Bypass En. [1] } { Not Used [0] }
#define INT_ENABLE			0x38	///< Interrupt Enable Register
//{ Not Used [7] } { Motion En. [6] } { Not Used [5] } { FIFO OVFL [4] } { I2C MST INT [3] } { Not Used [2-1] } { Data Ready [0] }
#define INT_STATUS			0x3A	///< Interrupt Status Register
//{ Not Used [7] } { Motion Int. [6] } { Not Used [5] } { FIFO Int. [4] } { I2C MST INT [3] } { Not Used [2-1] } { Data Ready [0] }
#define ACCEL_XOUT_H		0x3B	///< X Accel. High Byte Register
//{ X Accel. Value High Byte [7-0] }
#define ACCEL_XOUT_L		0x3C	///< X Accel. Low Byte Register
//{ X Accel. Value Low Byte [7-0] }
#define ACCEL_YOUT_H		0x3D	///< Y Accel. High Byte Register
//{ Y Accel. Value High Byte [7-0] }
#define ACCEL_YOUT_L		0x3E	///< Y Accel. Low Byte Register
//{ Y Accel. Value Low Byte [7-0] }
#define ACCEL_ZOUT_H		0x3F	///< Z Accel. High Byte Register
//{ Z Accel. Value High Byte [7-0] }
#define ACCEL_ZOUT_L		0x40	///< Z Accel. Low Byte Register
//{ Z Accel. Value Low Byte [7-0] }
#define TEMP_OUT_H			0x41	///< Temperature High Byte Register
//{ Temperature Value High Byte [7-0] }
#define	TEMP_OUT_L			0x42	///< Temperature Low Byte Register
//{ Temperature Value Low Byte [7-0] }
#define GYRO_XOUT_H			0x43	///< X Gyro High Byte Register
//{ X Gyro Value High Byte [7-0] }
#define GYRO_XOUT_L			0x44	///< X Gyro Low Byte Register
//{ X Gyro Value Low Byte [7-0] }
#define GYRO_YOUT_H			0x45	///< Y Gyro High Byte Register
//{ Y Gyro Value High Byte [7-0] }
#define GYRO_YOUT_L			0x46	///< Y Gyro Low Byte Register
//{ Y Gyro Value Low Vyte [7-0] }
#define GYRO_ZOUT_H			0x47	///< Z Gyro High Byte Register
//{ Z Gyro Value High Byte [7-0] }
#define GYRO_ZOUT_L			0x48	///< Z Gyro Low Byte Register
//{ Z Gyro Value Low Byte [7-0] }
#define EXT_SENS_DATA_00	0x49	///< External Sensor Data Register 0
//{ Value [7-0] }
#define EXT_SENS_DATA_01	0x4A	///< External Sensor Data Register 1
//{ Value [7-0] }
#define EXT_SENS_DATA_02	0x4B	///< External Sensor Data Register 2
//{ Value [7-0] }
#define EXT_SENS_DATA_03	0x4C	///< External Sensor Data Register 3
//{ Value [7-0] }
#define EXT_SENS_DATA_04	0x4D	///< External Sensor Data Register 4
//{ Value [7-0] }
#define EXT_SENS_DATA_05	0x4E	///< External Sensor Data Register 5
//{ Value [7-0] }
#define EXT_SENS_DATA_06	0x4F	///< External Sensor Data Register 6
//{ Value [7-0] }
#define EXT_SENS_DATA_07	0x50	///< External Sensor Data Register 7
//{ Value [7-0] }
#define EXT_SENS_DATA_08	0x51	///< External Sensor Data Register 8
//{ Value [7-0] }
#define EXT_SENS_DATA_09	0x52	///< External Sensor Data Register 9
//{ Value [7-0] }
#define EXT_SENS_DATA_10	0x53	///< External Sensor Data Register 10
//{ Value [7-0] }
#define EXT_SENS_DATA_11	0x54	///< External Sensor Data Register 11
//{ Value [7-0] }
#define EXT_SENS_DATA_12	0x55	///< External Sensor Data Register 12
//{ Value [7-0] }
#define EXT_SENS_DATA_13	0x56	///< External Sensor Data Register 13
//{ Value [7-0] }
#define EXT_SENS_DATA_14	0x57	///< External Sensor Data Register 14
//{ Value [7-0] }
#define EXT_SENS_DATA_15	0x58	///< External Sensor Data Register 15
//{ Value [7-0] }
#define EXT_SENS_DATA_16	0x59	///< External Sensor Data Register 16
//{ Value [7-0] }
#define EXT_SENS_DATA_17	0x5A	///< External Sensor Data Register 17
//{ Value [7-0] }
#define EXT_SENS_DATA_18	0x5B	///< External Sensor Data Register 18
//{ Value [7-0] }
#define EXT_SENS_DATA_19	0x5C	///< External Sensor Data Register 19
//{ Value [7-0] }
#define EXT_SENS_DATA_20	0x5D	///< External Sensor Data Register 20
//{ Value [7-0] }
#define EXT_SENS_DATA_21	0x5E	///< External Sensor Data Register 21
//{ Value [7-0] }
#define EXT_SENS_DATA_22	0x5F	///< External Sensor Data Register 22
//{ Value [7-0] }
#define EXT_SENS_DATA_23	0x60	///< External Sensor Data Register 23
//{ Value [7-0] }
#define I2C_SLV0_DO			0x63	///< I2C Slave 0 Data Out
//{ I2C SLV0 Data Out [7-0] }
#define I2C_SLV1_DO			0x64	///< I2C Slave 1 Data Out
//{ I2C SLV1 Data Out [7-0] }
#define I2C_SLV2_DO			0x65	///< I2C Slave 2 Data Out
//{ I2C SLV2 Data Out [7-0] }
#define I2C_SLV3_DO			0x66	///< I2C Slave 3 Data Out
//{ I2C SLV3 Data Out [7-0] }
#define I2C_MST_DELAY_CTRL	0x67	///< I2C Master Delay Control Reg
//{ Delay Shadow En. [7] } { Not Used [6-5] } { I2C SLV4 Delay [4] } { I2C SLV3 Delay [3] } { I2C SLV2 Delay [2] } { I2C SLV1 Delay [1] } { I2C SLV0 Delay [0] }
#define SIGNAL_PATH_RESET	0x68	///< Signal Path Reset Register
//{ Not Used [7-3] } { Gyro Reset [2] } { Accel. Reset [1] } { Temp Reset [0] }
#define MOT_DETECT_CTRL		0x69	///< Motion Detection Control Reg
//{ Not Used [7-6] } { Accel. Power-on Delay [5-4] } { Not Used [3-0] }
#define USER_CTRL			0x6A	///< User Control Register
//{ Not Used [7] } { FIFO En. [6] } { I2C Master En. [5] } { I2C IF DIS [4] } { Not Used [3] } { FIFO Reset [2] } { I2C Master Reset [1] } { Signal Cond. Reset [0] }
#define PWR_MGMT_1			0x6B	///< Power Management Register 1
//{ Device Reset [7] } { Sleep [6] } { Cycle [5] } { Not Used [4] } { Temp. Disable [3] } { Clock Select [2-0] }
#define PWR_MGMT_2			0x6C	///< Power Management Register 2
//{ Low-power wake control [7-6] } { Standby X Accel [5] } { Standby Y Accel [4] } { Standby Z Accel. [3] } { Standby X Gyro [2] } { Standby Y Gyro [1] } { Standby Z Gyro [0] }
#define FIFO_COUNTH			0x72	///< FIFO Size High Byte Register
//{ FIFO Size Value High Byte [7-0] }
#define FIFO_COUNTL			0x73	///< FIFO Size Low Byte Register
//{ FIFO Size Value Low Byte [7-0] }
#define FIFO_R_W			0x74	///< FIFO Data Read/Write Register
//{ FIFO Value [7-0] }
#define WHOAMI				0x75	///< I2C Address Register
//{ Not Used [7] } { Upper 5 bits of I2C Address [6-1] } { Not Used [0] }
// NOTE: the least-significant bit of the device I2C address is provided by the status of the AD0 pin if the MPU6050 is used (this is not reflected in WHOAMI)


/*****************************************************************************************
 * Register Values
 ****************************************************************************************/
//CONFIG
#define EXT_SYNC_SET0          0x00 	///< FSYNC Disabled
#define EXT_SYNC_SET1          0x08 	///< FSYNC on low bit of TEMP_OUT_L
#define EXT_SYNC_SET2          0x10 	///< FSYNC on low bit of GYRO_XOUT_L
#define EXT_SYNC_SET3          0x18 	///< FSYNC on low bit of GYRO_YOUT_L
#define EXT_SYNC_SET4          0x20 	///< FSYNC on low bit of GYRO_ZOUT_L
#define EXT_SYNC_SET5          0x28 	///< FSYNC on low bit of ACCEL_XOUT_L
#define EXT_SYNC_SET6          0x30 	///< FSYNC on low bit of ACCEL_YOUT_L
#define EXT_SYNC_SET7          0x38 	///< FSYNC on low bit of ACCEL_ZOUT_L
#define DLPF_CFG0              0x00 	///< DLPF on setting 0. Check Register Description document for notes.
#define DLPF_CFG1              0x01 	///< DLPF on setting 0. Check Register Description document for notes.
#define DLPF_CFG2              0x02 	///< DLPF on setting 0. Check Register Description document for notes.
#define DLPF_CFG3              0x03 	///< DLPF on setting 0. Check Register Description document for notes.
#define DLPF_CFG4              0x04 	///< DLPF on setting 0. Check Register Description document for notes.
#define DLPF_CFG5              0x05 	///< DLPF on setting 0. Check Register Description document for notes.
#define DLPF_CFG6              0x06 	///< DLPF on setting 0. Check Register Description document for notes.


// GYRO CONFIG
#define XG_ST				0x80		///< Start X gyro test bit
#define YG_ST				0x40		///< Start Y gyro test bit
#define ZG_ST				0x20		///< Start Z gyro test bit
#define FS_SEL1				0x10		///< FS_SEL bit 1
#define FS_SEL0				0x08		///< FS_SEL bit 0

typedef enum gyroRange					/// Enumerated gyro range control type
{
	FSR_250dps = 0,						///< Full-scale range of 250 degrees per second
	FSR_500dps = FS_SEL0,				///< Full-scale range of 500 degrees per second
	FSR_1000dps = FS_SEL1,				///< Full-scale range of 1000 degrees per second
	FSR_2000dps = FS_SEL0 + FS_SEL1		///< Full-scale range of 2000 degrees per second
}gyroFSR;

// ACCEL_CONFIG
#define XA_ST				0x80		///< Start X accel test bit
#define YA_ST				0x40		///< Start Y accel test bit
#define ZA_ST				0x20		///< Start Z accel test bit
#define AFS_SEL1			0x10		///< AFS_SEL bit 1
#define AFS_SEL0			0x08		///< AFS_SEL bit 0

typedef enum accelerometerRange			/// Enumerated accel range control type
{
	FSR_2G = 0,							///< Full-scale range of 2g
	FSR_4G = AFS_SEL0,					///< Full-scale range of 4g
	FSR_8G = AFS_SEL1,					///< Full-scale range of 8g
	FSR_16G = AFS_SEL0 + AFS_SEL1		///< Full-scale range of 16g
}accelFSR;

// FIFO EN
#define TEMP_FIFO_EN		0x80		///< Temperature to FIFO
#define XG_FIFO_EN			0x40		///< X Gyro to FIFO
#define YG_FIFO_EN			0x20		///< Y Gyro to FIFO
#define ZG_FIFO_EN			0x10		///< Z Hyro to FIFO
#define ACCEL_FIFO_EN		0x08		///< Accel X,Y,Z to FIFO
#define SLV2_FIFO_EN		0x04		///< I2C Slave 2 to FIFO
#define SLV1_FIFO_EN		0x02		///< I2C Slave 1 to FIFO
#define SLV0_FIFO_EN		0x01		///< I2C Slave 0 to FIFO

// I2C MASTER CONTROL
#define	MULT_MST_EN			0x80		///< Enable multi-master capability
#define WAIT_FOR_ES			0x04		///< Delay data ready interrupt until EXT_SENS_DATA loaded
#define SLV3_FIFO_EN		0x02		///< Enables EXT_SENS_DATA for SLV3 to be written to FIFO
#define	I2C_MST_P_NSR		0x01		///< 0: reset occurs between slave reads, 1: stop and start marking between reads
#define I2C_MST_CLK_MASK	0x0F		///< I2C Clk Rate divisor mask (SEE REGISTER MAP PAGE 19 FOR FREQUENCIES)

// I2C ADDR
#define	I2C_RW				0x80		///< I2C Read/Write Control (1: Read, 0: Write)
#define I2C_ADDR_MASK		0x7F		///< 7 Bit slave address field

// I2C CTRL
#define I2C_SLV_EN			0x80		///< Enable slave device for data transfers
#define I2C_SLV_BYTE_SW		0x40		///< Enable byte swapping (high and low byte order)
#define I2C_SLV_REG_DIS		0x20		///< 1: Read/write only, 0: write before read
#define I2C_SLV_GRP			0x10		///< Byte pairing for word order (0: 0 and 1 form a word, 1: 1 and 2 form a word)
#define I2C_SLV_LEN_MASK	0x0F		///< Number of bytes transferred to/from slave device

// I2C MASTER STATUS
#define PASS_THROUGHT		0x80		///< Status of FSYNC interrupt from an external device (interrupt triggered if FSYNC_INT_EN asserted in INT_PIN_CFG)
#define I2C_SLV4_DONE		0x40		///< Set to 1 when Slave 4 completes transmit (interrupt triggered if SLV4_DONE_INT asserted in I2C_SLV4_CTRL)
#define I2C_LOST_ARB		0x20		///< Set to 1 when I2C master loses arbitration of the bus (interrupt triggered if I2C_MST_INT_EN asserted in INT_ENABLE)
#define I2C_SLV4_NACK		0x10		///< Set when I2C master receives a NACK from slave 4 (interrupt triggered if I2C_MST_INT_EN asserted in INT_ENABLE)
#define I2C_SLV3_NACK		0x08		///< Same for slave 3
#define I2C_SLV2_NACK		0x04		///< Same for slave 2
#define I2C_SLV1_NACK		0x02		///< Same for slave 1
#define I2C_SLV0_NACK		0x01		///< Same for slave 0

// INTERRUPT PIN CONFIG
#define INT_LEVEL			0x80		///< Current status of the INT pin
#define INT_OPEN			0x40		///< 0: INT pin configured as push-pull, 1: INT pin configured as open-drain
#define LATCH_INT_EN		0x20		///< 0: INT pin emits a 50us long pulse, 1: INT pin is held high until cleared
#define INT_RD_CLEAR		0x10		///< 0: Interrupt status bits are cleared only by reading INT_STATUS, 1: Interrupt bits cleared on any read
#define FSYNC_INT_LEVEL		0x08		///< 0: FSYNC is active high, 1: FSYNC is active low
#define FSYNC_INT_EN		0x04		///< 0: Disable FSYNC interrupts, 1: Enable FSYNC interrupts
#define I2C_BYPASS_EN		0x02		///< 0: Host cannot directly access I2C bus, 1: If I2C_MST_EN is 0 the host processor can directly access the auxiliary I2C bus

// INTERRUPT ENABLE/STATUS (write to INT_ENABLE, read from INT_STATUS)
#define MOT_INT				0x40		///< Enable motion detection interrupt bit
#define FIFO_OFLOW_INT		0x10		///< Enable FIFO overflow interrupt generation
#define I2C_MST_INT_INT		0x08		///< Enable I2C interrupt sources
#define	DATA_RDY_INT		0x01		///< Enable sensor register write interrupt

// I2C MASTER DELAY CONTROL
#define DELAY_ES_SHADOW		0x80		///< When set delays shadowing of external sensor data until all data has been RX'd
#define	I2C_SLV4_DLY_EN		0x10		///< Slave 4 only accessed at a decreased rate
#define I2C_SLV3_DLY_EN		0x08		///< Same as #I2C_SLV4_DLY_EN for slave 3
#define I2C_SLV2_DLY_EN		0x04		///< Same as #I2C_SLV4_DLY_EN for slave 2
#define I2C_SLV1_DLY_EN		0x02		///< Same as #I2C_SLV4_DLY_EN for slave 1
#define I2C_SLV0_DLY_EN		0x01		///< Same as #I2C_SLV4_DLY_EN for slave 0

// SIGNAL PATH RESET
#define GYRO_RESET			0x04		///< Reset analog and digital gyro signal paths
#define ACCEL_RESET			0x02		///< Reset analog and digital accel signal paths
#define TEMP_RESET			0x01		///< Reset analog and digital temp signal paths

// MOTION DETECTION CONTROL
// NOTE: accelerometer has a default start-up delay of 4ms, these bits can be used to extend it up to 7ms
#define ACCEL_ON_DELAY1		0x20		///< Accel. power on additional delay bit 1
#define ACCEL_ON_DELAY0		0x10		///< Accel. power on additional delay bit 0
#define AOD_MASK			ACCEL_ON_DELAY0 + ACCEL_ON_DELAY1	///< Mask for acceleromter on delay bits

typedef enum accelOnDelay							/// Accel on delay enumerated type
{
	delay_4ms = 0,									///< On delay of 4ms total (+0 extra ms)
	delay_5ms = ACCEL_ON_DELAY0,					///< On delay of 5ms total (+1 extra ms)
	delay_6ms = ACCEL_ON_DELAY1,					///< On delay of 6ms total (+2 extra ms)
	delay_7ms = ACCEL_ON_DELAY0 + ACCEL_ON_DELAY1	///< On delay of 7ms total (+3 extra ms)
} accelDelay;

// USER CONTROL
// NOTE: for MPU6000 the primary SPI interface will be enabled in place of primary I2C when I2C_IF_DIS is 1
#define	FIFO_EN				0x04		///< Enable FIFO operation
#define I2C_MST_EN			0x02		///< Enable I2C Master mode
#define I2C_IF_DIS			0x01		///< MPU6000: Disable I2C and enable SPI
										///< MPU6050: Always write as 0
#define	FIFO_RESET			0x04		///< Reset FIFO buffer when FIFO_EN = 0 (auto-clears to 0 on reset complete)
#define I2C_MST_RESET		0x02		///< Reset I2C Master when I2C_MST_EN = 0 (auto-clears to 0 on reset complete)
#define SIG_COND_RESET		0x01		///< Resets the signal path for all sensor (auto-clear to 0 on reset complete)

// POWER MANAGEMENT 1
#define DEVICE_RESET		0x80		///< Reset all internal registers to defaults (auto-clears to 0 on reset complete)
#define SLEEP				0x40		///< Enter sleep mode
#define CYCLE				0x20		///< When set and SLEEP cleared, the device will wake to take a single sample from each sensor
										///< NOTE: the rate of this wake is determined by the value of LP_WAKE_CTRL
#define TEMP_DIS			0x08		///< Disables the temperature sensor
#define CLKSEL2				0x04		///< Clock source selection bit 2
#define CLKSEL1				0x02		///< Clock source selection bit 1
#define CLKSEL0				0x01		///< Clock source selection bit 0
// Operating clock freq enumeration
#define CLK_SEL_8MHZ		0x00		///< Select the internal 8MHz oscillator for operation
#define CLK_SEL_XG_PLL		0x01		///< Use the internal PLL with X gyro reference
#define CLK_SEL_YG_PLL		0x02		///< Use the internal PLL with Y gyro reference
#define CLK_SEL_ZG_PLL		0x03		///< Use the internal PLL with Z gyro reference
#define CLK_SEL_EXT32_PLL	0x04		///< Use the internal PLL with external 32.768kHz crystal reference
#define CLK_SEL_EXT19_PLL	0x05		///< Use the internal PLL with external 19.2MHz crystal reference
#define CLK_SEL_STOP		0x07		///< Stop the clock and keep the timing generator in reset

// POWER MANAGEMENT 2
#define LP_WAKE_CTRL1		0x80		///< Low-power wake up control bit 1
#define LP_WAKE_CTRL0		0x40		///< Low-power wake up control bit 0
#define STBY_XA				0x20		///< X accel. standby mode control bit
#define STBY_YA				0x10		///< Y accel. standby mode control bit
#define STBY_ZA				0x08		///< Z accel. standby mode control bit
#define STBY_XG				0x04		///< X gyro standby mode control bit
#define STBY_YG				0x02		///< Y gyro standby mode control bit
#define STBY_ZG				0x01		///< Z gyro standby mode control bit
// wake up freqs (must enable CYCLE bit in POWER MANAGEMENT 1 to use these codes)
#define LP_WAKE_1.25HZ		0x00		///< Low-power wake up at 1.25Hz
#define LP_WAKE_5HZ			0x01		///< Low-power wake up at 5Hz
#define LP_WAKE_20HZ		0x02		///< Low-power wake up at 20Hz
#define LP_WAKE_40HZ		0x03		///< Low-power wake up at 40Hz

// WHO AM I
#define WHOAMI_MASK			0x7E		///< Mask for 6 bit I2C address of MPU-60X0
#define WHOAMI_VAL			0x68		///< WWHOAMI value from register

typedef struct mpuInformation			/// Typedef structure for MPU6050 state control
{
	unsigned char smplrt_div;			///< Sample rate divisor
	unsigned char config;				///< Chip configuration
	unsigned char gyro_config;			///< Gyro configuration
	unsigned char accel_config;			///< Accel configuration
	unsigned char mot_thr;				///< Motion threshold
	unsigned char fifo_en;				///< Fifo enable
	unsigned char i2c_mst_ctrl;			///< MPU6050 I2C master control
	unsigned char int_pin_cfg;			///< Interrupt pin configuration
	unsigned char int_enable;			///< Interrupt enable
	unsigned char mot_detect_ctrl;		///< Motion detection control
	unsigned char user_ctrl;			///< User control
	unsigned char pwr_mgmt_1;			///< Power management 1
	unsigned char pwr_mgmt_2;			///< Power management 2
}mpuInfo;
#define MPU_INFO_SIZE	13

typedef struct axisDat					/// 3 Dimensional Axis Data Type
{
	int x;								///< X-axis value
	int y;								///< Y-axis value
	int z;								///< Z-axis value
}axisData;

typedef struct axisCtrl					/// 6 axis control bit field
{
	volatile unsigned Zgyro : 1;		///< Z gyro control
	volatile unsigned Ygyro : 1;		///< Y gyro control
	volatile unsigned Xgyro : 1;		///< X gyro control
	volatile unsigned Zaccel : 1;		///< Z accel control
	volatile unsigned Yaccel : 1;		///< Y accel control
	volatile unsigned Xaccel : 1;		///< X accel control
	volatile unsigned Temp : 1;			///< Temperature control
}axisField;

typedef enum boolean					/// Boolean typedef
{
	True = 0xFF,						///< True (!= 0) value
	False = 0x00						///< False (== 0) value
}bool;

// Function prototypes
unsigned char mpuRegRead(unsigned char regAddr);
int mpuRegWrite(unsigned char regAddr, unsigned char toWrite);
int mpuInit(void);
int mpuSetup(mpuInfo* info);
inline void mpuReset(void);
unsigned char mpuSleepEn(bool en);
unsigned int mpuSetSampRate(unsigned int SR);
unsigned char mpuAccelRangeConfig(accelFSR range);
axisData mpuGetAccel(void);
unsigned char mpuGyroRangeConfig(gyroFSR range);
axisData mpuGetGyro(void);
int mpuGetTemp(void);
unsigned int mpuMotionConfig(unsigned char thresh, bool intEn, accelDelay onDelay);
unsigned int mpuFifoConfig();
inline unsigned char mpuGetIntStatus(void);
inline unsigned char mpuWhoAmI(void);

#endif
