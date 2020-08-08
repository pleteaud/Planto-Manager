/*
 * rtc.c
 *
 * Created: 7/14/2019 3:49:15 PM
 *  Author: plete
 */ 

/************************************************************************/
/*                     Includes/Constants                               */
/************************************************************************/
#include "rtc.h"
#include "ds3231_regs_and_utils.h"
#include "cmd_structure.h"
#include "buffer.h"
#include "cmd_proc.h"
#include <avr/io.h>
#include "timer.h"



/************************************************************************/
/*                     Struct Implementation                            */
/************************************************************************/
struct rtc_manager_s 
{
	enum rtc_state_e state;
	uint8_t time[TIME_UNITS_TOTAL];
	alarm_t alarm1;
	alarm_t alarm2;
	uint8_t ctrlReg;
	uint8_t ctrlStatReg;
	uint8_t agingOffsetReg;
	uint16_t tempReg;
	uint8_t errorCount;
	uint8_t errorList[256];
};

/************************************************************************/
/*                      Private Variables                               */
/************************************************************************/
static rtc_manager_t rtcMan;
static bool AxIntTrig = false;

/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/
static void getDigits(uint8_t num, uint8_t *digitP);
static uint64_t configTime(uint8_t *time, uint8_t totTimeUnits);
static void insertTimeInPayload(uint64_t configTime, uint8_t *payload, uint8_t numTimeUnits);
static void rtcAddError(rtc_manager_t *rtcP, enum rtc_errors_e error);
static void configMatchingConditions(uint32_t *timeBits, enum alarm_match_options_e matchFlag, enum alarm_pos_e pos);
static bool verifyAxData(uint8_t *dataP, uint8_t *axTime, enum alarm_match_options_e, enum alarm_pos_e pos);
static bool verifyCtrlReg(uint8_t *dataP, uint8_t ctrlReg);
/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/
/* Retrieve active RTC manager */
rtc_manager_t* retrieveActiveRTC(void)
{
	return &rtcMan;
}

/* Initialize an rtc_manager_t object's data members with the exception of time */
void rtcInit()
{
	/* Initial value of Ctrl Reg 00x00100 */
	rtcMan.ctrlReg |= INTCN_FLAG; 
	/*Initial State of the Status Registers should be 0 */
	rtcMan.ctrlStatReg = rtcMan.agingOffsetReg = rtcMan.tempReg = rtcMan.errorCount = 0;
	rtcMan.state = RTC_IDLE;
	/* Initialize Alarm Objects */
	alarmInit(&rtcMan.alarm1);
	rtcMan.alarm1.matchFlag = A1_MATCH_DT_HR_MIN_SEC; /* 0 */
	alarmInit(&rtcMan.alarm2);
	rtcMan.alarm2.matchFlag = A2_MATCH_DT_HR_MIN; /* 0 */
	rtcMan.errorList[256]= 0;
	
	
}

/* Indicate whether RTC is free and ready to be used */
bool rtcIsFree(rtc_manager_t *rtcP)
{
	bool status;
	status = (rtcP->state == RTC_IDLE) ? true : false;
	return status;
}

/* Retrieve Control Register of RTC object*/
uint8_t rtcGetCtrlReg(rtc_manager_t *rtcP)
{
	return rtcP->ctrlReg;
}

/* Retrieve Alarm of RTC object*/
alarm_t* rtcGetAlarm(rtc_manager_t *rtcP, enum alarm_pos_e pos)
{
	return (pos == ALARM_1) ? &rtcP->alarm1 : &rtcP->alarm2; 
}
/* Callback function to indicate A1 or A2 flag has been triggered */
void AxInterruptCB(void)
{
	/* Disable External interrupt */
	EIMSK &= ~(1 << INT0);
	AxIntTrig = true;
}

