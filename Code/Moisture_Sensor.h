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

#ifndef MOISTURE_SENSOR_H_
#define MOISTURE_SENSOR_H_

/************************************************************************/
/*							Enums Definition		 	                */
/************************************************************************/
typedef enum soil_moisture_sensor_state_e
{
	MS_UNCALIBRATED=0,
	MS_IDLE,
	MS_STABLILIZING,
	MS_READ,
	MS_READ_COMPLETE
} soil_moisture_sensor_state_e;

/************************************************************************/
/*				            Struct Definition							*/
/************************************************************************/
typedef struct soil_moisture_sensor_s
{
	soil_moisture_sensor_state_e state; 
	double moisture;
	uint8_t sampleNum;
	uint16_t timeStamp;
	bool calibrateFlag;
} soil_moisture_sensor_t;

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
void soilSensInit(soil_moisture_sensor_t *sensorP);
double soilSenGetMoisture(soil_moisture_sensor_t *sensorP);
void soilSenPwrRelay(bool relayState);
void soilSenCalibrate(soil_moisture_sensor_t *sensorP);
void readSens(soil_moisture_sensor_t *sensorP);
#endif /* MOISTURE_SENSOR_H_ */