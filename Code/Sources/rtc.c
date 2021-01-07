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
#include <avr/io.h>
#include "LCD.h"
#include "stdio.h"
#include "port.h"
#include "i2cMasterControl.h"


#define INTCN_PIN		PINC	
#define INTCN_PIN_NUM	PINC3


/* Commands data buffer sizes*/
enum cmd_sizes
{
	READ_ALL_REGS_CMD_SIZE = 1,
	SET_RTC_TIME_CMD_SIZE = 8,
	SET_RTC_SEC_CMD_SIZE = 2,
	SET_RTC_MIN_CMD_SIZE = 2,
	SET_RTC_HR_CMD_SIZE = 2,
	SET_RTC_DY_CMD_SIZE = 2,
	SET_RTC_DT_CMD_SIZE = 2,
	SET_RTC_MON_CEN_CMD_SIZE = 2,
	SET_ALARM1_TIME_CMD_SIZE = 5,
	SET_ALARM2_TIME_CMD_SIZE = 4,
	SET_CONTROL_REG_CMD_SIZE = 2,
	SET_STAT_REG_CMD_SIZE = 2,
};

/* Command response data buffer size */
enum resp_sizes
{
	READ_ALL_REGS_RESP_SIZE = 19 	
};

/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/
static void configTime(uint8_t *time, uint8_t totTimeUnits);

static void rtcAddError(rtc_manager_t *rtcP, enum rtc_errors_e error);

static uint32_t configMatchingConditions(enum alarm_match_options_e matchFlag, enum alarm_pos_e pos);

static bool verifyAxData(uint8_t *dataP, uint8_t *axTime, uint8_t len);

static bool verifyCtrlReg(uint8_t *dataP, uint8_t ctrlReg);

static void rtcUpdate(rtc_manager_t *rtcP, uint8_t regs[]);

static bool rtcSetTimeRegs(rtc_manager_t *rtcP, uint8_t cmdBuffSize, uint8_t startAddr,
						   uint8_t *time, uint8_t numTimeUnits);

/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/

/* Initialize an rtc_manager_t object's data members with the exception of time */
void rtcInit(rtc_manager_t *rtcP)
{
	/* Initial value of Ctrl Reg 00x00100 */
	rtcP->ctrlReg |= INTCN_FLAG; 
	rtcSetCtrlReg(rtcP, INTCN_FLAG);
	
	/* Initial time on power up is 01/01/00 01 00:00:00 (DD/MM/YY DOW HH:MM:SS) */
	for (uint8_t i = 0; i < TIME_UNITS_TOTAL; i++)
	{
		switch(i)
		{
			case TIME_UNITS_DY:
				rtcP->time[i] = MON;
				break;
			case TIME_UNITS_MO_CEN:
				rtcP->time[i] = JAN;
				break;
			case TIME_UNITS_DT:
				rtcP->time[i] = 1;
				break;
			default:
				rtcP->time[i] = 0;
		}
	}
	
	// Initial State of the Status Registers should be 0
	rtcP->ctrlStatReg = rtcP->agingOffsetReg = rtcP->tempReg = rtcP->errorCount = 0;
	
	/* Initialize Alarm Objects */
	alarmInit(&rtcP->alarm1);
	rtcP->alarm1.matchFlag = A1_MATCH_DT_HR_MIN_SEC; // 0
	alarmInit(&rtcP->alarm2);
	rtcP->alarm2.matchFlag = A2_MATCH_DT_HR_MIN; // 0 
	
	for (int i = 0; i < 256; i ++)
	{
		rtcP->errorList[i] = 0; // Initialize error count list
	}
}

/* Set seconds of RTC */
bool rtcSetSeconds(rtc_manager_t *rtcP, uint8_t seconds)
{	
	if (rtcSetTimeRegs(rtcP, SET_RTC_SEC_CMD_SIZE, RTC_SEC_ADDR, &seconds, 1))
	{
		rtcP->time[RTC_SEC_ADDR] = seconds; // Update rtc object 
		return true;
	}
	
	return false;
}

