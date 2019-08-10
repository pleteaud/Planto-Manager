/*
 * buffer.h
 *
 * Created: 7/6/2019 2:59:13 PM
 *  Author: plete
 */ 


#ifndef BUFFER_H_
#define BUFFER_H_

/************************************************************************/
/*							Includes	                                */
/************************************************************************/
#include "stdint.h"

/************************************************************************/
/*				Type Defs + Struct Declaration							*/
/************************************************************************/
typedef struct buffer_s buffer_t;

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
void bufferInit(buffer_t *bufP);
void *bufferSetData(buffer_t *bufP, uint8_t size);
void *bufferGetData(buffer_t *bufP, uint8_t size);
void bufferClear(buffer_t *buffP);
buffer_t* retrieveActiveBuffer(void);
uint8_t bufferGetCtrlStatus(const buffer_t *buffP);
uint8_t checkAvailableSpace(buffer_t *buffP);
#endif /* BUFFER_H_ */



