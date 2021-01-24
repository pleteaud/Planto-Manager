/*
 * ds3231.c
 *
 * Created: 7/14/2019 3:49:15 PM
 *  Author: plete
 */ 

/************************************************************************/
/*                     Includes/Constants                               */
/************************************************************************/
#include "ds3231.h"
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
static void configTime(uint8_t *time, const uint8_t totTimeUnits);

static void ds3231AddError(ds3231_t *deviceP, const enum ds3231_errors_e error);

static uint32_t configMatchingConditions(const enum alarm_match_options_e matchFlag, const ALARM_NUMB pos);

static bool verifyAxData(const uint8_t *dataP, const uint8_t *axTime, const uint8_t len);

static bool verifyCtrlReg(const uint8_t *dataP, const uint8_t ctrlReg);

static void ds3231Update(ds3231_t *deviceP, const uint8_t regs[]);

static bool ds3231SetTimeRegs(ds3231_t *deviceP, const uint8_t cmdBuffSize, const uint8_t startAddr,
						   uint8_t *time, const uint8_t numTimeUnits);

static bool verifyTime(uint8_t time, time_units_t unit);
/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/

/* Initialize an ds3231_t object's data members with the exception of time */
void ds3231Init(ds3231_t *deviceP)
{
	/* Initial value of Ctrl Reg 00x00100 */
	deviceP->ctrlReg |= INTCN_FLAG; 
	ds3231SetCtrlReg(deviceP, INTCN_FLAG);
	
	/* Initial time on power up is 01/01/00 01 00:00:00 (DD/MM/YY DOW HH:MM:SS) */
	for (uint8_t i = 0; i < TIME_UNITS_TOTAL; i++)
	{
		switch(i)
		{
			case TIME_UNITS_DY:
				deviceP->time[i] = MON;
				break;
			case TIME_UNITS_MO_CEN:
				deviceP->time[i] = JAN;
				break;
			case TIME_UNITS_DT:
				deviceP->time[i] = 1;
				break;
			default:
				deviceP->time[i] = 0;
		}
	}
	
	// Initial State of the Status Registers should be 0
	deviceP->ctrlStatReg = deviceP->agingOffsetReg = deviceP->tempReg = deviceP->errorCount = 0;
	
	/* Initialize Alarm Objects */
	alarmInit(&deviceP->alarm1);
	deviceP->alarm1.matchFlag = A1_MATCH_DT_HR_MIN_SEC; // 0
	alarmInit(&deviceP->alarm2);
	deviceP->alarm2.matchFlag = A2_MATCH_DT_HR_MIN; // 0 
	
	for (int i = 0; i < 256; i ++)
	{
		deviceP->errorList[i] = 0; // Initialize error count list
	}
}

/* Set seconds of RTC */
bool ds3231SetSeconds(ds3231_t *deviceP, uint8_t seconds)
{	
	if (ds3231SetTimeRegs(deviceP, SET_RTC_SEC_CMD_SIZE, RTC_SEC_ADDR, &seconds, 1))
	{
		deviceP->time[RTC_SEC_ADDR] = seconds; // Update ds3231 object 
		return true;
	}
	
	return false;
}

/* Set minutes of RTC */
bool ds3231SetMinutes(ds3231_t *deviceP, uint8_t min)
{
	if (ds3231SetTimeRegs(deviceP, SET_RTC_MIN_CMD_SIZE, RTC_MIN_ADDR, &min, 1))
	{
		deviceP->time[RTC_MIN_ADDR] = min; // Update ds3231 object 
		return true;
	}
	
	return false;
}

/* Set hours of RTC */
bool ds3231SetHour(ds3231_t *deviceP, uint8_t hour)
{	
	if (ds3231SetTimeRegs(deviceP, SET_RTC_HR_CMD_SIZE, RTC_HRS_ADDR, &hour, 1))
	{
		deviceP->time[RTC_HRS_ADDR] = hour; // Update ds3231 object 
		return true;
	}
	
	return false;
}

/* Set day of RTC */
bool ds3231SetDay(ds3231_t *deviceP, enum days_e day)
{
	if (ds3231SetTimeRegs(deviceP, SET_RTC_DY_CMD_SIZE, RTC_DY_ADDR, &day, 1))
	{
		deviceP->time[RTC_DY_ADDR] = day; // Update ds3231 object 
		return true;
	}
	return false;
}

