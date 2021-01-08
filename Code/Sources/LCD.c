/*
 * LCD.c
 *
 * Created: 8/8/2020 7:34:15 PM
 *  Author: plete
 * Influenced by the Liquid Crystal Library for Arduino https://github.com/arduino-libraries/LiquidCrystal
 */

/************************************************************************/
/*							Includes/Constants	 	                    */
/************************************************************************/

#include "LCD.h"
#include "timer.h"
#include "stdbool.h"

#define INSTRUCTION_FLAG			0x00
#define DATA_FLAG					0x01


// LCD Instructions 
#define CLEAR_DISPLAY				0x01
#define RETURN_HOME					0x02
#define ENTRY_MODE_SET				0x04
#define DISPLAY_CONTROL				0x08
#define CURSOR_SHIFT				0x10
#define FUNCTION_SET				0x20
#define SET_CGRAM_ADDR				0x40
#define SET_DDRAM_ADDR				0x80

// flags for display entry mode
#define ENTRY_RIGHT					0x00
#define ENTRY_LEFT					0x02
#define ENTRY_SHIFT_INCREMENT		0x01
#define ENTRY_SHIFT_DECREMENT		0x00

// flags for display on/off control
#define DISPLAY_ON					0x04
#define DISPLAY_OFF					0x00
#define CURSOR_ON					0x02
#define CURSOR_OFF					0x00
#define BLINK_ON					0x01
#define BLINK_OFF					0x00

// flags for display/cursor shift
#define DISPLAY_MOVE				0x08
#define CURSOR_MOVE					0x00
#define MOVE_RIGHT					0x04
#define MOVE_LEFT					0x00

// flags for function set
#define _8_BIT_DATA					0x10
#define _4_BIT_DATA					0x00
#define _2_LINE_MODE				0x08
#define _1_LINE_MODE				0x00
#define _5x11_FONT					0x04
#define _5x8_FONT					0x00

#define LINE1_ADDR_OFFSET			0x40

/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/
static void lcdWrite(lcd_t *lcdP, unsigned char data, uint8_t rsFlag);
static uint8_t lcdRead(lcd_t *lcdP, uint8_t rsFlag);

/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/
void lcdInit(lcd_t *lcdP, mcp23017_t *ioExpander, volatile uint8_t *ctrlDdr, volatile uint8_t *ctrlPort,
			 uint8_t rsPin, uint8_t rwPin, uint8_t enPin, bool lines, bool font)
{
	/*bool status = false;*/
	// Set Direction of Ctrl port Pins to output with value 0
	*ctrlDdr |= (1 << rsPin) | (1 << rwPin) | (1 << enPin);
	*ctrlPort &= ~((1 << rsPin) | (1 << rwPin) | (1 << enPin));
	
	// Set lcd data members 
	lcdP->ctrlDdr = ctrlDdr;
	lcdP->ctrlPort = ctrlPort;
	lcdP->rsPin = rsPin;
	lcdP->rwPin = rwPin;
	lcdP->enPin = enPin;
	lcdP->ioExpander = ioExpander;
	
	// Initialize io expander to have GPIO B as output
// 	mcpInit(&ioExpander, 0, &DDRB, &PORTB, PINB3);
// 	mcpSetPortDir(&ioExpander, MCP23017_PORTB, 0);

	/*** Process to initialize LCM Pg 16 of LCD1602A data sheet ***/
    milli_delay(50);						// power on delay for > 15ms 
	// Send three Function Set (8-bit data length) 
	lcdWrite(lcdP, FUNCTION_SET | _8_BIT_DATA | _1_LINE_MODE | _5x8_FONT, INSTRUCTION_FLAG);
	milli_delay(5);							// Wait for > 4.1ms
	lcdWrite(lcdP, FUNCTION_SET | _8_BIT_DATA | _1_LINE_MODE | _5x8_FONT, INSTRUCTION_FLAG);
	micro_delay(150);						// Wait for > 100uS
	lcdWrite(lcdP, FUNCTION_SET | _8_BIT_DATA | _1_LINE_MODE | _5x8_FONT, INSTRUCTION_FLAG);
	lcdP->functionSet = _8_BIT_DATA | (lines ? _2_LINE_MODE : _1_LINE_MODE ) | (font ? _5x11_FONT : _5x8_FONT);
	
	// Set data length, # lines, and font. # lines and font cant be changed after this point
	lcdWrite(lcdP, FUNCTION_SET | lcdP->functionSet, INSTRUCTION_FLAG);	// 0x38
	
	// Display off
	lcdNoDisplay(lcdP);					// 0x08
	// Display clear
	lcdClear(lcdP);						//0x01
	// Entry mode set
	lcdLeftToRight(lcdP);				//0x06
	/* Done initializing LCD */
	
	// Turn on display, cursor, and blink
	lcdDisplay(lcdP);
	lcdCursor(lcdP);
	lcdBlink(lcdP);
	
}

