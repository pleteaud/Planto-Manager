/*
 * ds3231.h
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

// Time units
enum time_units_e
{
	TIME_UNITS_SEC = 0, TIME_UNITS_MIN, TIME_UNITS_HR, TIME_UNITS_DY, TIME_UNITS_DT,
	TIME_UNITS_MO_CEN, 	TIME_UNITS_YR, TIME_UNITS_TOTAL
};

typedef enum days_e 
{
	MON = 1, TUES, WED,	THURS, FRI, SAT, SUN
} DAYS;

typedef enum months_e 
{
	JAN = 1, FEB, MAR, APR, MAY, JUN, 
	JUL, AUG, SEP, OCT, NOV, DEC
} MONTHS;

enum ds3231_errors_e
{
	INCONSISTENT_CTRL_REG = 0,
	INCONSISTENT_STAT_REG,
	INCONSISTENT_A1,
	INCONSISTENT_A2,
	OSC_STOP
};

typedef enum alarm_pos_e
{
	ALARM_1 = 0,
	ALARM_2 = 1,
	NUM_OF_ALARMS
} ALARM_NUMB;

/************************************************************************/
/*				Type Defs + Struct Implementation						*/
/************************************************************************/
typedef struct ds3231_s 
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
} ds3231_t;

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
void ds3231Init(ds3231_t *deviceP);

bool ds3231SetSeconds(ds3231_t *deviceP, uint8_t seconds);

bool ds3231SetMinutes(ds3231_t *deviceP, uint8_t min);

bool ds3231SetHour(ds3231_t *deviceP, uint8_t hour);

bool ds3231SetDay(ds3231_t *deviceP, DAYS day);

bool ds3231SetDate(ds3231_t *deviceP, uint8_t date);

bool ds3231SetMonCen(ds3231_t *deviceP, uint8_t month, bool century);

bool ds3231SetTime(ds3231_t *deviceP, uint8_t *time);

bool ds3231SetCtrlReg(ds3231_t *deviceP, const uint8_t newCtrlReg);

bool ds3231SetStatReg(ds3231_t *deviceP, const uint8_t newStatReg);

bool ds3231SetAlarm1(ds3231_t *deviceP, uint8_t *time, const enum alarm_match_options_e matchFlag,
				 void (*funcP)(void *objP), void *objP);
				 
bool ds3231SetAlarm2(ds3231_t *deviceP, uint8_t *time, const enum alarm_match_options_e matchFlag,
				  void (*funcP)(void *objP), void *objP);

void ds3231SetAlarmCallback(ds3231_t *deviceP, const ALARM_NUMB alarm, void (*funcP)(void *objP), void *objP);
  
bool ds3231ReadRegisters(ds3231_t *deviceP, uint8_t respData[]);

void ds3231Poll(ds3231_t *deviceP);

#endif /* RTC_H_ */