/* Set date of RTC */
bool ds3231SetDate(ds3231_t *deviceP, uint8_t date)
{
	if (ds3231SetTimeRegs(deviceP, SET_RTC_DT_CMD_SIZE, RTC_DT_ADDR, &date, 1))
	{
		deviceP->time[RTC_DT_ADDR] = date; // Update ds3231 object 
		return true;
	}
	
	return false;
}

/* Set month and century of RTC */
bool ds3231SetMonCen(ds3231_t *deviceP, const uint8_t month, const bool century)
{
	uint8_t monCen = century << 7 | month ; // configure century flag of month/century register
	if (ds3231SetTimeRegs(deviceP, SET_RTC_MON_CEN_CMD_SIZE, RTC_M_CEN_ADDR, &monCen, 1))
	{
		deviceP->time[RTC_M_CEN_ADDR] = monCen;
		return true;
	}
	
	return false;
}

/* Set all time units of RTC object */ 
bool ds3231SetTime(ds3231_t *deviceP, uint8_t *time)
{
	if (ds3231SetTimeRegs(deviceP, SET_RTC_TIME_CMD_SIZE, RTC_SEC_ADDR, time, TIME_UNITS_TOTAL))
	{
		// If successful update time data member
		for (uint8_t i = RTC_SEC_ADDR; i < TIME_UNITS_TOTAL; i++) //startAddr will match enum value of register
		{	
			if(verifyTime(time[i], i))
				deviceP->time[i] = time[i];
			else
				return false;
		}
		return true;
	}
	return false;
}

/* Initiate the command to set the RTC control register */
bool ds3231SetCtrlReg(ds3231_t *deviceP, const uint8_t newCtrlReg)
{
	uint8_t cmdBuffer[SET_CONTROL_REG_CMD_SIZE];
	
	
	cmdBuffer[0]  = RTC_CTRL_ADDR;	// Store address of control register in command buffer
	cmdBuffer[1]  = newCtrlReg;		// Store new control register
	
	if(i2cMasterTransmit(DS3231_SLAVE_ADDR, cmdBuffer, SET_CONTROL_REG_CMD_SIZE))
	{
		deviceP->ctrlReg = newCtrlReg; // update ds3231 object
		return true;
	}
	else
	return false;
}

/* Initiate the command to set the RTC status register */
bool ds3231SetStatReg(ds3231_t *deviceP, const uint8_t newStatReg)
{
	
	uint8_t cmdBuffer[SET_STAT_REG_CMD_SIZE];
	cmdBuffer[0]  = RTC_CTRL_STAT_ADDR;	// Store address of ds3231 seconds register in command buffer
	cmdBuffer[1]  = newStatReg;	// Store new state register
	
	if (i2cMasterTransmit(DS3231_SLAVE_ADDR, cmdBuffer, SET_STAT_REG_CMD_SIZE))
	{
		deviceP->ctrlStatReg = newStatReg; // Update ds3231 object
		return true;
	}
	else
	return false;
}

/* Set RTC Alarm 1 */
bool ds3231SetAlarm1(ds3231_t *deviceP, uint8_t *time, const enum alarm_match_options_e matchFlag,
			      void (*funcP)(void *objP), void *objP)
{
	uint32_t matchBits = 0;
	
	uint8_t cmdBuffer[SET_ALARM1_TIME_CMD_SIZE];
	cmdBuffer[0] = A1_SEC_ADDR; // Store addres of alarm 1 register
	
	/* Config time bits */
	configTime(time,TOTAL_ALARM1_REGISTERS);
	alarmStoreTime(&deviceP->alarm1, time);
	matchBits = configMatchingConditions(matchFlag,ALARM_1);
	
	int len = TOTAL_ALARM1_REGISTERS - 1; /* Used to extract each byte from matchBits in the following loop*/
	for (int i = 0; i < TOTAL_ALARM1_REGISTERS; i++)
	deviceP->alarm1.time[i] |= matchBits >> (len-i) * BYTE_BIT_COUNT;
	deviceP->alarm1.matchFlag = matchFlag;
	
	/* Load time into cmd payload */
	for (int i = 0; i < TOTAL_ALARM1_REGISTERS; i++)
	cmdBuffer[START_ADDRESS_OFFSET + i] = deviceP->alarm1.time[i];
	
	if (i2cMasterTransmit(DS3231_SLAVE_ADDR, cmdBuffer, SET_ALARM1_TIME_CMD_SIZE))
	{
		ds3231SetAlarmCallback(deviceP, ALARM_1, funcP, objP);
		return ds3231SetCtrlReg(deviceP, deviceP->ctrlReg | AI1E_FLAG);
	}
	else
	return false;
}

