/*
 * i2cMaster.h
 *
 * Created: 7/15/2019 9:46:46 PM
 *  Author: plete
 */ 


#ifndef I2C_MASTER_CONFIG_H_
#define I2C_MASTER_CONFIG_H_

#include "i2c_master.h"
#include "stdbool.h"
//
//static i2c_operations_t *i2cDataCompleteCb;
//static i2c_operations_t *i2cWriteCollisionCb;
//static i2c_operations_t *i2cAddrNackCb;
//static i2c_operations_t *i2cDataNackCb;
//static i2c_operations_t *i2cTimeoutCb;

/* by default all the callbacks return stop. However, this abstraction allows the other classes to safely alter the callback f(x)'s safely */

void i2cMasterInit(uint8_t slaveAddress);
//void i2cSetCallBack(i2c_callback_index cbIndex, i2c_operations_t cbFunction);
bool i2cMasterRead(uint8_t *buffP,uint8_t dataSize);
bool i2cMasterTransmit(uint8_t *payload,uint8_t dataSize);




#endif /* I2CMASTER_H_ */