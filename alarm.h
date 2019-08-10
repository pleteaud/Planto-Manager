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

/************************************************************************/
/*							Enums Definition		 	                */
/************************************************************************/
enum alarm_state_e {ALARM_SET = 0, ALARM_UNSET};
enum alarm_registers_e {ALARM1 = 0, ALARM2};
enum days_e {NONE = 0, MON, TUES, WED, THURS, FRI, SAT, SUN};

/************************************************************************/
/*				Type Defs + Struct Definition							*/
/************************************************************************/
typedef struct alarm_callback_s
{
	void *objP;
	void (*alarmOnCB)(void *objP);
} alarm_callback_t;

typedef struct alarm_s
{
	enum days_e day;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	enum alarm_state_e state;
	alarm_callback_t alarmCB;
	uint8_t alarmPos;
} alarm_t;

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
void alarmsInit(alarm_t *alarmP, uint8_t numAlarms);
bool alarmSet(alarm_t *alarmP, enum days_e day, uint8_t hour, uint8_t minutes, uint8_t seconds);
void alarmSetCB(alarm_t *alarmP, void (*funcP)(void *objP), void *objP);
void alarmsPoll(alarm_t *alarmList, uint8_t numAlarms);
void alarmIrqHandle(alarm_t *alarmList, uint8_t numAlarms);
#endif /* ALARM_H_ */