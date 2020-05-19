/*
 * DHT11.h
 *
 * Created: 3/25/2020 3:17:18 PM
 *  Author: plete
 */ 


#ifndef DHT11_H_
#define DHT11_H_

/************************************************************************/
/*							Includes				 	                */
/************************************************************************/
#include "stdint.h"
#include "timer.h"

/************************************************************************/
/*							Constants						            */
/************************************************************************/
#define ERROR_LIST_SIZE 100

/************************************************************************/
/*							Enums Definition		 	                */
/************************************************************************/
enum dht11_states_e
{ 
	DHT11_IDLE = 0, 
	DHT11_READ,
	DHT11_COOLDOWN
};
enum comm_status_e
{
	READ_SUCCESS = 0,
	NO_RESPONSE_ERROR,
	TIME_OUT_ERROR,
	BAD_CHECKSUM_ERROR,
};
/************************************************************************/
/*				Type Defs + Struct Definition							*/
/************************************************************************/
typedef struct dht11_sensor_s
{
	uint8_t temperature;
	uint8_t humidity;
	uint8_t pin;
	enum dht11_states_e state;
	uint8_t errorCounter;
	uint8_t errorList[ERROR_LIST_SIZE];
}dht11_sensor_t;

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
void dht11Init(dht11_sensor_t *sensorP, uint8_t pinNum);
uint8_t dht11GetTemp(dht11_sensor_t *sensorP);
uint8_t dht11GetRH(dht11_sensor_t *sensorP);
uint8_t dht11Poll(dht11_sensor_t *sensorP);
#endif /* DHT11_H_ */
