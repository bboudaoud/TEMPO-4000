/*
 * rtc.h
 *
 *  Created on: Aug 12, 2013
 *      Author: bb3jd
 */
#ifndef RTC_H_
#define RTC_H_

typedef enum month_type{
	JAN = 1,
	FEB = 2,
	MAR = 3,
	APR	= 4,
	MAY	= 5,
	JUN	= 6,
	JUL = 7,
	AUG	= 8,
	SEP = 9,
	OCT	= 10,
	NOV = 11,
	DEC	= 12
} month;


typedef struct timestamp
{
	unsigned int year;
	unsigned int mon;
	unsigned int day;
	unsigned int hour;
	unsigned int min;
	unsigned int sec;
} time;

time* rtcGetTime(void);
void rtcSetTime(time* t);
void rtcInit(time* currTime);

#endif /* RTC_H_ */