/**
*	Clear all the display data in all DDRAM address
*	Set address counter to 00H
*	Return cursor to top left
*/
void lcdClear(lcd_t *lcdP)
{
	lcdWrite(lcdP, CLEAR_DISPLAY, INSTRUCTION_FLAG);
	milli_delay(2);	// wait more than cmd execution time of 1.53ms 
}

/**
*	Set Address counter to DDRAM address "00H".
*	Return cursor to its original site and return display to its original status, if shifted
*/
void lcdHome(lcd_t *lcdP)
{
	lcdWrite(lcdP, RETURN_HOME, INSTRUCTION_FLAG);
	milli_delay(2);	// wait more than cmd execution time of 1.53ms
}

/* entire display is turned off (data not altered)*/
void lcdNoDisplay(lcd_t *lcdP)
{
	lcdP->displaycontrol &= ~DISPLAY_ON;
	lcdWrite(lcdP, DISPLAY_CONTROL | lcdP->displaycontrol, INSTRUCTION_FLAG);
	micro_delay(100);	// wait more than cmd execution time of 39uS
}

/* entire display is turned on*/
void lcdDisplay(lcd_t *lcdP)
{
	lcdP->displaycontrol |= DISPLAY_ON;
	lcdWrite(lcdP, DISPLAY_CONTROL | lcdP->displaycontrol, INSTRUCTION_FLAG);
	micro_delay(100);	// wait more than cmd execution time of 39uS
}

/*cursor doesn't blink*/
void lcdNoBlink(lcd_t *lcdP)
{
	lcdP->displaycontrol &= ~BLINK_ON;
	lcdWrite(lcdP, DISPLAY_CONTROL | lcdP->displaycontrol, INSTRUCTION_FLAG);
	micro_delay(100);	// wait more than cmd execution time of 39uS
}

/* cursor blinks*/
void lcdBlink(lcd_t *lcdP)
{
	lcdP->displaycontrol |= BLINK_ON;
	lcdWrite(lcdP, DISPLAY_CONTROL | lcdP->displaycontrol, INSTRUCTION_FLAG);
	micro_delay(100);	// wait more than cmd execution time of 39uS
}

/* cursor disappears, but I/D register is unchanged */
void lcdNoCursor(lcd_t *lcdP)
{
	lcdP->displaycontrol &= ~CURSOR_ON;
	lcdWrite(lcdP, DISPLAY_CONTROL | lcdP->displaycontrol, INSTRUCTION_FLAG);
	micro_delay(100);	// wait more than cmd execution time of 39uS
}

/* cursor turned on */
void lcdCursor(lcd_t *lcdP)
{
	lcdP->displaycontrol |= CURSOR_ON;
	lcdWrite(lcdP, DISPLAY_CONTROL | lcdP->displaycontrol, INSTRUCTION_FLAG);
	micro_delay(100);	// wait more than cmd execution time of 39uS
}

/*Shift all the display to the left, cursor moves according to the display*/
void lcdScrollDisplayLeft(lcd_t *lcdP)
{
	lcdWrite(lcdP, CURSOR_SHIFT | DISPLAY_MOVE | MOVE_LEFT, INSTRUCTION_FLAG);
	micro_delay(100);	// wait more than cmd execution time of 39uS
}


/*Shift all the display to the right, cursor moves according to the display*/
void lcdScrollDisplayRight(lcd_t *lcdP)
{
	lcdWrite(lcdP, CURSOR_SHIFT | DISPLAY_MOVE | MOVE_RIGHT, INSTRUCTION_FLAG);
	micro_delay(100);	// wait more than cmd execution time of 39uS
}

/* cursor/blink moves to the right and DDRAM address increases by 1 */
void lcdLeftToRight(lcd_t *lcdP)
{
	lcdP->entryModeSet |=  ENTRY_LEFT;
	lcdWrite(lcdP, ENTRY_MODE_SET | lcdP->entryModeSet, INSTRUCTION_FLAG);
	micro_delay(100);	// wait more than cmd execution time of 39uS
}

/* cursor/blink moves to the left and DDRAM address decreased by 1 */
void lcdRightToLeft(lcd_t *lcdP)
{
	lcdP->entryModeSet &=  ~ENTRY_LEFT;
	lcdWrite(lcdP, ENTRY_MODE_SET | lcdP->entryModeSet, INSTRUCTION_FLAG);
	micro_delay(100);	// wait more than cmd execution time of 39uS
}

/* shifting of entire display (according to id) is done when DDRAM write operation is executed */
void lcdAutoscroll(lcd_t *lcdP)
{
	lcdP->entryModeSet |=  ENTRY_SHIFT_INCREMENT;
	lcdWrite(lcdP, ENTRY_MODE_SET | lcdP->entryModeSet, INSTRUCTION_FLAG);
	micro_delay(100);	// wait more than cmd execution time of 39uS
}

/* shifting of entire display is not done when DDRAM read/write operation is done */
void lcdNoAutoscroll(lcd_t *lcdP)
{
	lcdP->entryModeSet &=  ~ENTRY_SHIFT_INCREMENT;
	lcdWrite(lcdP, ENTRY_MODE_SET | lcdP->entryModeSet, INSTRUCTION_FLAG);
	micro_delay(100);	// wait more than cmd execution time of 39uS
}

