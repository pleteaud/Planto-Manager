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
#include "LCD.h"
#include "stdio.h"
#include "port.h"


#define INTCN_PIN		PINC	
#define INTCN_PIN_NUM	PINC3


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
/* Custom Pattern byte for clock */
unsigned char clockSymbol[] = {0x0,0xe,0x15,0x17,0x11,0xe,0x0,0x00};
uint8_t clkSymLoc = 0;
unsigned char calendarSymbol[] = {0x00,0x11,0x1F,0x13,0x1F,0x1F,0x00,0x00};
uint8_t calSymLoc = 1;

/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/
static void configTime(uint8_t *time, uint8_t totTimeUnits);
static void rtcAddError(rtc_manager_t *rtcP, enum rtc_errors_e error);
static uint32_t configMatchingConditions(enum alarm_match_options_e matchFlag, enum alarm_pos_e pos);
static bool verifyAxData(uint8_t *dataP, uint8_t *axTime, uint8_t len);
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
	/* Build Clock Symbol */
	lcdBuildSym(clkSymLoc,clockSymbol);
	/* Print symbol */
	lcdSetDDRAMAdrr(0,0);
	lcdWriteSymbol(clkSymLoc);
	/* Build Calendar Symbol */
	lcdBuildSym(calSymLoc,calendarSymbol);
	/* Print Symbol */
	lcdSetDDRAMAdrr(0,7);
	lcdWriteSymbol(calSymLoc);
	/* Print initial time */
	printTime();
}
void rtcSetAxCB(rtc_manager_t *rtcP,enum alarm_pos_e pos, void (*funcP)(void *objP), void *objP)
{
	if(pos == ALARM_1) 
		alarmSetCB(&rtcP->alarm1,funcP,objP);
	else 
		alarmSetCB(&rtcP->alarm2,funcP,objP);
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

/* Polling function to navigate RTC object states and to execute corresponding actions */ 
void rtcPoll(void)
{
	rtc_manager_t *rtcP = retrieveActiveRTC();
	switch (rtcP->state)
	{
		case RTC_IDLE:
			/* Check if INTCN was pulled low */
			if (!(INTCN_PIN & (1<<INTCN_PIN_NUM)) && (rtcMan.ctrlReg & AI1E_FLAG || rtcMan.ctrlReg & AI2E_FLAG)) {rtcReadRegisters(rtcP);}
			break;
		case RTC_SETTING_A1:
		case RTC_SETTING_A2:
		case RTC_SETTING_CR:
		case RTC_SETTING_SR:
		case RTC_SETTING_TIME:
			/* Check if command is over */
			if(cmdPocRtnIsReady())
			{
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
					if (!verifyAxData(dataP + A1_SEC_ADDR,rtcP->alarm1.time, TOTAL_ALARM1_REGISTERS)) {rtcAddError(rtcP,INCONSISTENT_A1);}
				}
				/* Verify Alarm2 registers if it's set */
				if(rtcP->ctrlReg & AI2E_FLAG)
				{
					if (!verifyAxData(dataP + A2_MIN_ADDR,&rtcP->alarm2.time[1],TOTAL_ALARM2_REGISTERS)) {rtcAddError(rtcP,INCONSISTENT_A2);}
				}
				
				/* Check Status register and clear respective bits */
				rtcP->ctrlStatReg = *(dataP + RTC_CTRL_STAT_ADDR);
				if (rtcP->ctrlStatReg & OSC_FLAG)
				{
					/* Report error: OSC should normally be 0. */
					rtcAddError(rtcP, OSC_STOP);
				}
				if ((rtcP->ctrlStatReg & A1I_FLAG) && (rtcP->ctrlReg & AI1E_FLAG))
				{
					/* Execute A1 callback */
					alarmExecuteCB(&rtcP->alarm1);
				}
				if ((rtcP->ctrlStatReg & A2I_FLAG) && (rtcP->ctrlReg & AI2E_FLAG))
				{
					/* Execute A2 callback which is to Update LCD Screen */ 
					alarmExecuteCB(&rtcP->alarm2);
				}
				
				/* Clear buffer for next command */
				bufferClear(retrieveActiveBuffer());
				/* Clear CMD complete flag and clear flags */
				rtcSetStatReg(rtcP,0);
				/* Print Time */
				printTime();
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
	status = true;
	/* Set appropriate state */
	rtcP->state = RTC_SETTING_TIME;
	
	/* Configure Time for DS3231  */
	configTime(time, TIME_UNITS_TOTAL);
	
	/* Configure command header + payload */
	cmd_hdr_t *cmdHdr;
	cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(set_curr_time_cmd_t));
	cmdHdr->type = SET_CURRENT_TIME;
	cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(set_curr_time_cmd_t);
	
	/* Store address pointer of first register of DS3231 before sending payload */
	cmdHdr->payload[0] = RTC_SEC_ADDR;
	for (int i = 0; i < TIME_UNITS_TOTAL; i++){*(cmdHdr->payload + START_ADDRESS_OFFSET + i) = time[i];}
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
	
	/* Send command to set control register */
	cmd_hdr_t *cmdHdr;
	cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(set_sat_reg_cmd_t));
	cmdHdr->type = SET_STAT_REG;
	cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(set_sat_reg_cmd_t);
	cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
	
	/* Store address pointer of control register */
	cmdHdr->payload[0] = RTC_CTRL_STAT_ADDR;
	cmdHdr->payload[1] = newStatReg;
	return true;
}

