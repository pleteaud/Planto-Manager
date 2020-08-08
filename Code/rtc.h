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

enum rtc_state_e 
{
	RTC_IDLE = 0, 
	RTC_SETTING_CR, 
	RTC_SETTING_A1, 
	RTC_SETTING_A2, 
	RTC_SETTING_SR,
	RTC_SETTING_TIME,
	RTC_READING_ALL_REGS
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
/*				Type Defs + Struct Declaration							*/
/************************************************************************/
typedef struct rtc_manager_s rtc_manager_t;

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
rtc_manager_t* retrieveActiveRTC(void);
void rtcInit();
bool rtcIsFree(rtc_manager_t *rtcP);
uint8_t rtcGetCtrlReg(rtc_manager_t *rtcP);
alarm_t *rtcGetAlarm(rtc_manager_t *rtcP, enum alarm_pos_e pos);
void AxInterruptCB(void);
void rtcPoll(void);
bool rtcReadRegisters(rtc_manager_t *rtcP);
bool rtcSetTime(rtc_manager_t *rtcP, uint8_t *time);
bool rtcSetCtrlReg(rtc_manager_t *rtcP,uint8_t newCtrlReg);
bool rtcSetStatReg(rtc_manager_t *rtcP,uint8_t newStatReg);
bool rtcSetAlarm(rtc_manager_t *rtcP, enum alarm_pos_e pos, uint8_t *time, enum alarm_match_options_e matchFlag);
#endif /* RTC_H_ */
