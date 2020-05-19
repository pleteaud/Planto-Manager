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

/************************************************************************/
/*                     Struct Implementation                            */
/************************************************************************/
struct rtc_manager_s 
{
	uint8_t time[TIME_UNITS_TOTAL];
	uint8_t ctrlReg;
	uint8_t ctrlSREG;
	uint8_t agingOffsetReg;
	uint16_t tempReg;
	enum rtc_state_e state;
};

/************************************************************************/
/*                      Private Variables                               */
/************************************************************************/
static rtc_manager_t rtcMan;
static bool updateStatusReg = false;

/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/
static void insertTimeInPayload(uint64_t configTime, uint8_t *payload, uint8_t numTimeUnits);

/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/
/* Initialize an rtc_manager_t object with the specific control and status register value */
void rtcInit(rtc_manager_t *rtcP, uint8_t ctrlRegConfigBits, uint8_t statusConfigBits, uint8_t *initTime)
{
	rtcP->ctrlReg = ctrlRegConfigBits;
	rtcP->ctrlSREG = statusConfigBits;
	rtcP->agingOffsetReg = 0;
	rtcP->tempReg = 0;
	rtcP->state = RTC_UNSET;
		
	/* Store values inside array of time */
	for (int i = TIME_UNITS_SEC; i < TIME_UNITS_TOTAL; i++)
	{
		rtcP->time[i] = *(initTime + i);
	}
	
	/* configure the RTC control registers */
	rtcSetCtrlReg(rtcP);
}

/* Enable Oscillator */
void rtcEnableOsc(rtc_manager_t *rtcP)
{
	rtcP->ctrlReg = (rtcP->ctrlReg & ~EOSC_FLAG);
}

/* Enable Battery-Backed Square-Wave Enable */
void rtcEnableBBSQW(rtc_manager_t *rtcP)
{
	rtcP->ctrlReg |=  (rtcP->ctrlReg & ~BBSQW_FLAG);
}

/* Disable Oscillator */
void rtcDisableOsc(rtc_manager_t *rtcP)
{
	rtcP->ctrlReg |= EOSC_FLAG; 
}

/* Disable Battery-Backed Square-Wave Enable */
void rtcDisableBBSQW(rtc_manager_t *rtcP)
{
	rtcP->ctrlReg |= BBSQW_FLAG;
}

/* Forces a temperature sensor conversion */
void rtcConvTemp(rtc_manager_t *rtcP)
{
	rtcP->ctrlReg |= CONV_FLAG;
}

/* Set frequency of the square wave output: */
/* 00-1Hz, 01-1.024kHz, 10-4.096kHz, 11-8.192kHz */
void rtcSelectRate(rtc_manager_t *rtcP, bool rate1, bool rate2)
{
	/* Configure Rate 1 */
	((rate1 == true) ? (rtcP->ctrlReg |= RS1_FLAG) : (rtcP->ctrlReg = (rtcP->ctrlReg & ~RS1_FLAG)));
	
	/* Configure Rate 2 */
	((rate2 == true) ? (rtcP->ctrlReg |= RS2_FLAG) : (rtcP->ctrlReg = (rtcP->ctrlReg & ~RS2_FLAG)));
}

/* Enable Global Alarm Interrupts */
void rtcEnableAlarmInt(rtc_manager_t *rtcP)
{
	rtcP->ctrlReg |= INTCN_FLAG;
}

/* Enable Alarm 1 Interrupts */
void rtcEnableA1Int(rtc_manager_t *rtcP)
{
	rtcP->ctrlReg |= AI1E_FLAG;
}

/* Enable Alarm 2 Interrupts */
void rtcEnableA2Int(rtc_manager_t *rtcP)
{
	rtcP->ctrlReg |= AI2E_FLAG;
}

/* Disable Global Alarm Interrupts */
void rtcDisableAlarmInt(rtc_manager_t *rtcP)
{
	rtcP->ctrlReg = (rtcP->ctrlReg & ~INTCN_FLAG);
}

