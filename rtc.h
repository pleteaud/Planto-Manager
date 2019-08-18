/*
 * rtc.h
 *
 * Created: 7/14/2019 2:42:38 PM
 *  Author: plete
 */ 


#ifndef RTC_H_
#define RTC_H_

/************************************************************************/
/*							Includes	                                */
/************************************************************************/
#include "stdbool.h"
#include "stdint.h"

/************************************************************************/
/*							Constants						            */
/************************************************************************/
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
	MON = 0, 
	TUES, 
	WED, 
	THURS, 
	FRI, 
	SAT, 
	SUN
};
enum rtc_state_e {RTC_SET = 0, RTC_UNSET};

/************************************************************************/
/*				Type Defs + Struct Declaration							*/
/************************************************************************/
typedef struct rtc_manager_s rtc_manager_t;

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
rtc_manager_t* retrieveActiveRTC(void);
void rtcInit(rtc_manager_t *rtcP, uint8_t ctrlRegConfigBits, uint8_t statusConfigBits);
void rtcEnableOsc(rtc_manager_t *rtcP);
void rtcEnableBBSQW(rtc_manager_t *rtcP);
void rtcDisableOsc(rtc_manager_t *rtcP);
void rtcDisableBBSQW(rtc_manager_t *rtcP);
void rtcConvTemp(rtc_manager_t *rtcP);
void rtcSelectRate(rtc_manager_t *rtcP, bool rate1, bool rate2);
void rtcEnableAlarmInt(rtc_manager_t *rtcP);
void rtcEnableA1Int(rtc_manager_t *rtcP);
void rtcEnableA2Int(rtc_manager_t *rtcP);
void rtcDisableAlarmInt(rtc_manager_t *rtcP);
void rtcEnableA1Int(rtc_manager_t *rtcP);
void rtcEnableA2Int(rtc_manager_t *rtcP);
void rtcUpdateStatus(void);
void rtcPoll(void);
bool rtcSetTime(rtc_manager_t *rtcP, uint8_t *time);
void rtcUpdateSREG(rtc_manager_t *rtcP, uint8_t newSREG);
bool rtcSetCtrlReg();
uint8_t rtcGetSREG(rtc_manager_t *rtcP);
#endif /* RTC_H_ */
