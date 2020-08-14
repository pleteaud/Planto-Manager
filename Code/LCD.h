/*
 * LCD.h
 *
 * Created: 8/8/2020 3:42:40 PM
 *  Author: plete
 */ 


#ifndef LCD_H_
#define LCD_H_

#include "stdint.h"
#include "stdbool.h"
void lcdInit();
void lcdReturnHome(void);
void lcdClearDisplay(void);
void lcdEntryModeSet(bool id, bool sh);
void lcdDisplayCtrl(bool displayCtrl, bool cursorCtrl, bool blinkCtrl);
void lcdCursorDisplayShift(uint8_t shiftPattern);
void lcdFunctionSet(bool dataLength,bool displayLine, bool fontCtrl);
void lcdSetDDRAMAdrr(uint8_t row, uint8_t column);
void lcdSetCGRAMAdrr(uint8_t column);
uint8_t lcdReadBSYandAddr();
uint8_t lcdReadData();
void lcdWriteString(char *s);
void lcdBuildSym(uint8_t location, unsigned char *ptr);
void lcdWriteSymbol(uint8_t location);

//void lcdWriteString(char *s);
#endif /* LCD_H_ */

