/*
 * alarm.c
 *
 * Created: 6/29/2019 9:23:40 PM
 *  Author: plete
 */ 

/************************************************************************/
/*                     Includes/Constants                               */
/************************************************************************/
#include "alarm.h"
#include "rtc.h"
#include "stddef.h"
#include "ds3231_regs_and_utils.h"
#include "buffer.h"
#include "cmd_structure.h"
#include "cmd_proc.h"


/* Maximums */
#define MAX_TIME_DIGITS			(0x07)
#define MAX_ALARM_TIME_UNITS	(0x04) //total time - MAX PAYLOAD SIZE - ALARM ADDRESS OFFSET - 1 CAUSE 0 COUNTS AS A UNIT (SECONDS) 
#define TOTAL_ALARM1_REGISTERS	(0x04)
#define TOTAL_ALARM2_REGISTERS	(0x03)

/* Bit Manipulations */
#define AxM1_FLAG_SHIFT         (0x1f)
#define AxM2_FLAG_SHIFT         (0x17)
#define AxM3_FLAG_SHIFT         (0x0f)
#define AxM4_FLAG_SHIFT         (0x07)
#define DY_DT_STATUS_BIT_OFFSET (0x01)
#define DY_DT_STATUS_BIT_POS    (AxM4_FLAG_SHIFT - DY_DT_STATUS_BIT_OFFSET)
#define AxF_STATUS_MASK			(0x01)
#define A1F_STATUS_OFFSET		(0x00)
#define A2F_STATUS_OFFSET		(0x01)

/************************************************************************/
/*                      Private Variables                               */
/************************************************************************/
static uint8_t alarmFlagStatus = 00;
static bool alarmIntSet = false;

/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/
static void alarmInit(alarm_t *alarmP, uint8_t alarmPos);
static void alarmCBInit(alarm_t *alarmP);
static void alarmExecuteCB(alarm_t *alarmP);
static void alarmCheck(alarm_t *alarmHandle, uint8_t hour, uint8_t minutes);
static void configPayload(enum alarm_registers_e ax, uint8_t *time);
static void insertTimeInPayload(uint32_t configTime, uint8_t *payload, uint8_t numTimeUnits);

/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/

/** Initializes all alarm in the list of alarms */
void alarmsInit(alarm_t *listOfAlarms, uint8_t numberOfAlarms)
{
	for(int i = 0; i < numberOfAlarms; i++)
	{
		alarmInit(listOfAlarms + i, i);
	}
}

/* Sets the day, hour, minutes, and seconds of an alarm */
/* Note if unsuccessful, higher object must attempt to set it again */
bool alarmSet(alarm_t *alarmP, enum days_e day, uint8_t hour, uint8_t minutes, uint8_t seconds)
{
	bool status = false;
	
	alarmP->day = day;
	alarmP->hours = hour;
	alarmP->minutes = minutes;
	alarmP->seconds = seconds;
	alarmP->state = ALARM_UNSET;
	
	uint8_t time[MAX_ALARM_TIME_UNITS] = {seconds, minutes, hour, day};
	/* Request to send new command */
	/* If cmdProc isn't ready, wait till next attempt to set alarm */
	//if(cmdProcCtrlRequestNewCmd())
	//{
		//return status;
	//}
	
	/* Check for the next available alarm */
	switch(alarmFlagStatus)
	{	
		case 0:
		case 2:
		{
			/* Program first alarm if either alarm registers are free {00}, or the first alarm is free {10}.*/
			configPayload(ALARM1, time);
			alarmFlagStatus |= AxF_STATUS_MASK << A1F_STATUS_OFFSET;
			status = true;
			break;
		}
		case 1:
		{
			/* Program second alarm register */
			configPayload(ALARM2, time);
			alarmFlagStatus |= AxF_STATUS_MASK << A2F_STATUS_OFFSET;
			status = true;
			break;
		}
		case 3:
		{
			/* No alarm currently available */
			break;
		}
		default:
		{
			/* The code shouldn't reach here */
		}
	}
	return status;
}

/* Sets the callback function for an alarm */
void alarmSetCB(alarm_t *alarmP, void (*funcP)(void *objP), void *objP)
{
	alarmP->alarmCB.objP = objP;
	alarmP->alarmCB.alarmOnCB = funcP;		
}

/*checks if an alarm has been triggered */
/* In the case an alarm hasn't been set yet, it will ..... */
void alarmsPoll(alarm_t *alarmList, uint8_t numAlarms)
{
	if (alarmIntSet)
	{
		//alarmIrqHandle(alarmIntSet);
		return;
	}
	else
	{
		//alarmCheck(alarmList);	
	}
	
	
}

/* Set alarm interrupt flag */
void alarmInterruptSet(void)
{
	alarmIntSet = true;
}

