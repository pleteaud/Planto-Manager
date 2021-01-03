/*
 * alarm.c
 *
 * Created: 6/29/2019 9:23:40 PM
 *  Author: Davo Pleteau
 */ 

/************************************************************************/
/*                     Includes/Constants                               */
/************************************************************************/
#include "alarm.h"
#include "stddef.h"
/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/
static void alarmCBInit(alarm_t *alarmP);
/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/

/* Initializes an alarm object */
void alarmInit(alarm_t *alarmP)
{
	/* Initialize Call back structures to NULL to guarantee no function or obj is attached it. */
	alarmCBInit(alarmP);
}

void alarmStoreTime(alarm_t *alarmP, uint8_t *time)
{
	for(int i = 0; i < MAX_ALARM_TIME_UNITS; i++)
	{
		alarmP->time[i] = *(time + i);
	}
}

/* Sets the callback function for an alarm */
void alarmSetCB(alarm_t *alarmP, void (*funcP)(void *objP), void *objP)
{
	alarmP->alarmCB.objP = objP;
	alarmP->alarmCB.alarmOnCB = funcP;		
}

/* Execute an alarm's callback function */
void alarmExecuteCB(alarm_t* alarmP)
{
	if (alarmP->alarmCB.alarmOnCB != NULL)
	{
		alarmP->alarmCB.alarmOnCB(alarmP->alarmCB.objP);
	}
	
}

/************************************************************************/
/*                     Private Functions Implementation                 */
/************************************************************************/
/* Initializes an alarm callback object */
static void alarmCBInit(alarm_t* alarmP)
{
	alarmP->alarmCB.objP = NULL;
	alarmP->alarmCB.alarmOnCB = NULL; 
}
