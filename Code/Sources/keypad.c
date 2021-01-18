/*
 * keypad.c
 *
 * Created: 1/10/2021 5:23:28 PM
 *  Author: plete
 */

 
/************************************************************************/
/*							Includes/Constants	 	                    */
/************************************************************************/
#include "keypad.h"
#include "timer.h"

#define DEBOUNCE_TIME				5		//ms
#define HOLD_TIME					250		//ms
#define MAX_COUNT					HOLD_TIME / DEBOUNCE_TIME
#define ROWS_SIZE					4
#define COLUMN_SIZE					4
#define INVALID_VAL					255

unsigned char keymap[ROWS_SIZE][COLUMN_SIZE] = {{'1','2','3','A'},
												{'4','5','6','B'},
												{'7','8','9','C'},
												{'*','0','#','D'}};

/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/
static void poll(keypad_t *keypadP, char *s);
static void mapToArrIndex(uint8_t *rowOrCol);
static void keypadReset(keypad_t *keypadP);
static uint8_t readCol(void);
static uint8_t readRow(void);

/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/

/* Initialize a keyboard object */
void keypadInit(keypad_t *keypadP)
{
	keypadP->row = keypadP->column = 0;
	keypadP->count = MAX_COUNT;
	keypadP->lastPollTime = 0;
}

/* Get a key press. nonblocking */
char getKeyPress(keypad_t *keypadP)
{	
	uint32_t currTime = getMillis();
	int64_t elapseTime = currTime - keypadP->lastPollTime;
	char s = '\0';
	
	// fix dT range if last time is less
	if (elapseTime < 0)
		elapseTime = (MAX_INT32_VAL - keypadP->lastPollTime) + currTime;
	
	if (elapseTime > 5)
	{	
		keypadP->lastPollTime = currTime;
		poll(keypadP, &s);
	}
		
	return s;
}

/************************************************************************/
/*                     Private Functions Implementation                 */
/************************************************************************/

/* Poll for key press with debounce and hold time */
static void poll(keypad_t *keypadP, char *s)
{	
	uint8_t tempCol = readCol();	// read column input
	uint8_t tempRow = readRow();	// read row input
	
	mapToArrIndex(&tempCol);			// map temp column to keypad array index 
	mapToArrIndex(&tempRow);			// map temp row to keypad array index 
			
	if (tempRow == INVALID_VAL && tempCol == INVALID_VAL)
	{
		//no push or invalid push
		keypadReset(keypadP);
		return;
	}
			
	if (keypadP->count == MAX_COUNT)
	{
		keypadP->column = tempCol;
		keypadP->row = tempRow;
		keypadP->count--;
		return;
	}
			
	else if (keypadP->count > 0)
	{
		// check if new column or row is the same as previous
		if (tempCol == keypadP->column && tempRow == keypadP->row)
			keypadP->count--;
		else 
			keypadReset(keypadP);
			
		return;
	}						

	*s = keymap[keypadP->row][keypadP->column]; // get key from keymap
	
	keypadReset(keypadP);	// Reset values for next poll
}

 /* Read column input */
 static uint8_t readCol(void)
 {
	PORTD =  0xF0;			// set row to high
	DDRD  = 0xF0;			//set column pins (0-3) as input and row pins as output (4-7)
	return PIND & 0x0F;		// read column input
 }
 /* Read row input */
 static uint8_t readRow(void)
 {
	PORTD =  0x0F;			// set col to high
	DDRD  = 0x0F;			//set column pins (0-3) as output and row pins as intput (4-7)
	return (PIND & 0xF0) >> 4;
 }
 
 /* Reset Keypad object */
static void keypadReset(keypad_t *keypadP)
{
	keypadP->count = MAX_COUNT;
	keypadP->column = 0;
	keypadP->row = 0;
	keypadP->lastPollTime = 0;
}

static void mapToArrIndex(uint8_t *rowOrCol)
{
	//map row or col to keypad array index 
	switch (*rowOrCol)
	{
		case 1:
			*rowOrCol = 3;
			break;
		case 2:
			*rowOrCol = 2;
			break;
		case 4:
			*rowOrCol = 1;
			break;
		case 8:
			*rowOrCol = 0;
			break;
		default:
			*rowOrCol = INVALID_VAL;
	}
}
