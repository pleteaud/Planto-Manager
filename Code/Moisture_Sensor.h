/*
 * Moisture_Sensor.h
 *
 * Created: 5/12/2020 12:20:26 PM
 *  Author: plete
 */ 

#include "stdint.h"
#include "stdbool.h"
#include "timer.h"

#ifndef MOISTURE_SENSOR_H_
#define MOISTURE_SENSOR_H_

enum moisture_sensor_state 
{
	MS_IDLE=0,
	MS_STABLILIZING,
	MS_READ,
	MS_READ_COMPLETE,
};

typedef struct moisture_sensor_s
{
	uint8_t adcChannel;
	enum moisture_sensor_state state; 
	double moisture;
	uint8_t sampleNum;
	uint8_t timeStamp;
}moisture_sensor_t;

void msInit(moisture_sensor_t *sensorP,uint8_t chann);
bool readMoisture(moisture_sensor_t *sensorP);
double msGetMoisture(moisture_sensor_t *sensorP);
void powerSensor(bool relayState);
#endif /* MOISTURE_SENSOR_H_ */