/* Set minutes of RTC */
bool rtcSetMinutes(rtc_manager_t *rtcP, uint8_t min)
{
	if (rtcSetTimeRegs(rtcP, SET_RTC_MIN_CMD_SIZE, RTC_MIN_ADDR, &min, 1))
	{
		rtcP->time[RTC_MIN_ADDR] = min; // Update rtc object 
		return true;
	}
	
	return false;
}

/* Set hours of RTC */
bool rtcSetHour(rtc_manager_t *rtcP, uint8_t hour)
{	
	if (rtcSetTimeRegs(rtcP, SET_RTC_HR_CMD_SIZE, RTC_HRS_ADDR, &hour, 1))
	{
		rtcP->time[RTC_HRS_ADDR] = hour; // Update rtc object 
		return true;
	}
	
	return false;
}

/* Set day of RTC */
bool rtcSetDay(rtc_manager_t *rtcP, enum days_e day)
{
	if (rtcSetTimeRegs(rtcP, SET_RTC_DY_CMD_SIZE, RTC_DY_ADDR, &day, 1))
	{
		rtcP->time[RTC_DY_ADDR] = day; // Update rtc object 
		return true;
	}
	return false;
}

/* Set date of RTC */
bool rtcSetDate(rtc_manager_t *rtcP, uint8_t date)
{
	if (rtcSetTimeRegs(rtcP, SET_RTC_DT_CMD_SIZE, RTC_DT_ADDR, &date, 1))
	{
		rtcP->time[RTC_DT_ADDR] = date; // Update rtc object 
		return true;
	}
	
	return false;
}

/* Set month and century of RTC */
bool rtcSetMonCen(rtc_manager_t *rtcP, uint8_t mon, bool century)
{
	uint8_t monCen = century << 7 | mon ; // configure century flag of month/century register
	if (rtcSetTimeRegs(rtcP, SET_RTC_MON_CEN_CMD_SIZE, RTC_M_CEN_ADDR, &monCen, 1))
	{
		rtcP->time[RTC_M_CEN_ADDR] = monCen;
		return true;
	}
	
	return false;
}

/* Set all time units of RTC object */ 
bool rtcSetTime(rtc_manager_t *rtcP, uint8_t *time)
{
	if (rtcSetTimeRegs(rtcP, SET_RTC_TIME_CMD_SIZE, RTC_SEC_ADDR, time, TIME_UNITS_TOTAL))
	{
		// If successful update time data member
		for (uint8_t i = RTC_SEC_ADDR; i < TIME_UNITS_TOTAL; i++) //startAddr will match enum value of register
			rtcP->time[i] = time[i];
		return true;
	}
	return false;
}

/* Initiate the command to set the RTC control register */
bool rtcSetCtrlReg(rtc_manager_t *rtcP,uint8_t newCtrlReg)
{
	uint8_t cmdBuffer[SET_CONTROL_REG_CMD_SIZE];
	
	
	cmdBuffer[0]  = RTC_CTRL_ADDR;	// Store address of control register in command buffer
	cmdBuffer[1]  = newCtrlReg;		// Store new control register
	
	if(i2cMasterTransmit(DS3231_SLAVE_ADDR, cmdBuffer, SET_CONTROL_REG_CMD_SIZE))
	{
		rtcP->ctrlReg = newCtrlReg; // update rtc object
		return true;
	}
	else
	return false;
}

/* Initiate the command to set the RTC status register */
bool rtcSetStatReg(rtc_manager_t *rtcP,uint8_t newStatReg)
{
	
	uint8_t cmdBuffer[SET_STAT_REG_CMD_SIZE];
	cmdBuffer[0]  = RTC_CTRL_STAT_ADDR;	// Store address of rtc seconds register in command buffer
	cmdBuffer[1]  = newStatReg;	// Store new state register
	
	if (i2cMasterTransmit(DS3231_SLAVE_ADDR, cmdBuffer, SET_STAT_REG_CMD_SIZE))
	{
		rtcP->ctrlStatReg = newStatReg; // Update rtc object
		return true;
	}
	else
	return false;
}

