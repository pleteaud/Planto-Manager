/*
 * main.h
 *
 * Created: 1/3/2021 5:43:27 AM
 *  Author: plete
 */ 


#ifndef MAIN_H_
#define MAIN_H_

/************************************************************************/
/*							Includes/Constants	 	                    */
/************************************************************************/
#include <atmel_start.h>
#include <util/delay.h>
//#include "DHT11.h"
#include "timer.h"
//#include "Moisture_Sensor.h"
#include "rtc.h"
#include "buffer.h"
#include "cmd_proc.h"
#include "i2cMasterControl.h"
#include "LCD.h"
#include "mcp23017.h"
#include "BME280_driver-master/bme280.h"

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/

void printTime();
void updateTime();
void printSymbols(lcd_t *lcd);


#endif /* MAIN_H_ */