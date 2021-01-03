/*
 * LCD.c
 *
 * Created: 8/8/2020 7:34:15 PM
 *  Author: plete
 */ 

#include "LCD.h"
#include "timer.h"
#include "stdbool.h"
#define TOTAL_CTRL_PINS			(0x03)

#define ID_FLAG					(0x02)
#define SH_FLAG					(0x01)

#define DISPLAY_CTRL_FLAG		(0x04)
#define CURSOR_CTRL_FLAG		(0x02)
#define BLINK_CTRL_FLAG			(0x01)

#define SHIFT_PATTERN_1			(0x00)
#define SHIFT_PATTERN_2			(0x01)
#define SHIFT_PATTERN_3			(0x02)
#define SHIFT_PATTERN_4			(0x03)

#define INTERFACE_DATA_LEN_FLAG	(0x10)
#define DISPLAY_LINE_NUM_FLAG	(0x08)
#define FONT_CTRL_FLAG			(0x04)

#define INSTRUCTION_FLAG		(0x00)
#define DATA_FLAG				(0x01)

#define LCD_DATA_DDR			DDRD
#define LCD_DATA_PORT			PORTD
#define LCD_CTRL_DDR 			DDRB
#define LCD_CTRL_PORT			PORTB

#define RS_PIN_NUM				PINB0		
#define RW_PIN_NUM				PINB1
#define EN_PIN_NUM				PINB2

static void lcdWrite(unsigned char data, uint8_t rsFlag);
static uint8_t lcdRead(uint8_t rsFlag);

void lcdInit()
{
	/* Set Data Direction for Data Ports and Ctrl Pins */
	LCD_DATA_DDR = 0xFF;
	LCD_DATA_PORT = 0x00;
	LCD_CTRL_PORT &= ~((1 << RS_PIN_NUM) | (1 << RW_PIN_NUM) | (1 << EN_PIN_NUM));
	LCD_CTRL_DDR |= (1 << RS_PIN_NUM) | (1 << RW_PIN_NUM) | (1 << EN_PIN_NUM);
	

	/* Turn Display ON, Cursor ON, Cursor Blink ON */
	lcdWrite(12,INSTRUCTION_FLAG);
	/* Clear and reset the Display */
	lcdClearDisplay();
	/* Move cursor back to top left cell (first cell) */
	lcdSetDDRAMAdrr(0,0);
	/* Set the cursor moving direction to the right: I/D high, SH Low */
	lcdWrite(6,INSTRUCTION_FLAG);
	/* Set Function Set as 8-bit data bus, two-line display */
	lcdWrite(0x38,INSTRUCTION_FLAG);
}

/* Set Address counter to DDRAM address "00H". */
/* Return cursor to its original site and return display to its orignial status, if shifted*/
void lcdReturnHome(void)
{
	milli_delay(2);
	lcdWrite(0x02,INSTRUCTION_FLAG);        	
}

/*Clear all the display data in all DDRAM address */
/* Set address counter to 00H */
/* Return cursor to top left */
void lcdClearDisplay(void)
{
	milli_delay(2);
	lcdWrite(0x01, INSTRUCTION_FLAG);       
}

void lcdEntryModeSet(bool id, bool sh)
{
	milli_delay(2);
	uint8_t instruc = 0x04; /* 0000 0100 */
	if (id){instruc |= ID_FLAG;}
	if (sh){instruc |= SH_FLAG;}
	lcdWrite(instruc,INSTRUCTION_FLAG); 
}

/* Control display/cursor/blink ON/OFF 1 bit register */
void lcdDisplayCtrl(bool displayCtrl, bool cursorCtrl, bool blinkCtrl)
{
	milli_delay(2);
	uint8_t instruc = 0x08; /* 0000 1000 */
	if (displayCtrl){instruc |= DISPLAY_CTRL_FLAG;}
	if (cursorCtrl){instruc |= CURSOR_CTRL_FLAG;}
	if (blinkCtrl){instruc |= BLINK_CTRL_FLAG;}
	lcdWrite(instruc,INSTRUCTION_FLAG);
}

void lcdCursorDisplayShift(uint8_t shiftPattern)
{
	milli_delay(2);
	uint8_t instruc = 0x10 | shiftPattern; 
	lcdWrite(instruc,INSTRUCTION_FLAG);
}

