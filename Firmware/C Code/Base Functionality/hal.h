/*
 * tempo4hal.h
 *
 *  Created on: Nov 8, 2013
 *      Author: bb3jd
 */
#ifndef TEMPO4HAL_H_
#define TEMPO4HAL_H_
#include <msp430.h>

// Bit Access Structure
typedef struct Bits8 ///< Bitwise access structure for a single byte
{
    volatile unsigned Bitx0 : 1 ;
    volatile unsigned Bitx1 : 1 ;
    volatile unsigned Bitx2 : 1 ;
    volatile unsigned Bitx3 : 1 ;
    volatile unsigned Bitx4 : 1 ;
    volatile unsigned Bitx5 : 1 ;
    volatile unsigned Bitx6 : 1 ;
    volatile unsigned Bitx7 : 1 ;
} Bits ;

// Bit Access Defines
#define B8_0(x) (((Bits *) (x))->Bitx0)		///< Bit 0 (LSB) Access Macro
#define B8_1(x) (((Bits *) (x))->Bitx1)		///< Bit 1 Access Macro
#define B8_2(x) (((Bits *) (x))->Bitx2)		///< Bit 2 Access Macro
#define B8_3(x) (((Bits *) (x))->Bitx3)		///< Bit 3 Access Macro
#define B8_4(x) (((Bits *) (x))->Bitx4)		///< Bit 4 Access Macro
#define B8_5(x) (((Bits *) (x))->Bitx5)		///< Bit 5 Access Macro
#define B8_6(x) (((Bits *) (x))->Bitx6)		///< Bit 6 Access Macro
#define B8_7(x) (((Bits *) (x))->Bitx7)		///< Bit 7 (MSB) Access Macro

// Port 1 Pins
#define LED1			B8_0(&P1OUT)			///< LED 1 Pin Define
#define LED2			B8_1(&P1OUT)			///< LED 2 Pin Define
#define	SW1				B8_2(&P1IN)				///< Switch 1 Pin Define
#define SW2				B8_3(&P1IN)				///< Switch 2 Pin Define
#define EXT_VCC_EN		B8_4(&P1OUT)			///< External VCC (VCC2) Control Define
#define	CHG				B8_5(&P1IN)				///< MAX1555 Charge Indicator Pin Define
#define USB_VCC			B8_6(&P1IN)				///< FT232 VCC Pin Define
#define MPU_INT			B8_7(&P1IN)				///< MPU Interrupt Pin Define
// Port 2 Pins
#define MPU_FSYNC		B8_7(&P2OUT)
// Port 4 Pins
#define TEST1_IN		B8_6(&P4IN)				///< TEST1 (when used as an input) pin define
#define TEST1_OUT		B8_6(&P4OUT)			///< TEST1 (when used as an output) pin define
#define TEST2_IN		B8_7(&P4IN)				///< TEST2 (when used as an input) pin define
#define TEST2_OUT		B8_7(&P4OUT)			///< TEST2 (when used as an output) pin define
// Port 5 Pins
#define MMC_CS			B8_2(&P5OUT)			///< MMC Chip Select (active low) pin define
#define MMC_CD			B8_3(&P5IN)				///< MMC Card Detect (active low) pin define

