/*
 * alarm.h
 *
 * Created: 6/29/2019 8:53:50 PM
 *  Author: Davo Pleteau
 */ 

#ifndef ALARM_H_
#define ALARM_H_

/************************************************************************/
/*							Includes/Constants	 	                    */
/************************************************************************/
#include "stdint.h"
#include "ds3231_regs_and_utils.h"
#define MAX_ALARM_TIME_UNITS	(A2_MIN_ADDR-A1_SEC_ADDR)	 
#define TOTAL_ALARM1_REGISTERS	(MAX_ALARM_TIME_UNITS) /* 4 */
#define TOTAL_ALARM2_REGISTERS	(RTC_CTRL_ADDR-A2_MIN_ADDR) /* 3 */
/************************************************************************/
/*							Enums Definition		 	                */
/************************************************************************/
enum alarm_match_options_e 
{
	A1_MATCH_ONCE_PER_SEC = 15, /* 1111 */
	A1_MATCH_SEC = 14, //01110
	A1_MATCH_MIN_SEC = 12, //01100
	A1_MATCH_HR_MIN_SEC = 8, //01000 1
	A1_MATCH_DT_HR_MIN_SEC = 0,//0 0000
	A1_MATCH_DY_HR_MIN_SEC = 16,//1 0000
	A2_MATCH_ONCE_PER_MIN = 7, //0111
	A2_MATCH_MIN = 6, //0110
	A2_MATCH_HR_MIN = 4,//0100
	A2_MATCH_DT_HR_MIN = 0,//0000
	A2_MATCH_DY_HR_MIN = 8 //1000
};
/************************************************************************/
/*							Struct Definition							*/
/************************************************************************/
typedef struct alarm_callback_s
{
	void *objP;
	void (*alarmOnCB)(void *objP);
} alarm_callback_t;

typedef struct alarm_s
{
	uint8_t time[MAX_ALARM_TIME_UNITS];
	enum alarm_match_options_e matchFlag;
	alarm_callback_t alarmCB;
} alarm_t;

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
void alarmInit(alarm_t *alarmP);
void alarmStoreTime(alarm_t *alarmP, uint8_t *time);
void alarmSetCB(alarm_t *alarmP, void (*funcP)(void *objP), void *objP);
void alarmExecuteCB(alarm_t *alarmP);
#endif /* ALARM_H_ */