/* Polling function to navigate RTC object states and to execute corresponding actions */ 
void rtcPoll(void)
{
	rtc_manager_t *rtcP = retrieveActiveRTC();
	switch (rtcP->state)
	{
		case RTC_IDLE:
			/* Check if Alarm Interrupt has been triggered */
			if (AxIntTrig)
			{
				rtcReadRegisters(rtcP);
				/* Clear Interrupt flag */
				AxIntTrig = false;
			}
			break;
		
		case RTC_SETTING_A1:
		case RTC_SETTING_A2:
		case RTC_SETTING_CR:
		case RTC_SETTING_SR:
		case RTC_SETTING_TIME:
			/* Check if command is over */
			if(cmdPocRtnIsReady())
			{
				/* Renable External Interrupts */
				if (rtcP->ctrlReg & INTCN_FLAG)
				{
					EIMSK = 1 << INT0;
				}
				/* Clear buffer for next command */
				bufferClear(retrieveActiveBuffer());
				/* Change State to IDLE */
				rtcP->state = RTC_IDLE;
				/* We'll let READ Register command state handle verifying cmd has been successfully executed */
			}
			break;
		case RTC_READING_ALL_REGS:
			if(cmdPocRtnIsReady())
			{
				/* Update RTC time array */
				uint8_t *dataP = bufferGetData(retrieveActiveBuffer(), sizeof(read_all_regs_resp_s));
				for(int i = 0; i < TIME_UNITS_TOTAL; i++){rtcP->time[i] = *(dataP + i);}
				/* Check see if errors in setting CR register */
				if(!verifyCtrlReg(dataP + RTC_CTRL_ADDR,rtcP->ctrlReg)){rtcAddError(rtcP,INCONSISTENT_CTRL_REG);}
				
				/* Verify Alarm1 registers if it's set */
				if(rtcP->ctrlReg & AI1E_FLAG)
				{
					if (!verifyAxData(dataP + A1_SEC_ADDR,rtcP->alarm1.time,rtcP->alarm1.matchFlag,ALARM_1))
					{
					rtcAddError(rtcP,INCONSISTENT_A1);}
				}
				/* Verify Alarm2 registers if it's set */
				if(rtcP->ctrlReg & AI2E_FLAG)
				{
					if (!verifyAxData(dataP + A2_MIN_ADDR,rtcP->alarm2.time,rtcP->alarm2.matchFlag,ALARM_2)){
					rtcAddError(rtcP,INCONSISTENT_A2);
					}
				}
				
				/* Check Status register and clear respective bits */
				rtcP->ctrlStatReg = *(dataP + RTC_CTRL_STAT_ADDR);
				uint8_t newSReg = 7; /* 00000111 */
				if (rtcP->ctrlStatReg & OSC_FLAG)
				{
					/* Report error: OSC should normally be 0. */
					rtcAddError(rtcP, OSC_STOP);
				}
				if (rtcP->ctrlStatReg & A1I_FLAG && (rtcP->ctrlReg & AI1E_FLAG))
				{
					/* Execute A1 callback */
					rtcP->alarm1.alarmCB.alarmOnCB(rtcP->alarm1.alarmCB.objP);
					/* Clear Alarm 1 bit */
					newSReg = newSReg & ~(A1I_FLAG);
				}
				if (rtcP->ctrlStatReg & A2I_FLAG && (rtcP->ctrlReg & AI2E_FLAG))
				{
					/* Execute A2 callback which is to Update LCD Screen */ 
					rtcP->alarm2.alarmCB.alarmOnCB(rtcP->alarm2.alarmCB.objP);
					/* Clear Alarm 2 bit */
					newSReg = newSReg & ~(A2I_FLAG);
				}
				
				/* Clear buffer for next command */
				bufferClear(retrieveActiveBuffer());
				/* Clear CMD complete flag and set new status */
				rtcSetStatReg(rtcP,newSReg);
			}
			break;
		default:
			break;	
	}
}

/* Initiate a command to read all DS3231 registers */
bool rtcReadRegisters(rtc_manager_t *rtcP)
{
	bool status = false;
	/* Request to send new command */
	if(!cmdProcCtrlRequestNewCmd())
	{
		return status;
	}
	status = true;
	/* Set appropriate state */
	rtcP->state = RTC_READING_ALL_REGS;
	
	/* Configure command header + payload */
	cmd_hdr_t *cmdHdr;
	cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(read_all_regs_cmd_s));
	cmdHdr->type = READ_ALL_REGS;
	cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(read_all_regs_cmd_s);
	
	/* Store address pointer of first register of DS3231 before sending payload */
	cmdHdr->payload[0] = RTC_SEC_ADDR;
	cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
	return status;
}

