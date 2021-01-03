/*
 * buffer.c
 *
 * Created: 7/6/2019 3:52:15 PM
 *  Author: plete
 */

/************************************************************************/
/*						Includes/Constants			                    */
/************************************************************************/
#include "buffer.h"
#include "stddef.h"

#define MAX_BUFFER_SIZE			(0x40)

#define BUFFER_OWNER_MASK		(0x01)
#define BUFFER_OWNER_POS		(0x00)
#define BUFFER_OWNER			(BUFFER_OWNER_MASK << BUFFER_OWNER_POS)

#define BUFFER_OVR_MASK			(0x01)
#define BUFFER_OVR_POS			(0x01)
#define BUFFER_OVR				(BUFFER_OVR_MASK << BUFFER_OVR_POS)

#define BUFFER_UND_MASK			(0x01)
#define BUFFER_UND_POS			(0x02)
#define BUFFER_UND				(BUFFER_UND_MASK << BUFFER_UND_POS)

enum
{
	BUFFER_NO_ERROR = 0x00,
	BUFFER_OWNER_MASTER = 0x00,
	BUFFER_OWNER_SLAVE  = 0x01
};



/************************************************************************/
/*						Struct Implementation		                    */
/************************************************************************/
/* Circular Buffer */
struct buffer_s
{
	uint8_t buffer[MAX_BUFFER_SIZE];
	uint8_t *dataInP;
	uint8_t *dataOutP;
	uint8_t flags;
	uint8_t count;
};

/************************************************************************/
/*						Private Variables			                    */
/************************************************************************/
static buffer_t buffer;

/************************************************************************/
/*					Public Functions Implementation	                    */
/************************************************************************/
/* Retrieve the current active buffer object */
/* Function will soon be deprecated */ 
buffer_t* retrieveActiveBuffer(void)
{
	return &buffer;
}

/* Determine who owns the buffer currently */
/* Owner is Master if Atmega chip is the one sending a command. */
/* Owner is Slave when device being communicated to is responding to a command */
uint8_t bufferGetCtrlStatus(const buffer_t *buffP)
{
	return (((buffP->flags & BUFFER_OWNER) == BUFFER_OWNER_MASTER) ? (BUFFER_OWNER_MASTER) : (BUFFER_OWNER_SLAVE));
}

/* Initialize buffer object */
void bufferInit(buffer_t *buffP)
{
	buffP->dataInP = buffP->buffer;
	buffP->dataOutP = buffP->buffer;
	buffP->flags = BUFFER_OWNER_MASTER; // Master has initial control over buffer 
	buffP->count = 0;
}

/* Reserve space within buffer for the amount of data that will be added */
/* Size determines how much data will be reserved */
void *bufferSetData(buffer_t *buffP, uint8_t size)
{
	uint8_t freeSpace = checkAvailableSpace(buffP);
	uint8_t *dataP;
	if(freeSpace == 0 || freeSpace < size)
	{	
		/* There's not enough space to put any more data */
		dataP = NULL;
		buffP->flags |= BUFFER_OVR;
	}
	/* If there's free space */
	else if(freeSpace >= size)
	{
		dataP = buffP->dataInP;
		buffP->dataInP += size;
		buffP->count += size;
	}
	return dataP;
}

/* Reserve space within buffer for the amount of data that will be removed */
/* Size determines how much data will be removed */
void *bufferGetData(buffer_t *buffP, uint8_t size)
{
	uint8_t filledSpace = buffP->count;
	uint8_t *dataP;
	if(filledSpace == 0 || filledSpace < size)
	{
		/* Indicate underflow (removing empty spaces) */
		dataP = NULL;
		buffP->flags |=  BUFFER_UND;
	}
	/* Retrieve current */ 
	else if(filledSpace >= size)
	{
		dataP = buffP->dataOutP;
		buffP->dataOutP += size;
		buffP->count -= size;
		
	}
	return dataP;
}

/* Transfer control of buffer to slave device */
void bufferTransferToSlave(buffer_t *buffP)
{
	buffP->flags = (buffP->flags & ~BUFFER_OWNER) | (BUFFER_OWNER_SLAVE << BUFFER_OWNER_POS);
}

/* Transfer control of buffer to master device */
void bufferTransferToMaster(buffer_t *buffP)
{
	buffP->flags = (buffP->flags & ~BUFFER_OWNER) | (BUFFER_OWNER_MASTER << BUFFER_OWNER_POS);
}

/* Clear buffer contents */
void bufferClear(buffer_t *buffP)
{
	buffP->dataInP = buffP->dataOutP = buffP->buffer;
	buffP->count = 0;	
}

uint8_t checkAvailableSpace(buffer_t *buffP)
{
	return MAX_BUFFER_SIZE-buffP->count;
}
