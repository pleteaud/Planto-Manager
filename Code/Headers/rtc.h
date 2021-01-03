/*
 * rtc.h
 *
 * Created: 7/14/2019 2:42:38 PM
 *  Author: plete
 */ 


#ifndef RTC_H_
#define RTC_H_

/************************************************************************/
/*							Includes/Constants	 	                    */
/************************************************************************/
#include "stdbool.h"
#include "stdint.h"
#include "alarm.h"

#define DIGITS_PER_TIME_UNIT	(0x02)

/************************************************************************/
/*						Enums Definition				                */
/************************************************************************/
enum time_units_e
{ 
	TIME_UNITS_SEC = 0,  
	TIME_UNITS_MIN, 
	TIME_UNITS_HR, 
	TIME_UNITS_DY, 
	TIME_UNITS_DT, 
	TIME_UNITS_MO_CEN, 
	TIME_UNITS_YR, 
	TIME_UNITS_TOTAL
}; 

enum days_e 
{
	MON = 1, 
	TUES, 
	WED, 
	THURS, 
	FRI, 
	SAT, 
	SUN
};
enum months_e 
{
	JAN = 1,
	FEB,
	MAR,
	APR,
	MAY,
	JUN,
	JUL,
	AUG,
	SEP,
	OCT,
	NOV,
	DEC
};


enum rtc_errors_e
{
	INCONSISTENT_CTRL_REG = 0,
	INCONSISTENT_STAT_REG,
	INCONSISTENT_A1,
	INCONSISTENT_A2,
	OSC_STOP
};

enum alarm_pos_e
{
	ALARM_1 = 0,
	ALARM_2 = 1,
	NUM_OF_ALARMS
};

/************************************************************************/
/*				Type Defs + Struct Implementation						*/
/************************************************************************/
typedef struct rtc_manager_s 
{
	uint8_t time[TIME_UNITS_TOTAL];
	alarm_t alarm1;
	alarm_t alarm2;
	uint8_t ctrlReg;
	uint8_t ctrlStatReg;
	uint8_t agingOffsetReg;
	uint16_t tempReg;
	uint8_t errorCount;
	uint8_t errorList[256];
} rtc_manager_t;

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
void rtcInit(rtc_manager_t *rtcP);

bool rtcSetSeconds(rtc_manager_t *rtcP, uint8_t seconds);

bool rtcSetMinutes(rtc_manager_t *rtcP, uint8_t min);

bool rtcSetHour(rtc_manager_t *rtcP, uint8_t hour);

bool rtcSetDay(rtc_manager_t *rtcP, enum days_e day);

bool rtcSetDate(rtc_manager_t *rtcP, uint8_t date);

bool rtcSetMonCen(rtc_manager_t *rtcP, uint8_t mon, bool century);

bool rtcSetTime(rtc_manager_t *rtcP, uint8_t *time);

bool rtcSetCtrlReg(rtc_manager_t *rtcP,uint8_t newCtrlReg);

bool rtcSetStatReg(rtc_manager_t *rtcP,uint8_t newStatReg);


bool rtcSetAlarm1(rtc_manager_t *rtcP, uint8_t *time, enum alarm_match_options_e matchFlag,
				 void (*funcP)(void *objP), void *objP);
				 
bool rtcSetAlarm2(rtc_manager_t *rtcP, uint8_t *time, enum alarm_match_options_e matchFlag,
				  void (*funcP)(void *objP), void *objP);

void rtcSetAlarmCallback(rtc_manager_t *rtcP, enum alarm_pos_e alarm, void (*funcP)(void *objP), void *objP);
  
bool rtcReadRegisters(rtc_manager_t *rtcP, uint8_t respData[]);

void rtcPoll(rtc_manager_t *rtcP);

#endif /* RTC_H_ */
