#include <atmel_start.h>
#include <util/delay.h>
#include "DHT11.h"
#include "timer.h"
#include "Moisture_Sensor.h"
dht11_sensor_t rSensor;
moisture_sensor_t ms;
int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
	
	int8_t temperature = 0;
	int8_t humidity = 0;
	double moisture;
	/* Delay a few seconds to allow dht11 to stablelize */
	//milli_delay(4000); 
	//dht11Init(&rSensor,6);
	msInit(&ms,0);
	while (1)
	{
		//if(dht11Poll(&rSensor))
		//{
			//DDRB |= 1<<PINB4;
			//PORTB = 1 << PINB4; //Turns ON All LEDs
			///* Delay for 2000 to allow next read */
			////milli_delay(500);
		//}
		if (readMoisture(&ms))
		{
			moisture = msGetMoisture(&ms); 
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
	
