/*
 * alarm.h
 *
 * Created: 6/29/2019 8:53:50 PM
 *  Author: plete
 */ 

#ifndef ALARM_H_
#define ALARM_H_

/************************************************************************/
/*							Includes				 	                */
/************************************************************************/
#include "stdint.h"
#include "stdbool.h"
#include "rtc.h"

/************************************************************************/
/*							Constants						            */
/************************************************************************/
#define MAX_ALARM_TIME_UNITS	(TIME_UNITS_TOTAL-TIME_UNITS_DY)	 //Sec, Min, Hours, Day/Date

/************************************************************************/
/*							Enums Definition		 	                */
/************************************************************************/
enum { ALARM_UNSET = 0, ALARM_SET };
enum dt_dy_flag_e { NONE = 0, DATE, DAY};

enum a1_match_option_e
{
	A1_MATCH_ONCE_PER_SEC = 0,
	A1_MATCH_SEC,
	A1_MATCH_MIN_SEC,
	A1_MATCH_HR_MIN_SEC,
	A1_MATCH_DT_HR_MIN_SEC,
	A1_MATCH_DY_HR_MIN_SEC
};

enum a2_match_option_e
{
	A2_MATCH_ONCE_PER_MIN = 0,
	A2_MATCH_MIN,
	A2_MATCH_HR_MIN,
	A2_MATCH_DT_HR_MIN,
	A2_MATCH_DY_HR_MIN
};



/************************************************************************/
/*				Type Defs + Struct Definition							*/
/************************************************************************/
typedef struct alarm_callback_s
{
	void *objP;
	void (*alarmOnCB)(void *objP);
} alarm_callback_t;

union alarm_match_u
{
	enum a1_match_option_e a1MF;
	enum a2_match_option_e a2MF;
};

typedef struct alarm_s
{
	uint8_t time[MAX_ALARM_TIME_UNITS];
	union alarm_match_u matchFlag;
	uint8_t flag;
	alarm_callback_t alarmCB;
} alarm_t;

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
void alarmsInit(alarm_t *alarmP, uint8_t numAlarms);
bool alarmSet(alarm_t *alarmP, uint8_t *time, enum dt_dy_flag_e dayDate);
void alarmSetCB(alarm_t *alarmP, void (*funcP)(void *objP), void *objP);
void alarmsPoll(alarm_t *alarmList, uint8_t numAlarms);
void alarmIrqHandle(alarm_t *alarmList, uint8_t numAlarms);
#endif /* ALARM_H_ */