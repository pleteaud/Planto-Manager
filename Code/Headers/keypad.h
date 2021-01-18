/*
 * keypad.h
 *
 * Created: 1/10/2021 3:36:03 PM
 *  Author: plete
 */ 


#ifndef KEYPAD_H_
#define KEYPAD_H_

#include "mcp23017.h"

typedef struct keypad_s
{
	uint8_t row; 
	uint8_t column;
	uint16_t count;
	uint32_t lastPollTime;
} keypad_t;
					
void keypadInit(keypad_t *keypadP);
char getKeyPress(keypad_t *keypadP);


#endif /* KEYPAD_H_ */