/* Disable Alarm1 Interrupts */
void rtcDisableA1Int(rtc_manager_t *rtcP)
{
	rtcP->ctrlReg |= (rtcP->ctrlReg & ~AI1E_FLAG);
}

/* Disable Alarm 2 Interrupts */
void rtcDisableA2Int(rtc_manager_t *rtcP)
{
	rtcP->ctrlReg |= (rtcP->ctrlReg & ~AI2E_FLAG);
}

/* Set the update status flag, to trigger a read status register */
void rtcUpdateStatus(void)
{
	updateStatusReg = true;
}

void rtcPoll(void)
{
	rtc_manager_t *rtcP = retrieveActiveRTC();
	/* Set time if it wasn't yet */
	if(rtcP->state == RTC_UNSET)
	{
		rtcSetTime(rtcP, rtcP->time);
		return;
	}
	/* Check if there's an update to the status  */
	if(!updateStatusReg)
	{
		return;
	}
	
	/* If there's an update, execute command to read status register */
	if(cmdProcCtrlRequestNewCmd())
	{
		return;
	}
	
	/* Clear updateStatusReg since RTC now has control of the command procedure */
	updateStatusReg = false;
	
	/* Send command to read status register */
	cmd_hdr_t *cmdHdr;
	cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(read_status_reg_cmd_t));
	cmdHdr->type = READ_STATUS_REGS;
	cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(read_status_reg_cmd_t);
	cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
	
	/* Store address pointer of control/status register */
	cmdHdr->payload[0] = RTC_CTRL_STAT_ADDR;
	return;
}

bool rtcSetCtrlReg(rtc_manager_t *rtcP)
{
	
	/* Check if there's an ongoing command */
	if(!cmdProcCtrlRequestNewCmd())
	{
		return false;
	}
	
	/* Send command to set control register */
	cmd_hdr_t *cmdHdr;
	cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(set_ctrl_reg_cmd_t));
	cmdHdr->type = SET_CONTROL_REGS;
	cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(set_ctrl_reg_cmd_t);
	cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
	
	/* Store address pointer of control register */
	cmdHdr->payload[0] = RTC_CTRL_ADDR;
	cmdHdr->payload[1] = rtcP->ctrlReg;
	return true;
}
/* Sets the RTC time (sec, min, hr, day, date, month/century, year) */
/* Note if unsuccessful, higher object must attempt to set it again */
bool rtcSetTime(rtc_manager_t *rtcP, uint8_t *time)
{
	bool status = false;
	/* Store values inside array of time */
	for (int i = TIME_UNITS_SEC; i < TIME_UNITS_TOTAL; i++)
	{
		rtcP->time[i] = *(time + i);
	}
	
	/* Configure Time bits */
	uint64_t timebits = 0;
	timebits = configTime(rtcP->time, TIME_UNITS_TOTAL);
	
	/* Request to send new command */
	/* If cmdProc isn't ready, wait till next attempt to set RTC */
	/* Otherwise we are ready to process a new command, so status is true */
	if(cmdProcCtrlRequestNewCmd())
	{
		return status;
	}
	status = true;
	
	/* Configure command header + payload */
	cmd_hdr_t *cmdHdr;
	cmdHdr = bufferSetData(retrieveActiveBuffer(), sizeof(cmd_hdr_t) + sizeof(set_curr_time_cmd_t));
	cmdHdr->type = SET_CURRENT_TIME;
	cmdHdr->length = sizeof(cmd_hdr_t) + sizeof(set_curr_time_cmd_t);
	
	/* Store address pointer of first register of DS3231 before sending payload */
	cmdHdr->payload[0] = RTC_SEC_ADDR;
	
	insertTimeInPayload(timebits,cmdHdr->payload + START_ADDRESS_OFFSET, TIME_UNITS_TOTAL);
	/* Indicate RTC has been set */
	rtcP->state = RTC_SET;
	/* Calculate Checksum */
	cmdHdr->checksum = cmdHdrCalcChecksum(cmdHdr,cmdHdr->length);
	
	return status;
}