/* Initiate the command to set the RTC time (sec, min, hr, day, date, month/century, year) */
bool rtcSetTime(rtc_manager_t *rtcP, uint8_t *time)
{
	bool status = false;
	/* Request to send new command */
	if(!cmdProcCtrlRequestNewCmd())
	{
		return status;
	}
	/* Set appropriate state */
	rtcP->state = RTC_SETTING_TIME;
	/* Store values inside array of time */
	for (int i = TIME_UNITS_SEC; i < TIME_UNITS_TOTAL; i++)
	{
		rtcP->time[i] = *(time + i);
	}
	
	/* Configure Time for DS3231  */
	uint64_t timebits = 0;
	timebits = configTime(rtcP->time, TIME_UNITS_TOTAL);
	status = true;
	
	/* Configure command header + payload */
	cmd_hdr_t *cmdHdr;
	cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(set_curr_time_cmd_t));
	cmdHdr->type = SET_CURRENT_TIME;
	cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(set_curr_time_cmd_t);
	
	/* Store address pointer of first register of DS3231 before sending payload */
	cmdHdr->payload[0] = RTC_SEC_ADDR;
	
	insertTimeInPayload(timebits,cmdHdr->payload + START_ADDRESS_OFFSET, TIME_UNITS_TOTAL);
	/* Calculate Checksum */
	cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
	
	return status;
}

/* Initiate the command to set the RTC control register */
bool rtcSetCtrlReg(rtc_manager_t *rtcP,uint8_t newCtrlReg)
{	
	/* Check if there's an ongoing command */
	if(!cmdProcCtrlRequestNewCmd())
	{
		return false;
	}
	/* Set appropriate state */
	rtcP->state = RTC_SETTING_CR;
	
	/* Configure RTC Control */
	rtcP->ctrlReg = newCtrlReg;
	
	/* Send command to set control register */
	cmd_hdr_t *cmdHdr;
	cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(set_ctrl_reg_cmd_t));
	cmdHdr->type = SET_CONTROL_REG;
	cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(set_ctrl_reg_cmd_t);
	cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
	
	/* Store address pointer of control register */
	cmdHdr->payload[0] = RTC_CTRL_ADDR;
	cmdHdr->payload[1] = rtcP->ctrlReg;
	return true;
}

/* Initiate the command to set the RTC status register */
bool rtcSetStatReg(rtc_manager_t *rtcP,uint8_t newStatReg)
{
	/* Check if there's an ongoing command */
	if(!cmdProcCtrlRequestNewCmd())
	{
		return false;
	}
	/* Set appropriate state */
	rtcP->state = RTC_SETTING_SR;
	
	/* Configure RTC Control */
	rtcP->ctrlStatReg = newStatReg;
	
	/* Send command to set control register */
	cmd_hdr_t *cmdHdr;
	cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(set_sat_reg_cmd_t));
	cmdHdr->type = SET_STAT_REG;
	cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(set_sat_reg_cmd_t);
	cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
	
	/* Store address pointer of control register */
	cmdHdr->payload[0] = RTC_CTRL_STAT_ADDR;
	cmdHdr->payload[1] = rtcP->ctrlStatReg;
	return true;
}

/* Initiate the command to set RTC alarm(1or2) registers */
bool rtcSetAlarm(rtc_manager_t *rtcP, enum alarm_pos_e pos, uint8_t *time, enum alarm_match_options_e matchFlag)
{
	bool status = false;
	/* Request to send new command */
	if(!cmdProcCtrlRequestNewCmd()) {return status;}
		
	uint32_t timebits = 0;
	/* Set up temporary cmd header to send to send to cmdPoll */
	cmd_hdr_t *cmdHdr;
	if (pos == ALARM_2)
	{
		rtcP->state = RTC_SETTING_A2;
		alarmStoreTime(&rtcP->alarm2, time);
		/* Configure command specifications */
		cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(set_alarm2_time_cmd_t));
		cmdHdr->type = SET_ALARM2_TIME;
		cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(set_alarm2_time_cmd_t);
		/* Store address pointer of alarm 2 before the time */
		cmdHdr->payload[0] = A2_MIN_ADDR;
		/* Config time bits */
		timebits |= configTime(rtcP->alarm2.time, MAX_ALARM_TIME_UNITS);
		configMatchingConditions(&timebits, matchFlag, ALARM_2);
		rtcP->alarm2.matchFlag = matchFlag;
		/* Load time into cmd payload */
		insertTimeInPayload(timebits, cmdHdr->payload + START_ADDRESS_OFFSET, TOTAL_ALARM2_REGISTERS); /* Alarm 2 only has 3 registers */
		/* Calculate Checksum */
		cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
		status = true;
	}
	else
	{
		rtcP->state = RTC_SETTING_A1;
		alarmStoreTime(&rtcP->alarm1, time);
		/* Configure command specifications */
		cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(set_alarm1_time_cmd_t));
		cmdHdr->type = SET_ALARM1_TIME;
		cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(set_alarm1_time_cmd_t);
		/* Store address pointer of alarm 1 before the time */
		cmdHdr->payload[0] = A1_SEC_ADDR;
		/* Config time bits */
		timebits = configTime(rtcP->alarm1.time, MAX_ALARM_TIME_UNITS);
		configMatchingConditions(&timebits, matchFlag, ALARM_1);
		rtcP->alarm1.matchFlag = matchFlag;
		/* Load time into cmd payload */
		insertTimeInPayload(timebits,cmdHdr->payload + START_ADDRESS_OFFSET, TOTAL_ALARM1_REGISTERS);
		/* Calculate Checksum */
		cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
		status = true;
	}
	return status;
}