void lcdFunctionSet(bool dataLength,bool displayLine, bool fontCtrl)
{
	milli_delay(2);
	uint8_t instruc = 0x20;
	if (dataLength){instruc |= INTERFACE_DATA_LEN_FLAG;}
	if (displayLine){instruc |= DISPLAY_LINE_NUM_FLAG;}
	if (fontCtrl){instruc |= FONT_CTRL_FLAG;}
	lcdWrite(instruc,INSTRUCTION_FLAG);
}

void lcdSetDDRAMAdrr(uint8_t row, uint8_t column)
{
	milli_delay(2);
	if(row==0)
	lcdWrite(0x80 + column,INSTRUCTION_FLAG);		/* Line 1 has address value between 00H to 27H */
	if(row==1)
	lcdWrite(0xC0+ column,INSTRUCTION_FLAG);		/* Line 2 has address value between 40H to 67H */
	/* Future update will take into consideration if there's only one line */
}

void lcdSetCGRAMAdrr(uint8_t column)
{
	milli_delay(2);
	lcdWrite(0x40 + column,INSTRUCTION_FLAG);		/* Line 1 has address value between 00H to 27H */
}


uint8_t lcdReadBSYandAddr()
{
	milli_delay(2);
	return lcdRead(INSTRUCTION_FLAG);
}

uint8_t lcdReadData()
{
	milli_delay(2);
	return lcdRead(DATA_FLAG);
}

void lcdWriteString(char *s)
{
	milli_delay(2);
	while (*s != '\0')
	{
		lcdWrite(*s,DATA_FLAG);
		s++;
	}
}
void lcdBuildSym(uint8_t location, unsigned char *ptr)
{
	if (location < 8)
	{
		lcdSetCGRAMAdrr(location*8);
		for (int i = 0; i < 8; i++)
		{
			lcdWrite(ptr[i],DATA_FLAG);
		}
	}
}
void lcdWriteSymbol(uint8_t location)
{
	lcdWrite(location,DATA_FLAG);
}
static void lcdWrite(unsigned char data, uint8_t rsFlag)
{
	milli_delay(2);
	if (rsFlag)
		LCD_CTRL_PORT = LCD_CTRL_PORT | (1 << RS_PIN_NUM);	//it is data rather than an instruction
	else
		LCD_CTRL_PORT = LCD_CTRL_PORT & ~(1 << RS_PIN_NUM) ;	//it is an instruction rather than data

	LCD_CTRL_PORT = LCD_CTRL_PORT & ~(1 << RW_PIN_NUM);	//it is write operation
	LCD_CTRL_PORT = LCD_CTRL_PORT & ~(1 << EN_PIN_NUM);	//set E to 0 (see Figure 1)
	LCD_DATA_PORT = data;			//put the instruction on the data bus
	LCD_CTRL_PORT = LCD_CTRL_PORT | (1 << EN_PIN_NUM);	//set E to 1 (see Figure 1)
	LCD_CTRL_PORT = LCD_CTRL_PORT & ~(1 << EN_PIN_NUM);	// set E to 0 to generate a falling edge
}
static uint8_t lcdRead(uint8_t rsFlag)
{
	milli_delay(2);
	uint8_t val = 0;
	if (rsFlag)
		LCD_CTRL_PORT = LCD_CTRL_PORT | (1 << RS_PIN_NUM);	//it is data rather than an instruction
	else
		LCD_CTRL_PORT = LCD_CTRL_PORT & ~(1 << RS_PIN_NUM) ;	//it is an instruction rather than data
		
	LCD_CTRL_PORT = LCD_CTRL_PORT | (1 << RW_PIN_NUM);		//it is read operation
	LCD_CTRL_PORT = LCD_CTRL_PORT & ~(1 << EN_PIN_NUM);	//set E to 0 (see Figure 1)
	LCD_CTRL_PORT = LCD_CTRL_PORT | (1 << EN_PIN_NUM);	//set E to 1 (see Figure 1)
	LCD_CTRL_PORT = LCD_CTRL_PORT & ~(1 << EN_PIN_NUM);	// set E to 0 to generate a falling edge
	val = LCD_DATA_PORT;		/* Read data bus; May need a micro second delay */
	return val;
}