/* Set RTC Alarm 1 */
bool rtcSetAlarm1(rtc_manager_t *rtcP, uint8_t *time, enum alarm_match_options_e matchFlag,
			      void (*funcP)(void *objP), void *objP)
{
	uint32_t matchBits = 0;
	
	uint8_t cmdBuffer[SET_ALARM1_TIME_CMD_SIZE];
	cmdBuffer[0] = A1_SEC_ADDR; // Store addres of alarm 1 register
	
	/* Config time bits */
	configTime(time,TOTAL_ALARM1_REGISTERS);
	alarmStoreTime(&rtcP->alarm1, time);
	matchBits = configMatchingConditions(matchFlag,ALARM_1);
	
	int len = TOTAL_ALARM1_REGISTERS - 1; /* Used to extract each byte from matchBits in the following loop*/
	for (int i = 0; i < TOTAL_ALARM1_REGISTERS; i++)
	rtcP->alarm1.time[i] |= matchBits >> (len-i) * BYTE_BIT_COUNT;
	rtcP->alarm1.matchFlag = matchFlag;
	
	/* Load time into cmd payload */
	for (int i = 0; i < TOTAL_ALARM1_REGISTERS; i++)
	cmdBuffer[START_ADDRESS_OFFSET + i] = rtcP->alarm1.time[i];
	
	if (i2cMasterTransmit(DS3231_SLAVE_ADDR, cmdBuffer, SET_ALARM1_TIME_CMD_SIZE))
	{
		rtcSetAlarmCallback(rtcP, ALARM_1, funcP, objP);
		return rtcSetCtrlReg(rtcP, rtcP->ctrlReg | AI1E_FLAG);
	}
	else
	return false;
}

/* Set RTC Alarm 2 */
bool rtcSetAlarm2(rtc_manager_t *rtcP, uint8_t *time, enum alarm_match_options_e matchFlag,
				  void (*funcP)(void *objP), void *objP)
{
	uint32_t matchBits = 0;
	
	uint8_t cmdBuffer[SET_ALARM2_TIME_CMD_SIZE];
	cmdBuffer[0] = A2_MIN_ADDR;	// Store address of alarm 2 register
	
	/* Config time bits */
	configTime(time,TOTAL_ALARM2_REGISTERS);
	alarmStoreTime(&rtcP->alarm2, time);
	matchBits = configMatchingConditions(matchFlag,ALARM_2);
	
	int len = TOTAL_ALARM2_REGISTERS - 1; /* Used to extract each byte from matchBits in the following loop*/
	for (int i = 0; i < TOTAL_ALARM2_REGISTERS; i++)
	rtcP->alarm2.time[i + 1] |= matchBits >> (len - i) * BYTE_BIT_COUNT; /*We start at second index of time because Alarm 2 doesn't have seconds (first index)*/
	rtcP->alarm2.matchFlag = matchFlag;
	
	/* Load time into cmd payload */
	for (int i = 0; i < TOTAL_ALARM2_REGISTERS; i++)
	cmdBuffer[START_ADDRESS_OFFSET + i] = rtcP->alarm2.time[i + 1];
	
	if (i2cMasterTransmit(DS3231_SLAVE_ADDR, cmdBuffer, SET_ALARM2_TIME_CMD_SIZE))
	{
		
		rtcSetAlarmCallback(rtcP, ALARM_2, funcP, objP);
		return rtcSetCtrlReg(rtcP, rtcP->ctrlReg | AI2E_FLAG);
	}
	else
	return false;
}