/* Retrieve active RTC manager */ 
rtc_manager_t* retrieveActiveRTC(void)
{
	return &rtcMan;
}

/* Extract current status register from an rtc_manager_t object */
uint8_t rtcGetSREG(rtc_manager_t *rtcP)
{
	return rtcP->ctrlSREG;
}

/* Update an rtc_manager_t's status register */ 
void rtcUpdateSREG(rtc_manager_t *rtcP, uint8_t newSREG)
{
	rtcP->ctrlSREG = newSREG;
}
/* Extract the digits values from a given number */
void getDigits(uint8_t timeUnit, uint8_t *digitP)
{	
	/* Make an array for 2 digits number */
	uint8_t tempDigArr[DIGITS_PER_TIME_UNIT] = {0};	
	
	/* Start extracting digits */
	int arrIndex = 0;
	while(timeUnit > 0)
	{
		tempDigArr[arrIndex]= timeUnit % 10;
		timeUnit /= 10;
		arrIndex++;
	}
	/* The digits in tempDigArr are in reverse. Must store the reverse of tempDigArr for correct digit order */
	arrIndex = DIGITS_PER_TIME_UNIT - 1; //Reinitialize array index to avoid going out of range of tempDigArr.
	for (int j = 0; j < DIGITS_PER_TIME_UNIT; j++)
	{
		digitP[j] = tempDigArr[arrIndex--];
	}
}

/* Configures the time bits in a way that satisfies the DS3231 registers */
uint64_t configTime(uint8_t *time, uint8_t totTimeUnits)
{
	//Allocate an array for the largest input of time data (which is 7){ seconds/ minutes/ hours/ day/ date/ month(century)/ year}
	uint8_t digits[TIME_UNITS_TOTAL*DIGITS_PER_TIME_UNIT] = {0}; 

	int digitsIndex = 0;
	for (int i = 0; i < totTimeUnits; i++)
	{
		getDigits(*(time+i), (digits + digitsIndex));
		digitsIndex += DIGITS_PER_TIME_UNIT;
	}
	
	/* Configure time bits as so: */
	/* Seconds {MSB 8 bits} + Minutes {8 bits} + Hours {8 bits} + Day  {8 bits} + Date {8 bits} + Month {8 bit} + Year {8 bit} + 0000 0000 */
	int bitsOffSet = (totTimeUnits * BYTE_SHIFT) - BYTE_SHIFT; //starts at 48 because seconds byte takes up 8 bits
	uint64_t timeBits = 0;
	uint64_t tempDigit = 0;
	for (int i = 0; i < totTimeUnits * DIGITS_PER_TIME_UNIT; i++)
	{
		if (i % DIGITS_PER_TIME_UNIT)
		{
			tempDigit |= digits[i];
			timeBits = (tempDigit << bitsOffSet) | timeBits;
			bitsOffSet -= BYTE_SHIFT;
			tempDigit = 0;
		}
		else
		{
			tempDigit |= digits[i] << NIBBLE_SHIFT;
		}
	}
	return timeBits;
}

/************************************************************************/
/*					Private Functions Implementation			        */
/************************************************************************/
/* Insert the configured time bit inside a 8 bit payload */
static void insertTimeInPayload(uint64_t configTime, uint8_t *payload, uint8_t numTimeUnits)
{
	uint8_t bitOffSet = (numTimeUnits * BYTE_SHIFT)-BYTE_SHIFT;
	for(int i = 0; i < numTimeUnits; i++)
	{
		*(payload + i) = configTime >> bitOffSet; // sec/min/hour/day
		bitOffSet -= BYTE_SHIFT;
	}
}
