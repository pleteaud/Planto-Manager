/*
 * i2c_master_config.c
 *
 * Created: 7/16/2019 9:57:40 PM
 *  Author: plete
 */ 

#include "i2c_master_config.h"
#include "buffer.h"

void i2cMasterInit(uint8_t slaveAddress)
{
	I2C_0_open(slaveAddress);	
}
bool i2cMasterRead(uint8_t *buffP, uint8_t dataSize)
{
	bool status = false;
	/* Set buffer */
	I2C_0_set_buffer(buffP, dataSize);
	
	/* Start I2C read */
	if(I2C_0_master_operation(true) != I2C_BUSY)
	{
		 status = true;
	}
	return status;
}
bool i2cMasterTransmit(uint8_t *payload,uint8_t dataSize)
{
	bool status = false;
	
	/* Set buffer */
	I2C_0_set_buffer(payload, dataSize);
	
	/* Start I2C write */
	if(I2C_0_master_operation(false) != I2C_BUSY || I2C_0_master_operation(false) != I2C_FAIL)
	{
		status = true;
	}
	return status;
}
i2c_operations_t i2cMasterReturnStopCb(void *p)
{
	bufferTransferToMaster(retrieveActiveBuffer());
	return i2c_stop;
}

i2c_operations_t i2cMasterReturnResetCb(void *p)
{
	return i2c_reset_link;
}

i2c_operations_t i2cMasterRestartWriteCb(void *p)
{
	return i2c_restart_write;
}

i2c_operations_t i2cMasterRestartReadCb(void *p)
{
	return i2c_restart_read;
}
