#include <atmel_start.h>
#include <util/delay.h>
#include "DHT11.h"
#include "timer.h"
#include "Moisture_Sensor.h"
dht11_sensor_t rSensor;
soil_moisture_sensor_t soilSensor;
int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
	
	/* Initialize sensors */
	dht11Init(&rSensor,6);
	sensorInit(&soilSensor,0);
	sensorPower(true);
	
	int8_t temperature = 0;
	int8_t humidity = 0;
	volatile double moisture;
	/* Test Calibration function */
	sensorCalibrate(&soilSensor);
	while (1)
	{
		//if(dht11ReadTempRH(&rSensor))
		//{
			//DDRB |= 1<<PINB4;
			//PORTB = 1 << PINB4; //Turns ON All LEDs
			///* Delay for 2000 to allow next read */
			////milli_delay(500);
		//}
		if (sensorRead(&soilSensor))
		{
			moisture = sensorGet(&soilSensor); 
		}
	}
}



	/* Initialize buffer */
//	bufferInit(retrieveActiveBuffer());
	/* Initialize cmd procedure */
	//cmdProcInit();
	/* Will add RTC Functionality later */
	///* Initialize Master I2C */
	//i2cMasterInit(DS3231_SLAVE_ADDR);
	//
	///* Initialize RTC */
	//uint8_t controlBit = 0;
	//controlBit |= INTCN_FLAG | AI2E_FLAG | AI1E_FLAG;
	//uint8_t statusBit = 0;
	//
	//uint8_t initialTime[7] = {37,30, 23, 5,31, 6, 89};
	//rtcInit(retrieveActiveRTC(), controlBit, statusBit,initialTime);
	
	/* Initialize alarms */
	//alarmsInit(list,numAlarms);
	//uint8_t time2[4] = {00, 36, 4, 0}; 

	
	

	//bool stat = alarmSet(&list[1],time2, NONE);
	/* Replace with your application code */
	
