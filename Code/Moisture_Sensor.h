/*
 * Moisture_Sensor.h
 *
 * Created: 5/12/2020 12:20:26 PM
 *  Author: Davo Pleteau
 */ 


/************************************************************************/
/*							Includes/Constants	 	                    */
/************************************************************************/
#include "stdint.h"
#include "stdbool.h"
#include "timer.h"

#ifndef MOISTURE_SENSOR_H_
#define MOISTURE_SENSOR_H_

/************************************************************************/
/*							Enums Definition		 	                */
/************************************************************************/
typedef enum soil_moisture_sensor_state_e
{
	MS_IDLE=0,
	MS_STABLILIZING,
	MS_READ,
	MS_READ_COMPLETE
} soil_moisture_sensor_state_e;

/************************************************************************/
/*				            Struct Definition							*/
/************************************************************************/
typedef struct soil_moisture_sensor_s
{
	uint8_t adcChannel;
	soil_moisture_sensor_state_e state; 
	double moisture;
	uint8_t sampleNum;
	uint8_t timeStamp;
} soil_moisture_sensor_t;

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
void sensorInit(soil_moisture_sensor_t *sensorP, uint8_t chann);
bool sensorRead(soil_moisture_sensor_t *sensorP);
double sensorGet(soil_moisture_sensor_t *sensorP);
void sensorPower(bool relayState);
void sensorCalibrate(soil_moisture_sensor_t *sensorP);
#endif /* MOISTURE_SENSOR_H_ */