/* Initiate the command to set RTC alarm(1or2) registers */
bool rtcSetAlarm(rtc_manager_t *rtcP, enum alarm_pos_e pos, uint8_t *time, enum alarm_match_options_e matchFlag)
{
	bool status = false;
	/* Request to send new command */
	if(!cmdProcCtrlRequestNewCmd()) {return status;}
		
	uint32_t matchBits = 0;
	/* Set up cmd header */
	cmd_hdr_t *cmdHdr;
	if (pos == ALARM_2)
	{
		rtcP->state = RTC_SETTING_A2;
		/* Configure command specifications */
		cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(set_alarm2_time_cmd_t));
		cmdHdr->type = SET_ALARM2_TIME;
		cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(set_alarm2_time_cmd_t);
		
		/* Store address pointer of alarm 2 before the time */
		cmdHdr->payload[0] = A2_MIN_ADDR;
		/* Config time bits */
		configTime(time,TOTAL_ALARM2_REGISTERS);
		alarmStoreTime(&rtcP->alarm2, time);
		matchBits = configMatchingConditions(matchFlag,ALARM_2);
		int len = TOTAL_ALARM2_REGISTERS - 1; /* Used to extract each byte from matchBits in the following loop*/
		for (int i = 0; i < TOTAL_ALARM2_REGISTERS; i++){ rtcP->alarm2.time[1+i] |= matchBits >> (len-i) * BYTE_BIT_COUNT; /*We start at second index of time because Alarm 2 doesn't have seconds (first index)*/}
		rtcP->alarm2.matchFlag = matchFlag;
		/* Load time into cmd payload */
		for (int i = 0; i < TOTAL_ALARM2_REGISTERS; i++){*(cmdHdr->payload + START_ADDRESS_OFFSET + i) = rtcP->alarm2.time[1+i];}
		/* Calculate Checksum */
		cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
		status = true;
	}
	else
	{
		rtcP->state = RTC_SETTING_A1;
		/* Configure command specifications */
		cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(set_alarm1_time_cmd_t));
		cmdHdr->type = SET_ALARM1_TIME;
		cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(set_alarm1_time_cmd_t);
		/* Store address pointer of alarm 1 before the time */
		cmdHdr->payload[0] = A1_SEC_ADDR;
		/* Config time bits */
		configTime(time,TOTAL_ALARM1_REGISTERS);
		alarmStoreTime(&rtcP->alarm1, time);
		matchBits = configMatchingConditions(matchFlag,ALARM_1);
		int len = TOTAL_ALARM1_REGISTERS - 1; /* Used to extract each byte from matchBits in the following loop*/
		for (int i = 0; i < TOTAL_ALARM1_REGISTERS; i++) { rtcP->alarm1.time[i] |= matchBits >> (len-i) * BYTE_BIT_COUNT;}
		rtcP->alarm1.matchFlag = matchFlag;
		/* Load time into cmd payload */
		for (int i = 0; i < TOTAL_ALARM1_REGISTERS; i++){*(cmdHdr->payload + START_ADDRESS_OFFSET + i) = rtcP->alarm1.time[i];}
		/* Calculate Checksum */
		cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
		status = true;
	}
	return status;
}