/* Set RTC Alarm 2 */
bool ds3231SetAlarm2(ds3231_t *deviceP, uint8_t *time, const enum alarm_match_options_e matchFlag,
				  void (*funcP)(void *objP), void *objP)
{
	uint32_t matchBits = 0;
	
	uint8_t cmdBuffer[SET_ALARM2_TIME_CMD_SIZE];
	cmdBuffer[0] = A2_MIN_ADDR;	// Store address of alarm 2 register
	
	/* Config time bits */
	configTime(time,TOTAL_ALARM2_REGISTERS);
	alarmStoreTime(&deviceP->alarm2, time);
	matchBits = configMatchingConditions(matchFlag,ALARM_2);
	
	int len = TOTAL_ALARM2_REGISTERS - 1; /* Used to extract each byte from matchBits in the following loop*/
	for (int i = 0; i < TOTAL_ALARM2_REGISTERS; i++)
	deviceP->alarm2.time[i + 1] |= matchBits >> (len - i) * BYTE_BIT_COUNT; /*We start at second index of time because Alarm 2 doesn't have seconds (first index)*/
	deviceP->alarm2.matchFlag = matchFlag;
	
	/* Load time into cmd payload */
	for (int i = 0; i < TOTAL_ALARM2_REGISTERS; i++)
	cmdBuffer[START_ADDRESS_OFFSET + i] = deviceP->alarm2.time[i + 1];
	
	if (i2cMasterTransmit(DS3231_SLAVE_ADDR, cmdBuffer, SET_ALARM2_TIME_CMD_SIZE))
	{
		
		ds3231SetAlarmCallback(deviceP, ALARM_2, funcP, objP);
		return ds3231SetCtrlReg(deviceP, deviceP->ctrlReg | AI2E_FLAG);
	}
	else
	return false;
}

/* Set alarm Callback */
void ds3231SetAlarmCallback(ds3231_t *deviceP, const ALARM_NUMB alarm, void (*funcP)(void *objP), void *objP)
{
	if (alarm)
		alarmSetCB(&deviceP->alarm2, funcP, objP);	// Set alarm 2 call back
	else
		alarmSetCB(&deviceP->alarm1, funcP, objP);	// Set alarm 1 call back
}

/* Initiate a command to read all RTC registers */
bool ds3231ReadRegisters(ds3231_t *deviceP, uint8_t respData[])
{
	uint8_t cmdBuffer[READ_ALL_REGS_CMD_SIZE];

	// Store address of ds3231 seconds register in command buffer
	cmdBuffer[0]  = RTC_SEC_ADDR;
	
	bool status = false;
	
	// Transmit address
	if (i2cMasterTransmit(DS3231_SLAVE_ADDR, cmdBuffer, READ_ALL_REGS_CMD_SIZE))
	// Read the 19 registers of ds3231
	status = i2cMasterRead(DS3231_SLAVE_ADDR, respData, READ_ALL_REGS_RESP_SIZE);
	
	return status;
}

 /* Polling routine to update an RTC object */
 void ds3231Poll(ds3231_t *deviceP)
 {
	 if (!(INTCN_PIN & (1<<INTCN_PIN_NUM)) && (deviceP->ctrlReg & AI1E_FLAG || deviceP->ctrlReg & AI2E_FLAG))
	 {
		  uint8_t registers[READ_ALL_REGS_RESP_SIZE];
		  // Read registers
		  ds3231ReadRegisters(deviceP, registers);
	  
		  // Update the RTC object and check for control/alarm registers mismatch
		  ds3231Update(deviceP, registers);
	  
		  // Clear CMD complete flag and clear flags
		  ds3231SetStatReg(deviceP,0);
	 }
 }



/************************************************************************/
/*					Private Functions Implementation			        */
/************************************************************************/
/* Configures the time bits in a way that satisfies the DS3231 registers */
static void configTime(uint8_t *time, const uint8_t totTimeUnits)
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
static void ds3231AddError(ds3231_t *deviceP, const enum ds3231_errors_e error)
{	
	deviceP->errorList[deviceP->errorCount++] = error; 	
}