// LED 1 (Green) Functions
#define LED1_CONFIG()		P1DS |= BIT0; P1OUT |= BIT0; P1DIR |= BIT0; P1SEL &= ~(BIT0)						///< Set P1.0 to output high (LED off)
#define LED1_ON()			LED1 = 0			///< LED 1 on macro
#define LED1_OFF()			LED1 = 1			///< LED 1 off macro
#define LED1_TOGGLE()		LED1 ^= 1			///< LED 1 toggle macro
#define LED_GREEN_CONFIG()	LED1_CONFIG()		///< Rename macro for green LED configuration
#define LED_GREEN_ON()		LED1_ON()			///< Rename macro for green LED (LED1) on
#define LED_GREEN_OFF()		LED1_OFF()			///< Rename macro for green LED (LED1) off
#define LED_GREEN_TOGGLE()	LED1_TOGGLE()		///< Rename macro for green LED (LED1) toggle
// LED 2 (Red) Functions
#define LED2_CONFIG()		P1DS |= BIT1; P1OUT |= BIT1; P1DIR |= BIT1; P1SEL &= ~(BIT1) 						///< Set P1.1 to output high (LED off)
#define LED2_ON()			LED2 = 0			///< LED2 on macro
#define LED2_OFF()			LED2 = 1			///< LED2 off macro
#define LED2_TOGGLE()		LED2 ^= 1			///< LED2 toggle macro
#define LED_RED_CONFIG()	LED2_CONFIG()		///< Rename macro for red LED configuration
#define LED_RED_ON()		LED2_ON()			///< Rename macro for red LED (LED2) on
#define LED_RED_OFF()		LED2_OFF()			///< Rename macro for red LED (LED2) off
#define LED_RED_TOGGLE()	LED2_TOGGLE()		///< Rename macro for red LED (LED2) toggle
// General LED defines
#define LED_CONFIG()		LED1_CONFIG(); LED2_CONFIG()	///< Group LED configuration
#define LEDS_OFF()			LED1_OFF(); LED2_OFF()			///< Group LED off
#define LEDS_ON()			LED1_ON(); LED2_ON()			///< Group LED on
// Switch 1 Functions
#define SW1_CONFIG()		P1OUT |= BIT2; P1REN |= BIT2; P1DIR &= ~(BIT2);  P1SEL &= ~(BIT2)	///< Set P1.2 to input w/ pull-up (switch pulls down)
#define SW1_PRESSED			SW1 == 0			///< SW1 depressed (closed) macro
#define SW1_RELEASED		SW1 == 1			///< SW1 released (open) macro
// Switch 2 Functions
#define SW2_CONFIG()		P1OUT |= BIT3; P1REN |= BIT3; P1DIR &= ~(BIT3);  P1SEL &= ~(BIT3)	///< Set P1.3 to input w/ pull-up (switch pulls down)
#define SW2_PRESSED			SW2 == 0			///< SW2 depressed (closed) macro
#define SW2_RELEASED		SW2 == 1			///< SW2 released (open) macro
// General Switch Defines
#define SW_CONFIG()			SW1_CONFIG(); SW2_CONFIG()		///< Group switch config
#define SW_PRESSED			SW1_PRESSED | SW2_PRESSED		///< Group switch depressed (closed) macro
#define SW_RELEASED			SW1_RELEASED & SW2_RELEASED		///< Group switch released (open) macro
// Ext. VCC Control Functions
#define EXT_VCC_CONFIG()	P1OUT &= ~(BIT4); P1DIR |= BIT4; P1SEL &= ~(BIT4)					///< Set P1.4 to output low (VCC2 off)
#define EXT_VCC_ON()		P1OUT |= 0x10		///< External VCC on macro
#define EXT_VCC_OFF()		P1OUT &= ~(0x10)	///< External VCC off macro
// MAX1555 Charge Indicator Functions
#define CHG_CONFIG()		P1REN &= ~(BIT5); P1DIR &= ~(BIT5); P1SEL &= ~(BIT5)				///< Set P1.5 to input (no pull-up/down)
#define	CHARGING			CHG == 0			///< Charging macro
// FT232 USB Power Indicator Functions
#define USB_VCC_CONFIG()	P1OUT &= ~(BIT6); P1DIR &= ~(BIT6); P1SEL &= ~(BIT6) //; P1REN |= BIT6	///< Set P1.6 to input (w/o pull-down)
#define USB_VCC_ON			USB_VCC == 1		///< USB VCC available macro
#define USB_VCC_OFF			USB_VCC == 0		///< USV VCC unavailable macro
// MPU Pin Indicator Functions
#define MPU_INT_CONFIG()	P1REN &= ~(BIT7); P1SEL &= ~(BIT7); P1DIR &= ~(BIT7)				///< Set P1.7 to input (no pull-up/down)
#define MPU_FSYNC_CONFIG()	P2OUT &= ~(BIT7); P2SEL &= ~(BIT7); P2DIR |= BIT7					///< Set P2.7 to output low
#define MPU_INT_HIGH		MPU_INT == 1		///< MPU interrupt line high macro
#define MPU_INT_LOW			MPU_INT == 0		///< MPU interrupt line low macro
#define MPU_FSYNC_ON()		MPU_FSYNC = 1		///< MPU frame synchronization on (high) macro
#define MPU_FSYNC_OFF()		MPU_FSYNC = 0		///< MPU frame synchronization off (low) macro
#define MPU_IO_CONFIG()		MPU_INT_CONFIG() //; MPU_FSYNC_CONFIG() 	///< MPU IO initialization macro
// MMC Pin Indicator functions
#define MMC_IO_CONFIG()		MMC_CS_CONFIG(); MMC_CD_CONFIG()
#define MMC_CS_CONFIG()		P5DIR |= BIT2; P5SEL &= ~(BIT2); P5OUT |= BIT2						///< Set P5.2 to output high (not selected)
#define MMC_CD_CONFIG()		P5OUT |= BIT3; P5REN |= BIT3; P5DIR &= ~(BIT3); P5SEL &= ~(BIT3)	///< Set P5.3 to input (w/ pull-up)
#define MMC_CARD_PRESENT	!(MMC_CD)		///< MMC Card Present Indicator

// MCLK Out on TEST1
#define MCLK_CONFIG()	P4SEL |= BIT6; P4DIR |= BIT6;\
			PMAPKEYID = PMAPKEY; P4MAP6 = PM_MCLK; PMAPKEYID = 0
#define MCLK_OFF()		P4SEL &= ~BIT6; P4DIR &= ~BIT6


#endif /* TEMPO4HAL_H_ */