/************************************************************************/
/*					Private Functions Implementation			        */
/************************************************************************/
/* Extract the 2 digits values from a given number for DS3231 */
/* Was hoping to make this a function that can do a defined number of digits, */
/* But unfortunately it's not wise to dynamically create arrays in embedded devices */
static void getDigits(uint8_t num, uint8_t *digitP)
{
	/* Make an array to hold for the digits (in this case 2)*/
	uint8_t digitArr[DIGITS_PER_TIME_UNIT] = {0};
	/* Start extracting digits */
	int arrIndex = 0;
	while(num > 0)
	{
		digitArr[arrIndex]= num % 10;
		num /= 10;
		arrIndex++;
	}
	/* Must reverse elements to get correct digit order */
	arrIndex = DIGITS_PER_TIME_UNIT - 1; /* Reinitialize array index to avoid going out of range of tempDigArr. */
	for (int j = 0; j < DIGITS_PER_TIME_UNIT; j++) {digitP[j] = digitArr[arrIndex--];}
}

/* Configures the time bits in a way that satisfies the DS3231 registers */
static uint64_t configTime(uint8_t *time, uint8_t totTimeUnits)
{
	/* Allocate an array where each element represent a digit of time data: */
	/* seconds(0-59) / minutes (0-59)/ hours(0-23 or 0-12)/ day(1-7)/ date(1-31)/ month(century) (1-12)/ year (00-99)} */
	uint8_t digits[TIME_UNITS_TOTAL*DIGITS_PER_TIME_UNIT] = {0};

	int digitsIndex = 0;
	/* Extract and Store digits inside digits array */
	/* totTimeUnits is set by higher object. Currently it can equal 7 (setting all rtc time) or 4 (setting alarm 1) or 3 (setting alarm 2) */
	for (int i = 0; i < totTimeUnits; i++)
	{
		getDigits(*(time+i), (digits + digitsIndex));
		digitsIndex += DIGITS_PER_TIME_UNIT;
	}
	
	/* Configure time bits as so: */
	/* Seconds {MSB 8 bits} + Minutes {8 bits} + Hours {8 bits} + Day {8 bits} + Date {8 bits} + Month {8 bit} + Year {8 bit} + NULL {LSB 8 bits} */
	uint64_t timeBits = 0;
	uint64_t tempDigit = 0;
	/* bitOffSet is used to place each digit in the rightful location inside timeBits */
	int bitsOffSet = (totTimeUnits * BYTE_SHIFT) - BYTE_SHIFT; //starts at 48 because seconds byte takes up 8 bits
	
	for (int i = 0; i < totTimeUnits * DIGITS_PER_TIME_UNIT; i++)
	{
		/* if i is odd, then it's the one's place digit that needs to be added to timeBits (e.g 9 from 69) */
		if (i % DIGITS_PER_TIME_UNIT)
		{
			tempDigit |= digits[i];
			timeBits = (tempDigit << bitsOffSet) | timeBits;
			bitsOffSet -= BYTE_SHIFT;
			tempDigit = 0;
		}
		/* if i is even or 0, then it's the ten's place digit that needs to be added to timeBits (e.g 6 from 69)*/
		else {tempDigit |= digits[i] << NIBBLE_SHIFT;}
	}
	return timeBits;
}

/* Insert the configured time bit inside a payload */
static void insertTimeInPayload(uint64_t time, uint8_t *payload, uint8_t numTimeUnits)
{
	/* Offset for starting at first time unit in "time" variable. */
	uint8_t bitOffSet = (numTimeUnits * BYTE_SHIFT)-BYTE_SHIFT;
	for(int i = 0; i < numTimeUnits; i++)
	{
		*(payload + i) = time >> bitOffSet; /* sec/min/hour/day */
		bitOffSet -= BYTE_SHIFT;
	}
}

