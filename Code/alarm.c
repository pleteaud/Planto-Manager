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
#include "stddef.h"
#include "ds3231_regs_and_utils.h"
#include "buffer.h"
#include "cmd_structure.h"
#include "cmd_proc.h"

#define TOTAL_ALARM1_REGISTERS	(0x04)
#define TOTAL_ALARM2_REGISTERS	(0x03)

#define ALARM_SET_MASK			(0x01)
#define ALARM_SET_POS			(0x00)
#define ALARM_SET_FLAG			(ALARM_SET_MASK << ALARM_SET_POS)

#define A1_SET_MASK				(0x01)
#define A1_SET_POS				(0x01)
#define A1_SET_FLAG				(A1_SET_MASK << A1_SET_POS)

#define A2_SET_MASK				(0x01)
#define A2_SET_POS				(0x02)
#define A2_SET_FLAG				(A2_SET_MASK << A2_SET_POS)

#define ALARM_DY_SET_MASK		(0x01)
#define ALARM_DY_SET_POS		(0x03)
#define ALARM_DY_SET_FLAG		(ALARM_DY_SET_MASK << ALARM_DY_SET_POS)

#define ALARM_DT_SET_MASK		(0x01)
#define ALARM_DT_SET_POS		(0x04)
#define ALARM_DT_SET_FLAG		(ALARM_DT_SET_MASK << ALARM_DT_SET_POS)

/************************************************************************/
/*                      Private Variables                               */
/************************************************************************/
static uint8_t axSet = 00; //Tracker for which alarm is set 
static uint8_t axIx = 00; //Tracker for which alarm interrupt is set

