#include <atmel_start.h>
#include <util/delay.h>
#include "DHT11.h"
#include "timer.h"
#include "Moisture_Sensor.h"
#include "rtc.h"
dht11_sensor_t rSensor;
soil_moisture_sensor_t soilSensor;

void tempCallBack(uint8_t *count);
uint8_t alarmCount = 0;
int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
	/* Turn off External interrupt until an alarm is set*/
	EIMSK &= ~(1 << INT0);
	/* Initialize temp and soil sensors */
	//dht11Init(&rSensor,6);
	//sensorInit(&soilSensor,0);
	//sensorPower(true);
	/* Initialize buffer */
	bufferInit(retrieveActiveBuffer());
	/* Initialize cmd procedure */
	cmdProcInit();
	/* Initialize Master I2C */
	i2cMasterInit(DS3231_SLAVE_ADDR);
	/* Initialize RTC */
	rtcInit();
	/* Set time registers */
	uint8_t initialTime[7] = {00,23,21,THURS,6, 8, 20};
	if (rtcSetCtrlReg(retrieveActiveRTC(), 0))
	{
		/* Wait till reg is set */
		while (!rtcIsFree(retrieveActiveRTC()))
		{
			cmdProcCtrPoll();
			rtcPoll();
		}
	}
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
	uint8_t a2Time[4] = {00, 00, 00, 0};
	alarmSetCB(rtcGetAlarm(retrieveActiveRTC(),ALARM_2),tempCallBack,alarmCount);
	if(rtcSetAlarm(retrieveActiveRTC(),ALARM_2,a2Time,A2_MATCH_ONCE_PER_MIN))
	{
		/* Wait till a2 is set */
		while (!rtcIsFree(retrieveActiveRTC()))
		{
			cmdProcCtrPoll();
			rtcPoll();
		}
	}
	
	if (rtcSetCtrlReg(retrieveActiveRTC(), rtcGetCtrlReg(retrieveActiveRTC()) | INTCN_FLAG | AI2E_FLAG))
	{
		/* Wait till reg is set */
		while (!rtcIsFree(retrieveActiveRTC()))
		{
			cmdProcCtrPoll();
			rtcPoll();
		}
	}
	if (rtcSetStatReg(retrieveActiveRTC(), 0))
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
	}

	/* Initialize sensors */
	
	//
	//int8_t temperature = 0;
	//int8_t humidity = 0;
	//volatile double moisture;
	///* Test Calibration function */
	//sensorCalibrate(&soilSensor);
	//while (1)
	//{
		////if(dht11ReadTempRH(&rSensor))
		////{
			////DDRB |= 1<<PINB4;
			////PORTB = 1 << PINB4; //Turns ON All LEDs
			/////* Delay for 2000 to allow next read */
			//////milli_delay(500);
		////}
		//if (sensorRead(&soilSensor))
		//{
			//moisture = sensorGet(&soilSensor); 
		//}
	//}
}

void tempCallBack(uint8_t *count)
{
	alarmCount++;
	if(alarmCount == 3)
	{
		uint8_t at = 8;
		at *=5;
	}
}

	