static void rtcAddError(rtc_manager_t *rtcP, enum rtc_errors_e error)
{	
	rtcP->errorList[rtcP->errorCount++] = error; 	
}

/* Configure the matching condition bits in the outgoing payload */ // change comment a bit idk
static void configMatchingConditions(uint32_t *timeBits, enum alarm_match_options_e matchFlag, enum alarm_pos_e pos)
{
	if (pos == ALARM_1)
	{
		switch(matchFlag)
		{
			case A1_MATCH_ONCE_PER_SEC:
			*timeBits |= AxM1_FLAG | AxM2_FLAG | AxM3_FLAG | A1M4_FLAG;
			break;
			case A1_MATCH_SEC:
			*timeBits |= AxM2_FLAG | AxM3_FLAG | A1M4_FLAG;
			break;
			case A1_MATCH_MIN_SEC:
			*timeBits |= AxM3_FLAG | A1M4_FLAG;
			break;
			case A1_MATCH_HR_MIN_SEC:
			*timeBits |= A1M4_FLAG;
			case A1_MATCH_DY_HR_MIN_SEC:
			*timeBits |= DY_DT_FLAG;
			break;
			case A1_MATCH_DT_HR_MIN_SEC:
			default:
			break;
		}
	}
	else
	{
		switch(matchFlag)
		{	
			case A2_MATCH_ONCE_PER_MIN:
				*timeBits |= AxM2_FLAG | AxM3_FLAG | A1M4_FLAG;
				break;
			case A2_MATCH_MIN:
				*timeBits |= AxM3_FLAG | A1M4_FLAG;
				break;
			case A2_MATCH_HR_MIN:
				*timeBits |= A1M4_FLAG;
				break;
			case A2_MATCH_DY_HR_MIN:
				*timeBits |= DY_DT_FLAG;
				break;
			case A2_MATCH_DT_HR_MIN:
			default:
				break;
		}
	}
}

static bool verifyAxData(uint8_t *dataP, uint8_t *axTime, enum alarm_match_options_e matchFlag, enum alarm_pos_e pos)
{
	uint8_t axMask = 0;
	bool status = false;
	if (pos == ALARM_1)
	{
		for (int j = 0; j <= A1_DY_DT_ADDR-A1_SEC_ADDR; j++)
		{
			/* Extract the alarm time that's currently stored inside the DS3231 */
			uint8_t alarmTime = *(dataP+j);
			axMask |= (alarmTime & A1M4_FLAG) >> (A1M4_FLAG_POS-j);
			if(axMask == 0 && j == A1_DY_DT_ADDR-A1_SEC_ADDR){ axMask |= (alarmTime & DY_DT_FLAG) >> (DY_DT_FLAG_POS-4);} /* Shift bit two places over */
			uint8_t shiftedRXAlarmTime = alarmTime << 1;
			uint8_t shiftedSetAlarmTime = axTime[j] << 1;
			if(shiftedRXAlarmTime != shiftedSetAlarmTime)
			{
				return status;
			}
		}
	}
	else
	{
		int i = 0; /* index of Alarm 2 minutes */
		for (int j = 0; j <= A2_DY_DT_ADDR-A2_MIN_ADDR; j++)
		{
			uint8_t alarmTime = *(dataP+j);
			axMask |= (alarmTime & A1M4_FLAG) >> (A1M4_FLAG_POS-j);
			if(axMask == 0 && j == A2_DY_DT_ADDR-A2_MIN_ADDR){ axMask |= (alarmTime & DY_DT_FLAG) >> (DY_DT_FLAG_POS-3);} /* Shift bit 3 places over */
			uint8_t shiftedRXAlarmTime = alarmTime << 1;
			uint8_t shiftedSetAlarmTime = axTime[j+1] << 1;
			if(shiftedRXAlarmTime != shiftedSetAlarmTime)
			{ 
				return status;
			} /* start checking at element i=1 cause Alarm2 doesn't have a seconds register */
		}
	}
	if (axMask != matchFlag){ 
		return status;}
	/* no errors occured */
	status = true;
	return status;
}

/* Check see if errors in setting CR register  */
static bool verifyCtrlReg(uint8_t *dataP, uint8_t ctrlReg)
{
	uint8_t tempCtrlReg = *dataP;
	/* (ignore CONV bit) */
	if (tempCtrlReg >> BBSQW_FLAG_POS != ctrlReg >> BBSQW_FLAG_POS && tempCtrlReg << RS2_FLAG_POS != ctrlReg << RS2_FLAG_POS) {return false;}
	return true;
}