/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/
static void alarmInit(alarm_t *alarmP, uint8_t alarmPos);
static void alarmCBInit(alarm_t *alarmP);
static void alarmExecuteCB(alarm_t *alarmP);
static void alarmCheck(alarm_t *alarmP);
static void configPayload(alarm_t *alarmP);
static void configMatchingConditions(alarm_t *alarmP, uint32_t *timeBits, uint8_t setAlarm);
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
bool alarmSet(alarm_t *alarmP, uint8_t *time, enum dt_dy_flag_e dayDate)
{
	bool status = false;
	for(int i = 0; i < MAX_ALARM_TIME_UNITS; i++)
	{
		alarmP->time[i] = *(time + i);
	}
	
	/* Set alarm to unset */
	alarmP->flag = alarmP->flag & ~ALARM_SET_FLAG;
	
	/* Set Alarm flags */
	if(dayDate == DAY)
	{
		 alarmP->flag |= ALARM_DY_SET_FLAG;
	}
	else if (dayDate == DATE)
	{
		 alarmP->flag |= ALARM_DT_SET_FLAG;
	}
	
	/* Request to send new command */
	/* If cmdProc isn't ready, try again later */
	if(!cmdProcCtrlRequestNewCmd())
	{
		return status;
	}
	
	/* Check for the next available alarm */
	switch(axSet)
	{	
		case 0:
		case 4:
		{
			/* Program first alarm if either alarm registers are free {00}, or the first alarm is free {10}.*/
			alarmP->flag |= A1_SET_FLAG | ALARM_SET_FLAG;
			alarmP->matchFlag.a1MF = A1_MATCH_HR_MIN_SEC;
			axSet |= A1_SET_FLAG;
			configPayload(alarmP);
			status = true;
			break;
		}
		case 2:
		{
			/* Program second alarm register */
			alarmP->flag |= A2_SET_FLAG | ALARM_SET_FLAG;
			alarmP->matchFlag.a2MF = A2_MATCH_HR_MIN;
			axSet |= A2_SET_FLAG;
			configPayload(alarmP);
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
	/* Check if alarm 1 of DS3231 has been set */
	if (rtcGetSREG(retrieveActiveRTC()) && A1I_FLAG)
	{
		axIx |= A1I_FLAG; 
	}
	/* Check if alarm 2 of DS3231 has been set */
	if (rtcGetSREG(retrieveActiveRTC()) && A2I_FLAG)
	{
		axIx |= A2I_FLAG;
	}
	for (int i = 0; i < numAlarms; i++)
	{
		alarmCheck((alarmList + i));
	}
}


/************************************************************************/
/*                     Private Functions Implementation                 */
/************************************************************************/

/* Initializes an alarm object */
static void alarmInit(alarm_t *alarmP, uint8_t alarmPos)
{
	alarmP->flag = alarmP->flag & ~A1_SET_FLAG & ~A2_SET_FLAG; //set alarm status off for specific alarm
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
static void alarmCheck(alarm_t *alarmP)
{
	if((alarmP->flag & axIx) == A1I_FLAG)
	{
		/* Execute callback and clear interrupt */
		alarmExecuteCB(alarmP);
		alarmP->flag = alarmP->flag & ~A1_SET_FLAG;
		axIx = (axIx & ~A1I_FLAG);
	}
	else if ((alarmP->flag & axIx) == A2I_FLAG)
	{
		/* Execute callback and clear interrupt */
		alarmExecuteCB(alarmP);
		alarmP->flag = alarmP->flag & ~A2_SET_FLAG;
		axIx = (axIx & ~A2I_FLAG);
	}
}

/* Configure the command payload for setting the alarm time in DS3231 device */
static void configPayload(alarm_t *alarmP)
{
	uint32_t timebits = 0;
	/* Set up temporary cmd header to send to send to cmdPoll */
	cmd_hdr_t *cmdHdr;
	if(alarmP->flag & A1_SET_FLAG)
	{
		cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(set_alarm1_time_cmd_t));
		cmdHdr->type = SET_ALARM1_TIME;
		cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(set_alarm1_time_cmd_t);
		
		/* Store address pointer of alarm 1 before the time */
		cmdHdr->payload[0] = A1_SEC_ADDR;
		
		/* Config time bits */
		timebits = configTime(alarmP->time, MAX_ALARM_TIME_UNITS);
		configMatchingConditions(alarmP, &timebits, A1_SET_FLAG);
		
		
		/* Load time into cmd payload */
		insertTimeInPayload(timebits,cmdHdr->payload + START_ADDRESS_OFFSET, TOTAL_ALARM1_REGISTERS);
		
		/* Calculate Checksum */	
		cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
		
	}
	else
	{
		/* Configure command specifications */
		cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(set_alarm2_time_cmd_t));
		cmdHdr->type = SET_ALARM2_TIME;
		cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(set_alarm2_time_cmd_t);
		
		/* Store address pointer of alarm 2 before the time */
		cmdHdr->payload[0] = A2_MIN_ADDR;
		
		/* Config time bits */
		timebits |= configTime(alarmP->time, MAX_ALARM_TIME_UNITS);
		configMatchingConditions(alarmP, &timebits, A2_SET_FLAG);
		
		
		/* Load time into cmd payload */
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
/* configure the matching condition bits in the outgoing payload */ // change comment a bit idk 
static void configMatchingConditions(alarm_t *alarmP, uint32_t *timeBits, uint8_t setAlarm)
{
	/* Configure alarm to only go off when hours, minutes and seconds match */
	
	if(setAlarm == A1_SET_FLAG)
	{
		if (((alarmP->flag & ALARM_DY_SET_FLAG) | (alarmP->flag & ALARM_DT_SET_FLAG)) != NONE)
		{
			alarmP->matchFlag.a1MF = A1_MATCH_DY_HR_MIN_SEC;	
		}
		
		switch(alarmP->matchFlag.a1MF)
		{
			case A1_MATCH_ONCE_PER_SEC:
			{
				*timeBits |= getAxMxFlag(AxM1_FLAG_SCALOR) | getAxMxFlag(AxM2_FLAG_SCALOR) | getAxMxFlag(AxM3_FLAG_SCALOR) | getAxMxFlag(AxM4_FLAG_SCALOR);	
				break;
			}
			case A1_MATCH_SEC:
			{
				*timeBits |= getAxMxFlag(AxM2_FLAG_SCALOR) | getAxMxFlag(AxM3_FLAG_SCALOR) | getAxMxFlag(AxM4_FLAG_SCALOR);
				break;
			}
			case A1_MATCH_MIN_SEC:
			{
				*timeBits |= getAxMxFlag(AxM3_FLAG_SCALOR) | getAxMxFlag(AxM4_FLAG_SCALOR);
				break;
			}
			case A1_MATCH_HR_MIN_SEC:
			{
				*timeBits |= getAxMxFlag(AxM4_FLAG_SCALOR);
				break;
			}
			case A1_MATCH_DY_HR_MIN_SEC:
			{
				*timeBits |= DY_DT_FLAG;
				break;
			}
			case A1_MATCH_DT_HR_MIN_SEC:
			{
				*timeBits = (*timeBits & ~DY_DT_FLAG);
				break;
			}
		}
	}
	else
	{
		if (((alarmP->flag & ALARM_DY_SET_FLAG) | (alarmP->flag & ALARM_DT_SET_FLAG)) != NONE)
		{
			alarmP->matchFlag.a2MF = A2_MATCH_DY_HR_MIN;
		}
		switch(alarmP->matchFlag.a2MF)
		{
			case A2_MATCH_ONCE_PER_MIN:
			{
				*timeBits |= getAxMxFlag(AxM2_FLAG_SCALOR) | getAxMxFlag(AxM3_FLAG_SCALOR) | getAxMxFlag(AxM4_FLAG_SCALOR);
				break;
			}
			case A2_MATCH_MIN:
			{
				*timeBits |= getAxMxFlag(AxM3_FLAG_SCALOR) | getAxMxFlag(AxM4_FLAG_SCALOR);
				break;
			}
			case A2_MATCH_HR_MIN:
			{
				*timeBits |= getAxMxFlag(AxM4_FLAG_SCALOR);
				break;
			}
			case A2_MATCH_DY_HR_MIN:
			{
				*timeBits |= DY_DT_FLAG;
				break;
			}
			case A2_MATCH_DT_HR_MIN:
			{
				*timeBits = (*timeBits & ~DY_DT_FLAG);
				break;
			}
		}
	}
}

