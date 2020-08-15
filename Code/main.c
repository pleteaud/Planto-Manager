#include <atmel_start.h>
#include <util/delay.h>
#include "main.h"
dht11_sensor_t rSensor;
soil_moisture_sensor_t soilSensor;
int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
	/* Initialize LCD */
	lcdInit();	
	/* Initialize temp and soil sensors */
	dht11Init(&rSensor);
	soilSensInit(&soilSensor);
	soilSenCalibrate(&soilSensor);
	/* Initialize buffer */
	bufferInit(retrieveActiveBuffer());
	/* Initialize cmd procedure */
	cmdProcInit();
	/* Initialize Master I2C */
	i2cMasterInit(DS3231_SLAVE_ADDR);
	/* Initialize and Configure RTC */
	rtcInit();
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
	
	uint8_t initialTime[7] = {50,5,14,FRI,14,AUG,20};
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
	alarmSetCB(rtcGetAlarm(retrieveActiveRTC(),ALARM_1), alarmTiggerCB, NULL);
	if(rtcSetAlarm(retrieveActiveRTC(),ALARM_1,a1Time,A1_MATCH_MIN_SEC))
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
		soilSenPoll(&soilSensor);
	}

	
}
//int main(void)
//{
	///* Initializes MCU, drivers and middleware */
	//atmel_start_init();
	//
	///* Initialize sensors */
	//dht11Init(&rSensor);
//
	//int8_t temperature = 0;
	//int8_t humidity = 0;
	//volatile double moisture;
	///* Test Calibration function */
	//while (1)
	//{
		//if(dht11Poll(&rSensor))
		//{
			//temperature = dht11GetTemp(&rSensor);
			//humidity = dht11GetRH(&rSensor);
		//}
	//
	//}
//}
//double moisture;
	///* Test Calibration function */
	//soilSenCalibrate(&soilSensor);
	//while (1)
	//{
		//if (sensorRead(&soilSensor))
		//{
			//moisture = soilSenGetMoisture(&soilSensor); 
		//}
	//}
	//
	///* Initialize sensors */
	//dht11Init(&rSensor);
	//int8_t temperature = 0;
	//int8_t humidity = 0;
	//while (1)
	//{
		//if(dht11ReadTempRH(&rSensor))
		//{
			//temperature = dht11GetTemp(&rSensor);
			//humidity = dht11GetRH(&rSensor); //Turns ON All LEDs
		///* Delay for 2000 to allow next read */
		////milli_delay(500);
		//}
	//}