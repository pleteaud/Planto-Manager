/*
 * i2cMasterControl.c
 *
 * Created: 7/16/2019 9:57:40 PM
 *  Author: plete
 */ 

/************************************************************************/
/*							Includes/Constants	 	                    */
/************************************************************************/
#include "i2cMasterControl.h"
#include "cmd_proc.h"
#include <driver_init.h>

#define MAX_ERRORS		(0x78)
/************************************************************************/
/*							Enums Definition		 	                */
/************************************************************************/
enum
{
	writeCollision = 1,
	addressNACK,
	dataNACK,
	timeOut,
	reset
};
/************************************************************************/
/*                      Private Variables                               */
/************************************************************************/
uint8_t errorList[MAX_ERRORS];
int errorListCount;
/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/
static i2c_operations_t i2cMasterReturnStopCb(void *p);
static i2c_operations_t i2cMasterReturnResetCb(void *p);
static i2c_operations_t i2cMasterRestartWriteCb(void *p);
static i2c_operations_t i2cMasterRestartReadCb(void *p);
static i2c_operations_t i2cWriteCollisionErrCB(void *p);
static i2c_operations_t i2cAdrrNackCB(void *p);
static i2c_operations_t i2cTimeoutErrCB(void *p);
static i2c_operations_t i2cDataNackCB(void *p);
/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/
/* Initialize I2C; Set up address; Assign callback functions */
void i2cMasterInit(uint8_t slaveAddress)
{
	I2C_0_open(slaveAddress);
	I2C_0_set_data_complete_callback(i2cMasterReturnStopCb,NULL);
	I2C_0_set_write_collision_callback(i2cWriteCollisionErrCB,NULL);	
	I2C_0_set_address_nack_callback(i2cAdrrNackCB, NULL);
	I2C_0_set_data_nack_callback(i2cDataNackCB, NULL);
	I2C_0_set_timeout_callback(i2cTimeoutErrCB,NULL);	
}

/* Initiate a I2C RX transaction. If I2C is busy, return false */
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
/* Initiate a I2C TX transaction. If I2C is busy, return false */
bool i2cMasterTransmit(uint8_t *payload,uint8_t dataSize)
{
	bool status = false;
	
	/* Set buffer */
	I2C_0_set_buffer(payload, dataSize);
	
	/* Start I2C write */
	if(I2C_0_master_operation(false) != I2C_BUSY){status = true;}
	return status;
}
/* Reset I2C Control Registers for next communication */
void resetI2c()
{
	// Reset bus by sending STOP
	TWCR = ((1 << TWSTO) | (1 << TWINT));
	// Reset module
	TWCR = (1 << TWINT) | (1 << TWEN);
	// uncomment the IRQ enable for an interrupt driven driver.
	TWCR |= (1 << TWIE);
}

/************************************************************************/
/*                     Private Functions Implementation                 */
/************************************************************************/
/* Callback function to handle the returning stop state of I2C.*/
/* Evokes cmdProcCmdDone, to indicate transaction is done */
static i2c_operations_t i2cMasterReturnStopCb(void *p)
{
	cmdProcCmdDone();
	return i2c_stop;
}

static i2c_operations_t i2cMasterReturnResetCb(void *p)
{
	return i2c_reset_link;
}

static i2c_operations_t i2cMasterRestartWriteCb(void *p)
{
	return i2c_restart_write;
}

static i2c_operations_t i2cMasterRestartReadCb(void *p)
{
	return i2c_restart_read;
}
static i2c_operations_t i2cWriteCollisionErrCB(void *p)
{
	errorList[errorListCount++] = writeCollision;
	return i2c_stop;
}
static i2c_operations_t i2cAdrrNackCB(void *p)
{
	errorList[errorListCount++] = addressNACK;
	return i2c_stop;
}
static i2c_operations_t i2cTimeoutErrCB(void *p)
{
	errorList[errorListCount++] = timeOut;
	return i2c_reset_link;
}

static i2c_operations_t i2cDataNackCB(void *p)
{
	errorList[errorListCount++] = dataNACK;
	return i2c_stop;
}