/**
*	Write binary 8-bit data to DDRAM/CGRAM. Selection of RAM is set by previous address set instruction
*/
void lcdPrint(lcd_t *lcdP, char *s)
{
	while (*s != '\0')
	{
		lcdWrite(lcdP, *s, DATA_FLAG);
		s++;
		micro_delay(100);	// wait more than cmd execution time of 43uS
	}
}

/**
*	Set DDRAM address to AC. There are 2 rows and 16 columns
*	@param	row: line number of LCD matrix.
*			0 = first line has value between 00H to 27H
			1 = second line has value between 40H to 67H 
*	@param	column: column number of LCD matrix
*			column = [0,16]
*/
void lcdSetCursor(lcd_t *lcdP, uint8_t row, uint8_t column)
{	
	if (row)
		lcdWrite(lcdP, SET_DDRAM_ADDR | LINE1_ADDR_OFFSET | column, INSTRUCTION_FLAG);		/* offset of C0 cause address 40H corresponds to {0011,AC5-AC0} */
	else	
		lcdWrite(lcdP, SET_DDRAM_ADDR | column, INSTRUCTION_FLAG);		/*  offset of 80H cause address 0 corresponds to {001,AC6-AC0} */
/* Future update will take into consideration if there's only one line */
	micro_delay(100);	// wait more than cmd execution time of 39uS
}

/* Build Symbol */
void lcdBuildSym(lcd_t *lcdP, uint8_t location, unsigned char *ptr)
{
	if (location < 8)
	{
		lcdWrite(lcdP, SET_CGRAM_ADDR + location * 8, INSTRUCTION_FLAG);
		micro_delay(100);
		
		for (int i = 0; i < 8; i++)
		{
			lcdWrite(lcdP, ptr[i], DATA_FLAG);
			micro_delay(100);	// wait more than cmd execution time of 43uS
		}
	}
}

/* Print Symbol*/
void lcdPrintSymbol(lcd_t *lcdP, uint8_t location)
{
	lcdWrite(lcdP, location, DATA_FLAG);
	micro_delay(100);	// wait more than cmd execution time of 43uS
}

/** 
*	Shows whether SPLC780D is in internal operation or not 
*	@ret	busyFlagAddr: stores busy flag and AC addess counter 
*/
uint8_t lcdReadBSYandAddr(lcd_t *lcdP)
{
	uint8_t busyFlagAddr = lcdRead(lcdP, INSTRUCTION_FLAG);
	return busyFlagAddr;
	// 0 us execution time
}

/** 
*	Read binary 8-bit data from DDRAM/CGRAM. Selection of RAM is set by previous address set instruction
*/
uint8_t lcdReadData(lcd_t *lcdP)
{
	return lcdRead(lcdP, DATA_FLAG);
	micro_delay(100);	// wait more than cmd execution time of 43uS
}

/************************************************************************/
/*                     Private Functions Implementation                 */
/************************************************************************/

/* Write data to lcd */
static void lcdWrite(lcd_t *lcdP, unsigned char data, uint8_t rsFlag)
{
	if (rsFlag)
		*lcdP->ctrlPort |= (1 << lcdP->rsPin);			//it is data rather than an instruction
	else
		*lcdP->ctrlPort &= ~(1 << lcdP->rsPin);			//it is an instruction rather than data

	*lcdP->ctrlPort &= ~(1 << lcdP->rwPin);				//it is write operation
	mcp23017SetPortLevel(lcdP->ioExpander, MCP23017_PORTB, data);	//put the instruction on the data bus
	*lcdP->ctrlPort &= ~(1 << lcdP->enPin);				// assure E is cleared
	micro_delay(1);
	*lcdP->ctrlPort |= (1 << lcdP->enPin);				//set E to 1 (see Figure 1)
	micro_delay(1);										// need to be on for > 230ns
	*lcdP->ctrlPort &= ~(1 << lcdP->enPin);				// set E to 0 to generate a falling edge
}

/* Read data to lcd */
static uint8_t lcdRead(lcd_t *lcdP, uint8_t rsFlag)
{
	milli_delay(2);
	uint8_t val = 0;
	if (rsFlag)
		*lcdP->ctrlPort |= (1 << lcdP->rsPin);				//it is data rather than an instruction
	else
		*lcdP->ctrlPort &= ~(1 << lcdP->rsPin);				//it is an instruction rather than data

	*lcdP->ctrlPort |= (1 << lcdP->rwPin);					//it is a read operation
	*lcdP->ctrlPort &= ~(1 << lcdP->enPin);					// assure E is cleared
	micro_delay(1);
	*lcdP->ctrlPort |= (1 << lcdP->enPin);					//set E to 1 (see Figure 1)
	micro_delay(1);											// need to be on for > 230ns
	*lcdP->ctrlPort &=  ~(1 << lcdP->enPin);				// set E to 0 to generate a falling edge
	mcpReadPortLevel(lcdP->ioExpander, MCP23017_PORTB, &val);	// Read port level
	return val;
}


