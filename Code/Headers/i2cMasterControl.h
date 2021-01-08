/*
 * i2cMasterControl.h
 *
 * Created: 7/15/2019 9:46:46 PM
 *  Author: plete
 */ 


#ifndef I2C_MASTER_CONFIG_H_
#define I2C_MASTER_CONFIG_H_
/************************************************************************/
/*							Includes/Constants	 	                    */
/************************************************************************/
#include "i2c_master.h"
#include <stdbool.h>
/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
void i2cMasterInit(const uint8_t slaveAddress);
//bool i2cMasterRead(uint8_t *buffP,uint8_t dataSize);
bool i2cMasterRead(const uint8_t newAddr, uint8_t *buffP, const uint8_t size);
//bool i2cMasterTransmit(uint8_t *payload,uint8_t dataSize);
bool i2cMasterTransmit(const uint8_t newAddr, uint8_t *payload, const uint8_t size);
void i2cMasterChangeAddr(const uint8_t newAddr);
void resetI2c(void);
bool returnBusy();
#endif /* I2CMASTER_H_ */