/* Set alarm Callback */
void rtcSetAlarmCallback(rtc_manager_t *rtcP, enum alarm_pos_e alarm, void (*funcP)(void *objP), void *objP)
{
	if (alarm)
	alarmSetCB(&rtcP->alarm2, funcP, objP);	// Set alarm 2 call back
	else
	alarmSetCB(&rtcP->alarm1, funcP, objP);	// Set alarm 1 call back
}

/* Initiate a command to read all RTC registers */
bool rtcReadRegisters(rtc_manager_t *rtcP, uint8_t respData[])
{
	uint8_t cmdBuffer[READ_ALL_REGS_CMD_SIZE];

	// Store address of rtc seconds register in command buffer
	cmdBuffer[0]  = RTC_SEC_ADDR;
	
	bool status = false;
	
	// Transmit address
	if (i2cMasterTransmit(DS3231_SLAVE_ADDR, cmdBuffer, READ_ALL_REGS_CMD_SIZE))
	// Read the 19 registers of ds3231
	status = i2cMasterRead(DS3231_SLAVE_ADDR, respData, READ_ALL_REGS_RESP_SIZE);
	
	return status;
}

 /* Polling routine to update an RTC object */
 void rtcPoll(rtc_manager_t *rtcP)
 {
	 if (!(INTCN_PIN & (1<<INTCN_PIN_NUM)) && (rtcP->ctrlReg & AI1E_FLAG || rtcP->ctrlReg & AI2E_FLAG))
	 {
		  uint8_t registers[READ_ALL_REGS_RESP_SIZE];
		  // Read registers
		  rtcReadRegisters(rtcP, registers);
	  
		  // Update the RTC object and check for control/alarm registers mismatch
		  rtcUpdate(rtcP, registers);
	  
		  // Clear CMD complete flag and clear flags
		  rtcSetStatReg(rtcP,0);
	 }
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

/* Update RTC object with data read from the DS3231 and check for erros */
static void rtcUpdate(rtc_manager_t *rtcP, uint8_t regs[])
{
	/* Update RTC time array */
	for(int i = 0; i < TIME_UNITS_TOTAL; i++)
	rtcP->time[i] = regs[i];
	
	/* Check see if errors in setting CR register */
	if(!verifyCtrlReg(regs + RTC_CTRL_ADDR, rtcP->ctrlReg))
	rtcAddError(rtcP,INCONSISTENT_CTRL_REG);
	
	/* Verify Alarm1 registers if it's set */
	if(rtcP->ctrlReg & AI1E_FLAG)
	{
		if (!verifyAxData(regs + A1_SEC_ADDR, rtcP->alarm1.time, TOTAL_ALARM1_REGISTERS))
		rtcAddError(rtcP,INCONSISTENT_A1);
	}
	
	/* Verify Alarm2 registers if it's set */
	if(rtcP->ctrlReg & AI2E_FLAG)
	{
		if (!verifyAxData(regs + A2_MIN_ADDR, &rtcP->alarm2.time[1], TOTAL_ALARM2_REGISTERS))
		rtcAddError(rtcP, INCONSISTENT_A2);
	}
	
	/* Check Status register and clear respective bits */
	rtcP->ctrlStatReg = regs[RTC_CTRL_STAT_ADDR];
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
}

/* Set time of DS3231 */
static bool rtcSetTimeRegs(rtc_manager_t *rtcP, uint8_t cmdBuffSize, uint8_t startAddr,
						   uint8_t *time, uint8_t numTimeUnits)
{
	uint8_t cmdBuffer[cmdBuffSize];
	cmdBuffer[0]  = startAddr;	// Store address of rtc seconds register in command buffer
	
	configTime(time, numTimeUnits);	// Configure Time for DS3231
	
	// Store new time in command buffer
	for (int i = 0; i < numTimeUnits; i++)
	cmdBuffer[i + START_ADDRESS_OFFSET] = time[i];
	
	return i2cMasterTransmit(DS3231_SLAVE_ADDR, cmdBuffer, cmdBuffSize);
}

