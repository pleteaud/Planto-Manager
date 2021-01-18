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
#include "ds3231.h"
#include "i2cMasterControl.h"
#include "LCD.h"
#include "mcp23017.h"
#include "BME280_driver-master/bme280.h"
#include "keypad.h"

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/

void setUpdateFlag();

void printTime(lcd_t *lcdP, ds3231_t *ds3231P);

void printSymbols(lcd_t *lcd);

void printBMEdata(lcd_t *lcd, struct bme280_data *comp_data);

uint8_t initBME(struct bme280_dev *sensor, int8_t (*user_i2c_read)(uint8_t, uint8_t*, uint32_t, void*),
				int8_t (*user_i2c_write)(uint8_t, uint8_t*, uint32_t, void*),
				void (*user_delay_us)(uint32_t, void *), uint8_t *devAddr);
			 
int8_t userI2cWrite(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);

int8_t userI2cRead(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);

void userDelayUs (uint32_t period, void *intf_ptr);

int8_t getSensorDataForcedMode(lcd_t *lcd, struct bme280_dev *dev);

#endif /* MAIN_H_ */