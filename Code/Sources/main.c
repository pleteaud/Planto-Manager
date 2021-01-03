#include <atmel_start.h>
#include <util/delay.h>
#include "main.h"
dht11_sensor_t rSensor;
soil_moisture_sensor_t soilSensor;
int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
	
	/* Initialize buffer */
	bufferInit(retrieveActiveBuffer());
	/* Initialize cmd procedure */
	cmdProcInit();
	/* Initialize Master I2C */
	i2cMasterInit(DS3231_SLAVE_ADDR);
	
	/* Initialize LCD */
	lcdInit();	
	/* Shift Display left 16 times as information about */
	/* sensors/rtc initialization will be printed there */
	for (int i = 0; i<16; i++)
	{
		lcdCursorDisplayShift(8);
	}
	
	/* Indicate dht11 is being initialized */
	lcdSetDDRAMAdrr(0,16);
	lcdWriteString("  Initializing  ");
	lcdSetDDRAMAdrr(1,16);
	lcdWriteString(" Temp & Hum Sen ");
	/* Initialize dht11 */
	dht11Init(&rSensor);
	
	 ///* Initialize soil moisture sensor */
	//lcdSetDDRAMAdrr(0,16);
	//lcdWriteString("  Initializing  ");
	//lcdSetDDRAMAdrr(1,16);
	//lcdWriteString(" Soil Moist Sen ");
	//milli_delay(500);
	//soilSensInit(&soilSensor);
	//milli_delay(1000);
	//soilSenCalibrate(&soilSensor);
	//milli_delay(1000);
	
	/* Initialize and Configure RTC */
	lcdSetDDRAMAdrr(0,16);
	lcdWriteString("  Initializing  ");
	lcdSetDDRAMAdrr(1,16);
	lcdWriteString(" Real Time Clock ");
	milli_delay(1000);
	rtcInit();
	/* Shift Display right 16 times as the  */
	/* primary information is printed there */
	for (int i = 0; i<16; i++)
	{
		lcdCursorDisplayShift(12);
	}
	/* Set time registers */
	if (rtcSetCtrlReg(retrieveActiveRTC(), INTCN_FLAG))
	{
		/* Wait till reg is set */
		while (!rtcIsFree(retrieveActiveRTC()))
		{
			cmdProcCtrPoll();
			rtcPoll();
		}
	}
	
	uint8_t initialTime[7] = {00,00,16,MON,24,AUG,20};
	if(rtcSetTime(retrieveActiveRTC(),initialTime))
	{
		/* Wait till time is set */
		while (!rtcIsFree(retrieveActiveRTC()))
		{
			cmdProcCtrPoll();
			rtcPoll();
		}
	}

	/* Set Alarm 2 to occur every minute */
	uint8_t a2Time[4] = {00, 00, 00, 00};
	if(rtcSetAlarm(retrieveActiveRTC(),ALARM_2,a2Time,A2_MATCH_ONCE_PER_MIN))
	{
		/* Wait till a2 is set */
		while (!rtcIsFree(retrieveActiveRTC()))
		{
			cmdProcCtrPoll();
			rtcPoll();
		}
	}
	/* Set Alarm 1 to occur every minute */
	uint8_t a1Time[4] = {00, 6, 00, 01};
	alarmSetCB(rtcGetAlarm(retrieveActiveRTC(),ALARM_1), NULL, NULL);
	if(rtcSetAlarm(retrieveActiveRTC(),ALARM_1,a1Time,A1_MATCH_SEC))
	{
		/* Wait till a1 is set */
		while (!rtcIsFree(retrieveActiveRTC()))
		{
			cmdProcCtrPoll();
			rtcPoll();
		}
	}
	
	if (rtcSetCtrlReg(retrieveActiveRTC(), rtcGetCtrlReg(retrieveActiveRTC()) | AI2E_FLAG | AI1E_FLAG))
	{
		/* Wait till reg is set */
		while (!rtcIsFree(retrieveActiveRTC()))
		{
			cmdProcCtrPoll();
			rtcPoll();
		}
	}
	/* Clear out Status register in case there is an interrupt that wasn't cleared from previous session */
	if (rtcSetStatReg(retrieveActiveRTC(),0))
	{
		/* Wait till reg is set */
		while (!rtcIsFree(retrieveActiveRTC()))
		{
		cmdProcCtrPoll();
		rtcPoll();
		}
	}

	while(1)
	{
		rtcPoll();
		cmdProcCtrPoll();
		dht11Poll(&rSensor);
	}

	
}