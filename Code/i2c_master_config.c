/*
 * i2c_master_config.c
 *
 * Created: 7/16/2019 9:57:40 PM
 *  Author: plete
 */ 

#include "i2c_master_config.h"
#include "buffer.h"

#define MAX_ERRORS		(0x78)

typedef struct i2cMaster_callback_s
{
	void *objP;
	i2c_operations_t (*i2cCB)(void *objP);
} i2cMaster_callback_t;

typedef struct i2cMaster_s{
	i2cMaster_callback_t  i2cDataCB;
	i2cMaster_callback_t i2cStopCB;
	i2cMaster_callback_t i2cErrCB;
	uint8_t errorList[MAX_ERRORS];
	int errorListCount;
}i2cMaster_t;

i2cMaster_t i2cMaster;

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
	if(I2C_0_master_operation(false) == I2C_NOERR)
	{
		status = true;
	}
	return status;
}
i2c_operations_t i2cMasterReturnStopCb(void *p)
{
	bufferTransferToMaster(retrieveActiveBuffer());
	cmdProcCmdDone();
	
	
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
i2c_operations_t i2cWriteCollisionErrCB(void *p)
{
	return i2cMaster.i2cErrCB(i2c_writeCollision);
}
i2c_operations_t i2cTimeoutErrCB(void *p)
{
	return i2cMaster.i2cErrCB(i2c_timeOut);
}
i2c_operations_t i2cAdrrNackCB(void *p)
{
	return i2cMaster.i2cErrCB(i2c_addressNACK);
}
i2c_operations_t i2cDataNackCB(void *p)
{
	return i2cMaster.i2cErrCB(i2c_dataNACK);
}

void setDataCompleteCallback(void (*funcP)(void *objP),void *obj)
{
	i2cMaster.i2cDataCB.i2cCB = funcP;
	i2cMaster.i2cDataCB.objP = obj;
}
void setStopCallback(void (*funcP)(void *objP),void *obj)
{
	i2cMaster.i2cStopCB.i2cCB = funcP;
	i2cMaster.i2cStopCB.objP = obj;
}

i2c_operations_t i2cFailureCB(void *error)
{
	i2cMaster.errorList[i2cMaster.errorListCount++] = &error;
	return i2c_stop;
}