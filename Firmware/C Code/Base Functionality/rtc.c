/*
 * rtc.c
 *
 *  Created on: Aug 12, 2013
 *      Author: bb3jd
 */
#include "timing.h"
#include "rtc.h"

static time localTime;

time* rtcGetTime(void)
{
	while(1){
		if(RTCCTL01 & RTCRDY) {
			localTime.sec = RTCSEC;
			localTime.min = RTCMIN;
			localTime.hour = RTCHOUR;
			localTime.day = RTCDAY;
			localTime.mon = RTCMON;
			localTime.year = RTCYEAR;
			break;
		}
	}
	return &localTime;
}

void rtcSetTime(time* t)
{
	RTCSEC = (unsigned char)(t->sec);
	RTCMIN = (unsigned char)(t->min);
	RTCHOUR = (unsigned char)(t->hour);
	RTCDAY = (unsigned char)(t->day);
	RTCMON = (unsigned char)(t->mon);
	RTCYEAR = t->year;
	rtcGetTime();			// Update the software buffer
}

void rtcInit(time* currTime)
{
	const aclkConf rtcConf = {LFXT,DIV1};	// Set up ACLK @ 32768Hz for RTC
	setACLK(rtcConf);
	RTCCTL01 = RTCMODE;
	rtcSetTime(currTime);
}

#pragma vector = RTC_VECTOR
__interrupt void RTC_ISR(void)
{
	rtcGetTime();
}

