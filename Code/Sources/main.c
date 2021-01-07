#include "main.h"

/* Instantiate LCD, RTC, and BME sensor */
lcd_t lcd;
rtc_manager_t rtc;
struct bme280_dev dev;
mcp23017_t ioExpander;

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

static bool updateFlag = false;
int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();

	i2cMasterInit(0);
	/* Initialize mcp23017 */
	mcpInit(&ioExpander, 0, &DDRB, &PORTB, PINB3); // Pin B 3 is reset pin
	mcpSetPortDir(&ioExpander, MCP23017_PORTB, 0);
	
	/* Initialize LCD */
	//lcdInit(&lcd, &DDRB, &PORTB, PINB0, PINB1, PINB2, true, false);	
	lcdInit(&lcd, &ioExpander, &DDRB, &PORTB, PINB0, PINB1, PINB2, true, false);	
	/* Initialize bme sensor */
	uint8_t devAddr = BME280_I2C_ADDR_PRIM;
	initBME(&dev, userI2cRead, userI2cWrite, userDelayUs, &devAddr);
	
	/* Initialize and Configure RTC */
	rtcInit(&rtc);
	
	// Set time 
	uint8_t initialTime[7] = {00,7, 13, WED, 6, JAN, 21};
	rtcSetTime(&rtc, initialTime);
	
	// Set Alarm 2 to occur every minute
	uint8_t a2Time[4] = {00, 00, 00, 00};
	rtcSetAlarm2(&rtc, a2Time, A2_MATCH_ONCE_PER_MIN, setUpdateFlag, NULL);
	
	/* Build and print special symbols */
	printSymbols(&lcd);

	while(1)
	{
		rtcPoll(&rtc);
		if(updateFlag)
		{
			printTime(&lcd, &rtc);
			if(!getSensorDataForcedMode(&lcd, &dev))
			{
				//print error
			}
			updateFlag = false;
		}
	}
	
}

/* Alarm 2 callback function to indicate rtc has been updated */
void setUpdateFlag()
{
	updateFlag = true;
}

/* Print time on LCD */
void printTime(lcd_t *lcdP,rtc_manager_t *rtcP)
{
	// Set cursor to (0,1) to print Hour:Min in LCD
	lcdSetCursor(lcdP, 0, 1);
	char lcdBuff[20] = {0};
	
	// Print Hour and Minute
	snprintf(lcdBuff, 20, "%02x:%02x", rtc.time[TIME_UNITS_HR], rtc.time[TIME_UNITS_MIN]);
	lcdPrint(lcdP, lcdBuff);
	
	// Set cursor to (0,7) to print Date/month/year on LCD 
	lcdSetCursor(lcdP, 0, 8);
	
	// Print date
	snprintf(lcdBuff, 20,"%02x/%02x/%02x", rtc.time[TIME_UNITS_MO_CEN] & 0x1F, rtc.time[TIME_UNITS_DT],
										rtc.time[TIME_UNITS_YR]);
	lcdPrint(lcdP, lcdBuff);
	lcdHome(lcdP);
}

/* Print and Build symbols. Will only be called once */
void printSymbols(lcd_t *lcdP)
{
	/* Build and print themometer and degree symbol */
	lcdBuildSym(lcdP, thermSymLoc,thermoSym);
	lcdBuildSym(lcdP, degreeSymLoc,degreeSym);
	lcdSetCursor(lcdP, 1,0);
	lcdPrintSymbol(lcdP, thermSymLoc);
	lcdSetCursor(lcdP, 1,6);
	lcdPrintSymbol(lcdP, degreeSymLoc);
	lcdPrint(lcdP, "F");
	
	/* Build and print humidity symbol*/
	lcdBuildSym(lcdP, humiditySymLoc,humiditySym);
	lcdSetCursor(lcdP, 1,9);
	lcdPrintSymbol(lcdP, humiditySymLoc);
	lcdSetCursor(lcdP, 1,15);
	lcdPrint(lcdP, "%");
	
	/* Build and Print clock symbol */
	lcdBuildSym(lcdP, clkSymLoc, clockSymbol); 
	lcdSetCursor(lcdP, 0,0);
	lcdPrintSymbol(lcdP, clkSymLoc);
	
	/* Build and print calendar symbol */
	lcdBuildSym(lcdP, calSymLoc,calendarSymbol);
	lcdSetCursor(lcdP, 0,7);
	lcdPrintSymbol(lcdP, calSymLoc);
	
	/* Print moisture Symbol */	
	
	lcdHome(lcdP);
}

