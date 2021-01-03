/*
 * LCD.h
 *
 * Created: 8/8/2020 3:42:40 PM
 *  Author: plete
 * Influenced by the Liquid Crystal Library for Arduino https://github.com/arduino-libraries/LiquidCrystal
 */ 


#ifndef LCD_H_
#define LCD_H_

/************************************************************************/
/*							Includes/Constants	 	                    */
/************************************************************************/
#include "stdint.h"
#include "stdbool.h"
#include "mcp23017.h"

/************************************************************************/
/*				Type Defs + Struct Declaration							*/
/************************************************************************/
typedef struct lcd_s
{
	/* Control Pins */
	volatile uint8_t *ctrlDdr;
	volatile uint8_t *ctrlPort;
	uint8_t rsPin, rwPin, enPin;
	/* I2C expander acts as data pins */ 
	mcp23017_t ioExpander;
	
	/* LCD configuration variables */
	uint8_t entryModeSet;
	uint8_t displaycontrol;
	uint8_t cursorDisplayShift;
	uint8_t functionSet;

} lcd_t;

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
void lcdInit(lcd_t *lcdP, volatile uint8_t *ctrlDdr, volatile uint8_t *ctrlPort,
			 uint8_t rwPin, uint8_t rsPin, uint8_t enPin, bool lines, bool font);
			 
void lcdClear(lcd_t *lcdP);

void lcdHome(lcd_t *lcdP);

void lcdNoDisplay(lcd_t *lcdP);

void lcdDisplay(lcd_t *lcdP);

void lcdNoBlink(lcd_t *lcdP);

void lcdBlink(lcd_t *lcdP);

void lcdNoCursor(lcd_t *lcdP);

void lcdCursor(lcd_t *lcdP);

void lcdScrollDisplayLeft(lcd_t *lcdP);

void lcdScrollDisplayRight(lcd_t *lcdP);

void lcdLeftToRight(lcd_t *lcdP);

void lcdRightToLeft(lcd_t *lcdP);

void lcdAutoscroll(lcd_t *lcdP);

void lcdNoAutoscroll(lcd_t *lcdP);

void lcdPrint(lcd_t *lcdP, char *s);

void lcdSetCursor(lcd_t *lcdP, uint8_t row, uint8_t column);

void lcdBuildSym(lcd_t *lcdP, uint8_t location, unsigned char *ptr);

void lcdPrintSymbol(lcd_t *lcdP, uint8_t location);

uint8_t lcdReadBSYandAddr(lcd_t *lcdP);

uint8_t lcdReadData(lcd_t *lcdP);

//void lcdWriteString(char *s);
#endif /* LCD_H_ */