/* Handles an alarm interrupt by requ ...*/ 
void alarmIrqHandle(alarm_t *alarmList, uint8_t numAlarms)
{
	/* Request to send new command */
	/* If cmdProc isn't ready, don't unset alarmInterruptSet flag to retry again */
	if(cmdProcCtrlRequestNewCmd())
	{
		return;
	}
	cmd_hdr_t *cmdHdr;
	cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + START_ADDRESS_OFFSET);
	cmdHdr->type = READ_STATUS_REGS;
	cmdHdr->length = sizeof(cmd_hdr_t) + START_ADDRESS_OFFSET;
	cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
	
	//read/repeated start
	// use I2C to extract status registers.
	// check which alarm is on by checking A2F/A1F status (bits 1 and 2)
	// if A1F is on, check address x07 to x09
	// if A2F is on, check address x0Bh to xCh
	// invoke alarmsCheck() to then determine which alarm was on.
}


/************************************************************************/
/*                     Private Functions Implementation                 */
/************************************************************************/

/* Initializes an alarm object */
static void alarmInit(alarm_t *alarmP, uint8_t alarmPos)
{
	alarmP->hours = 0;
	alarmP->minutes = 0;
	alarmP->state = ALARM_UNSET;
	alarmP->alarmPos = alarmPos;
	/* Initialize Call back structures to NULL to guarantee no function or obj is attached it. */
	alarmCBInit(alarmP);
}

/* Initializes an alarm callback object */
static void alarmCBInit(alarm_t* alarmP)
{
	alarmP->alarmCB.objP = NULL;
	alarmP->alarmCB.alarmOnCB = NULL; 
}

/* Execute an alarm's callback function */
static void alarmExecuteCB(alarm_t* alarmP)
{
	alarmP->alarmCB.alarmOnCB(alarmP);
}

/* .... */
static void alarmCheck(alarm_t *alarmP, uint8_t hour, uint8_t minutes)
{
	/* First check if specified alarm was configured/set. If not use the time stored in alarmP to invoke config payload */
	if (alarmP->state == ALARM_UNSET)
	{
		alarmSet(alarmP, alarmP->day, alarmP->hours, alarmP->minutes, alarmP->seconds);
		return;
	}
	if( (alarmP->hours == hour) && (alarmP->minutes == minutes))
	{
		alarmExecuteCB(alarmP);
		alarmP->state = ALARM_UNSET;
	}
}

/* Configure the command payload for setting the alarm time in DS3231 device */
static void configPayload(enum alarm_registers_e alarmReg, uint8_t *time)
{
	uint32_t timebits = 0;
	
	/* If a day is specified, set DT/DY register to match with day of week */
	if (time[3] != NONE)
	{
		timebits |= 1 << AxM4_FLAG_SHIFT;
		timebits |= 1 << DY_DT_STATUS_BIT_POS;
	}

	timebits |= configTime(time, MAX_ALARM_TIME_UNITS, DIGITS_PER_TIME_UNIT);
	
	
	/* Set up temporary cmd header to send to send to cmdPoll */
	cmd_hdr_t *cmdHdr;
	if(alarmReg == ALARM1)
	{
		cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + START_ADDRESS_OFFSET + TOTAL_ALARM1_REGISTERS);
		cmdHdr->type = SET_ALARM1_TIME;
		cmdHdr->length = sizeof(cmd_hdr_t) + START_ADDRESS_OFFSET + TOTAL_ALARM1_REGISTERS;
		
		/* Store address pointer of alarm 1 before the time */
		cmdHdr->payload[0] = A1_SEC_ADDR;
		insertTimeInPayload(timebits,cmdHdr->payload + START_ADDRESS_OFFSET, TOTAL_ALARM1_REGISTERS);
		
		/* Calculate Checksum */	
		cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
		
	}
	else
	{
		/* Configure command specifications */
		cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + START_ADDRESS_OFFSET + TOTAL_ALARM2_REGISTERS);
		cmdHdr->type = SET_ALARM2_TIME;
		cmdHdr->length = sizeof(cmd_hdr_t) + START_ADDRESS_OFFSET + TOTAL_ALARM2_REGISTERS;
		
		/* Store address pointer of alarm 2 before the time */
		cmdHdr->payload[0] = A2_MIN_ADDR;
		insertTimeInPayload(timebits, cmdHdr->payload + START_ADDRESS_OFFSET, TOTAL_ALARM2_REGISTERS);
		
		/* Calculate Checksum */
		cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
	}	 
}

/* Insert the configured time bit inside a 8 bit payload */
static void insertTimeInPayload(uint32_t configTime, uint8_t *payload, uint8_t numTimeUnits)
{
	uint8_t bitOffSet = (numTimeUnits * BYTE_SHIFT)-BYTE_SHIFT;
	for(int i = 0; i < numTimeUnits; i++)
	{
		*(payload + i) = configTime >> bitOffSet; // sec/min/hour/day
		bitOffSet -= BYTE_SHIFT;		
	}
}