/* Print BME280 temperature, humidity, and pressure */
void printBMEdata(lcd_t *lcdP, struct bme280_data *comp_data)
{
	
	float temp, press, hum;
		temp = 0.01f * comp_data->temperature;
		press = 0.0001f * comp_data->pressure;
		hum = 1.0f / 1024.0f * comp_data->humidity;
	
	#ifdef BME280_FLOAT_ENABLE
		temp = comp_data->temperature;
		press = 0.01 * comp_data->pressure;
		hum= comp_data->humidity;
	#else
	#ifdef BME280_64BIT_ENABLE
		temp = 0.01f * comp_data->temperature;
		press = 0.0001f * comp_data->pressure;
		hum = 1.0f / 1024.0f * comp_data->humidity;
	#else
		temp = 0.01f * comp_data->temperature;
		press = 0.01f * comp_data->pressure;
		hum = 1.0f / 1024.0f * comp_data->humidity;
	#endif
	#endif
	
	/* Print temperature */
	char lcdBuff[20] = {0}; // lcd buffer
	
	temp = (temp * 1.8) + 32; //convert to F
	
	/* Get interger and fractional value to print on lcd (floats don't work)*/
	unsigned int intergralVal = temp; 
	unsigned int fractionalVal = (temp - (uint32_t)temp) * 100;
	
	snprintf(lcdBuff, 20, "%u.%02u", intergralVal, fractionalVal);
	lcdSetCursor(lcdP, 1, 1);
	lcdPrint(lcdP, lcdBuff); // print
	
	/* Print humidity */
	intergralVal = hum;
	fractionalVal = (hum - (uint32_t)hum) * 100;
	
	snprintf(lcdBuff, 20, "%u.%02u", intergralVal, 3);
	lcdSetCursor(lcdP, 1, 10);
	lcdPrint(lcdP, lcdBuff); // Print
}

/* Initialize a BME sensor */
uint8_t initBME(struct bme280_dev *sensor, int8_t (*userI2cRead)(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr),
				int8_t (*userI2cWrite)(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr),
				void (*userDelayUs)(uint32_t period, void *intf_ptr),
				uint8_t *i2cAddr)
			
{
	int8_t rslt = BME280_OK;

	dev.intf_ptr = i2cAddr;
	dev.intf = BME280_I2C_INTF;
	dev.read = userI2cRead;
	dev.write = userI2cWrite;
	dev.delay_us = userDelayUs;

	rslt = bme280_init(&dev);
	return rslt;
} 

/* BME function to execute an i2c read */
int8_t userI2cRead(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
	 int8_t rslt = 1; /* Return 0 for Success, non-zero for failure */
	 uint8_t *i2cDevAddr = (uint8_t *)intf_ptr;
	 
    /*
     * The parameter intf_ptr can be used as a variable to store the I2C address of the device
     */
	
	// Transmit address
	if (i2cMasterTransmit(*i2cDevAddr, &reg_addr, 1))
		// Read BME sensor
		if (i2cMasterRead(*i2cDevAddr, reg_data, len)) 
			rslt = 0;
	
    return rslt;
}

/* BME function to execute an i2c write */
int8_t userI2cWrite(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
	 int8_t rslt = 1; /* Return 0 for Success, non-zero for failure */
	 uint8_t *i2cDevAddr = (uint8_t *)intf_ptr;
	 
	/* Copy reg_data and store reg_addr in the front. this prevents doing 2 i2c transmissions */
	uint8_t newRegData[len + 1];
	newRegData[0] = reg_addr; 
	
	// Copy reg_data
	for (uint8_t i = 0; i < len; i ++)
		newRegData[i + 1] = *(reg_data + i);
	
	// Transmit data
	if (i2cMasterTransmit(*i2cDevAddr, newRegData, len + 1))
		rslt = 0;
    
	return rslt;
}


/* BME function to execute a microseconds delay */
void userDelayUs (uint32_t period, void *intf_ptr)
{
	micro_delay(period);
}

/* BME Function to execute a sensor read */
int8_t getSensorDataForcedMode(lcd_t *lcdP, struct bme280_dev *dev)
{
    int8_t rslt;
    uint8_t settings_sel;
	uint32_t req_delay;
    struct bme280_data comp_data;

    /* Recommended mode of operation: Indoor navigation */
    dev->settings.osr_h = BME280_OVERSAMPLING_1X;
    dev->settings.osr_p = BME280_OVERSAMPLING_16X;
    dev->settings.osr_t = BME280_OVERSAMPLING_2X;
    dev->settings.filter = BME280_FILTER_COEFF_16;

    settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

    rslt = bme280_set_sensor_settings(settings_sel, dev);
	
	if (rslt != BME280_OK)
		return rslt;
	
	/*Calculate the minimum delay required between consecutive measurement based upon the sensor enabled
     *  and the oversampling configuration. */
    req_delay = bme280_cal_meas_delay(&dev->settings) * 1000;
	

	/* Get sensor data */
	rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, dev);
		
	if (rslt != BME280_OK)
		return rslt;
	/* Wait for the measurement to complete and print data @25Hz */
	dev->delay_us(req_delay, dev->intf_ptr);
	rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, dev);
		
	if (rslt != BME280_OK)
		return rslt;
		
	dev->delay_us(1000000, dev->intf_ptr);

	
    printBMEdata(lcdP, &comp_data);
		
    return rslt;
}