/* Configure the matching condition bits in the outgoing payload */ // change comment a bit idk
static uint32_t configMatchingConditions(const enum alarm_match_options_e matchFlag, const ALARM_NUMB pos)
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
static bool verifyAxData(const uint8_t *dataP, const uint8_t *axTime, const uint8_t len)
{
	for (int i = 0; i < len; i++)
	{
		if (*(dataP + i) != *(axTime + i)) 
			return false;
	}
	
	return true;
}

/* Check see if errors in setting CR register  */
static bool verifyCtrlReg(const uint8_t *dataP, const uint8_t ctrlReg)
{
	uint8_t tempCtrlReg = *dataP;
	/* (ignore CONV bit) */
	if (tempCtrlReg >> BBSQW_FLAG_POS != ctrlReg >> BBSQW_FLAG_POS && tempCtrlReg << RS2_FLAG_POS != ctrlReg << RS2_FLAG_POS) 
		return false;
		
	return true;
}

/* Update RTC object with data read from the DS3231 and check for erros */
static void ds3231Update(ds3231_t *deviceP, const uint8_t regs[])
{
	/* Update RTC time array */
	for(int i = 0; i < TIME_UNITS_TOTAL; i++)
	deviceP->time[i] = regs[i];
	
	/* Check see if errors in setting CR register */
	if(!verifyCtrlReg(regs + RTC_CTRL_ADDR, deviceP->ctrlReg))
	ds3231AddError(deviceP,INCONSISTENT_CTRL_REG);
	
	/* Verify Alarm1 registers if it's set */
	if(deviceP->ctrlReg & AI1E_FLAG)
	{
		if (!verifyAxData(regs + A1_SEC_ADDR, deviceP->alarm1.time, TOTAL_ALARM1_REGISTERS))
		ds3231AddError(deviceP,INCONSISTENT_A1);
	}
	
	/* Verify Alarm2 registers if it's set */
	if(deviceP->ctrlReg & AI2E_FLAG)
	{
		if (!verifyAxData(regs + A2_MIN_ADDR, &deviceP->alarm2.time[1], TOTAL_ALARM2_REGISTERS))
		ds3231AddError(deviceP, INCONSISTENT_A2);
	}
	
	/* Check Status register and clear respective bits */
	deviceP->ctrlStatReg = regs[RTC_CTRL_STAT_ADDR];
	if (deviceP->ctrlStatReg & OSC_FLAG)
	{
		/* Report error: OSC should normally be 0. */
		ds3231AddError(deviceP, OSC_STOP);
	}
	if ((deviceP->ctrlStatReg & A1I_FLAG) && (deviceP->ctrlReg & AI1E_FLAG))
	{
		/* Execute A1 callback */
		alarmExecuteCB(&deviceP->alarm1);
	}
	if ((deviceP->ctrlStatReg & A2I_FLAG) && (deviceP->ctrlReg & AI2E_FLAG))
	{
		/* Execute A2 callback which is to Update LCD Screen */
		alarmExecuteCB(&deviceP->alarm2);
	}
}

/* Set time of DS3231 */
static bool ds3231SetTimeRegs(ds3231_t *deviceP, const uint8_t cmdBuffSize, const uint8_t startAddr,
						   uint8_t *time, const uint8_t numTimeUnits)
{
	uint8_t cmdBuffer[cmdBuffSize];
	cmdBuffer[0]  = startAddr;	// Store address of ds3231 seconds register in command buffer
	
	configTime(time, numTimeUnits);	// Configure Time for DS3231
	
	// Store new time in command buffer
	for (int i = 0; i < numTimeUnits; i++)
	cmdBuffer[i + START_ADDRESS_OFFSET] = time[i];
	
	return i2cMasterTransmit(DS3231_SLAVE_ADDR, cmdBuffer, cmdBuffSize);
}

static bool verifyTime(uint8_t time, time_units_t unit)
{
	if (time < 0)
		return false;
		
	switch (unit)
	{
		case TIME_UNITS_SEC:
		case TIME_UNITS_MIN:
			if (time > 59)
				return false;
			break;
		case TIME_UNITS_HR:
			if (time > 23)
				return false;
			break;
		case TIME_UNITS_DY:
			if (time < MON || time > SUN)
				return false;
			break;
		case TIME_UNITS_DT:
			if (time > 31)
				return false;
			break;
		case TIME_UNITS_MO_CEN:
			if (time < 1 || time > 12)
				return false;
			break;
		case TIME_UNITS_YR:
			if (time > 99)
				return false;
			break;
	}
	return true;
}
