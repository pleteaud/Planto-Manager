#include "main.h"

//dht11_sensor_t rSensor;
//soil_moisture_sensor_t soilSensor;
void printTime();

/* Instantiate device objects */
lcd_t lcd;
rtc_manager_t rtc;


/* Custom Pattern byte for clock */
unsigned char clockSymbol[] = {0x0,0xe,0x15,0x17,0x11,0xe,0x0,0x00};
uint8_t clkSymLoc = 0;
unsigned char calendarSymbol[] = {0x00,0x11,0x1F,0x13,0x1F,0x1F,0x00,0x00};
uint8_t calSymLoc = 1;

/* Custom pattern bytes for BME sensor */
unsigned char thermoSym[] = {0x04,0x0A,0x0A,0x0E,0x0E,0x1F,0x1F,0x0E};
uint8_t thermSymLoc = 2;
unsigned char degreeSym[] = {0x0C,0x12,0x12,0x0C,0x00,0x00,0x00,0x00};
uint8_t degreeSymLoc = 3;
unsigned char humiditySym[] = {0x04,0x04,0x0A,0x0A,0x11,0x11,0x11,0x0E};
uint8_t humiditySymLoc = 4;

int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();

	i2cMasterInit(0);
	
	/* Initialize LCD */
	lcdInit(&lcd, &DDRB, &PORTB, PINB0, PINB1, PINB2, true, false);	
	
	// Indicate temperature sensor
	lcdSetCursor(&lcd, 0,0);
	lcdPrint(&lcd, "  Initializing  ");
	lcdSetCursor(&lcd, 1,0);
	lcdPrint(&lcd, " BME280 Sen ");
	lcdClear(&lcd);
	/* init bme sensor */
	
	/* Initialize and Configure RTC */
	lcdSetCursor(&lcd, 0,0);
	lcdPrint(&lcd, "  Initializing  ");
	lcdSetCursor(&lcd, 1,0);
	lcdPrint(&lcd, " Real Time Clock ");
	milli_delay(1000);
	rtcInit(&rtc);
	lcdClear(&lcd);
	
	/* Build and print special symbols */
	printSymbols(&lcd);
	
	// Set time 
	uint8_t initialTime[7] = {00,22, 19, SAT, 2, JAN, 21};
	rtcSetTime(&rtc, initialTime);
	printTime();
	
	rtcSetMinutes(&rtc, 37);
	rtcSetHour(&rtc, 4);
	rtcSetDay(&rtc, SUN);
	rtcSetDate(&rtc, 3);
	rtcSetMonCen(&rtc, FEB, 0);
	uint8_t reg[19];
	// Set Alarm 2 to occur every minute
	uint8_t a2Time[4] = {00, 00, 00, 00};
	rtcSetAlarm2(&rtc, a2Time, A2_MATCH_ONCE_PER_MIN, printTime, NULL);
	
// 	/* Set Alarm 1 to occur every minute */
// 	uint8_t a1Time[4] = {00, 6, 00, 01};
// 	rtcSetAlarm1(&rtc, a1Time, A1_MATCH_SEC, printTime, NULL);	

	while(1)
	{
		rtcPoll(&rtc);
		rtcReadRegisters(&rtc, reg);
	}
	
}

/* Print time on LCD */
void printTime()
{
	/* uint8_t space; used for debugging. e.g space = snprintf(buff,20,"%x", time) */
	/* Set cursor to (0,1) to Set Hour:Min in LCD */
	lcdSetCursor(&lcd, 0, 1);
	char buff[20];
	for (int i=TIME_UNITS_HR; i>TIME_UNITS_SEC; i--)
	{
		snprintf(buff,20,"%x",rtc.time[i]);
		
		if(rtc.time[i] < 10)
			lcdPrint(&lcd, "0");
		lcdPrint(&lcd, buff);
		
		if (i!=TIME_UNITS_SEC+1) 
			lcdPrint(&lcd, ":");
	}
	/* Set cursor to (0,7) to Set Date/month/year in LCD */
	lcdSetCursor(&lcd, 0, 8);
	if ((rtc.time[TIME_UNITS_MO_CEN] & 0x1F) < 0x09) 
		lcdPrint(&lcd, "0");
	snprintf(buff,20,"%x/", rtc.time[TIME_UNITS_MO_CEN] & 0x1F);
	lcdPrint(&lcd, buff);
	
	if(rtc.time[TIME_UNITS_DT] < 10)
		lcdPrint(&lcd, "0");
		
	snprintf(buff,20,"%x/", rtc.time[TIME_UNITS_DT]);
	lcdPrint(&lcd, buff);
	
	if(rtc.time[TIME_UNITS_YR] < 10)
		lcdPrint(&lcd, "0");
	snprintf(buff,20,"%x", rtc.time[TIME_UNITS_YR]);
	lcdPrint(&lcd, buff);
	
}

void printSymbols(lcd_t *lcd)
{
	/* Build and print themometer and degree symbol */
	lcdBuildSym(lcd, thermSymLoc,thermoSym);
	lcdBuildSym(lcd, degreeSymLoc,degreeSym);
	lcdSetCursor(lcd, 1,0);
	lcdPrintSymbol(lcd, thermSymLoc);
	lcdSetCursor(lcd, 1,3);
	lcdPrintSymbol(lcd, degreeSymLoc);
	lcdPrint(lcd, "F");
	
	/* Build and print humidity symbol*/
	lcdBuildSym(lcd, humiditySymLoc,humiditySym);
	lcdSetCursor(lcd, 1,6);
	lcdPrintSymbol(lcd, humiditySymLoc);
	lcdSetCursor(lcd, 1,9);
	lcdPrint(lcd, "%");
	
	/* Build and Print clock symbol */
	lcdBuildSym(lcd, clkSymLoc, clockSymbol); // Build Clock Symbol
	lcdSetCursor(lcd, 0,0);
	lcdPrintSymbol(lcd, clkSymLoc);
	
	/* Build and print calendar symbol */
	lcdBuildSym(lcd, calSymLoc,calendarSymbol);
	lcdSetCursor(lcd, 0,7);
	lcdPrintSymbol(lcd, calSymLoc);
	
	/* Print moisture Symbol */	
}