/* Print time on LCD */
void printTime(void)
{
	
	/* uint8_t space; used for debugging. e.g space = snprintf(buff,20,"%x", time) */
	/* Set cursor to (0,1) to Set Hour:Min in LCD */
	lcdSetDDRAMAdrr(0,1);
	char buff[20];
	for (int i=TIME_UNITS_HR; i>TIME_UNITS_SEC; i--)
	{
		snprintf(buff,20,"%x",rtcMan.time[i]);
		if(rtcMan.time[i] < 10){ lcdWriteString("0");}
		lcdWriteString(buff);
		if (i!=TIME_UNITS_SEC+1) {lcdWriteString(":");}
	}
	/* Set cursor to (0,7) to Set Date/month/year in LCD */
	lcdSetDDRAMAdrr(0,8);
	if ((rtcMan.time[TIME_UNITS_MO_CEN] & 0x1F) < 0x09)
	{
		lcdWriteString("0");
	}
	snprintf(buff,20,"%x/", rtcMan.time[TIME_UNITS_MO_CEN] & 0x1F);
	lcdWriteString(buff);
	snprintf(buff,20,"%x/", rtcMan.time[TIME_UNITS_DT]);
	lcdWriteString(buff);
	snprintf(buff,20,"%x", rtcMan.time[TIME_UNITS_YR]);
	lcdWriteString(buff);
	/* Return Cursor Home */
	lcdReturnHome();
}

/************************************************************************/
/*					Private Functions Implementation			        */
/************************************************************************/
/* Configures the time bits in a way that satisfies the DS3231 registers */
static void configTime(uint8_t *time, uint8_t totTimeUnits)
{
	/*int len; Commented out cause this is only used for debugging purposes. e.g len = snprintf();*/
	char c[10]; /* Random size that enough to fill any size time interger */
	char *ptr; //Not really used.
	for (int i = 0; i < totTimeUnits; i++)
	{
		snprintf(c,10,"%d",time[i]);
		time[i] = strtol(c,&ptr,16); /* Hex conversion of value */
	}
}

/* Add error to the error tracker in RTC object */
static void rtcAddError(rtc_manager_t *rtcP, enum rtc_errors_e error)
{	
	rtcP->errorList[rtcP->errorCount++] = error; 	
}

/* Configure the matching condition bits in the outgoing payload */ // change comment a bit idk
static uint32_t configMatchingConditions(enum alarm_match_options_e matchFlag, enum alarm_pos_e pos)
{
	uint32_t timeBits = 0;
	if (pos == ALARM_1)
	{
		switch(matchFlag)
		{
			case A1_MATCH_ONCE_PER_SEC:
			timeBits |= AxM1_FLAG | AxM2_FLAG | AxM3_FLAG | A1M4_FLAG;
			break;
			case A1_MATCH_SEC:
			timeBits |= AxM2_FLAG | AxM3_FLAG | A1M4_FLAG;
			break;
			case A1_MATCH_MIN_SEC:
			timeBits |= AxM3_FLAG | A1M4_FLAG;
			break;
			case A1_MATCH_HR_MIN_SEC:
			timeBits |= A1M4_FLAG;
			case A1_MATCH_DY_HR_MIN_SEC:
			timeBits |= DY_DT_FLAG;
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
			timeBits |= AxM2_FLAG | AxM3_FLAG | A1M4_FLAG;
			break;
			case A2_MATCH_MIN:
			timeBits |= AxM3_FLAG | A1M4_FLAG;
			break;
			case A2_MATCH_HR_MIN:
			timeBits |= A1M4_FLAG;
			break;
			case A2_MATCH_DY_HR_MIN:
			timeBits |= DY_DT_FLAG;
			break;
			case A2_MATCH_DT_HR_MIN:
			default:
			break;
		}
	}
return timeBits;
}

/* Function to check if the alarm data being sent from the ds3231 is the expected value */
static bool verifyAxData(uint8_t *dataP, uint8_t *axTime, uint8_t len)
{
	for (int i = 0; i < len; i++)
	{
		if (*(dataP + i) != *(axTime + i)) {return false;}
	}
	return true;
}

/* Check see if errors in setting CR register  */
static bool verifyCtrlReg(uint8_t *dataP, uint8_t ctrlReg)
{
	uint8_t tempCtrlReg = *dataP;
	/* (ignore CONV bit) */
	if (tempCtrlReg >> BBSQW_FLAG_POS != ctrlReg >> BBSQW_FLAG_POS && tempCtrlReg << RS2_FLAG_POS != ctrlReg << RS2_FLAG_POS) {return false;}
	return true;
}


