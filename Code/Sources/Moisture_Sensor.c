/*
 * Moisture_Sensor.c
 *
 * Created: 5/17/2020 6:07:41 PM
 *  Author: Davo Pleteau
 */

/************************************************************************/
/*                     Includes/Constants/Globals                       */
/************************************************************************/ 
#include "Moisture_Sensor.h"
#include "adc_basic.h"
#include "LCD.h"
#include "stdio.h"
#include "timer.h"



/* Setup relay port */
#define RELAY_DDR					DDRC
#define RELAY_PORT					PORTC
#define RELAY_OUTOUT_PIN			PINC2

/* Setup Calibration Button */
#define CAL_BTN_DDR			DDRB
//#define CAL_BTN_PORT		PORTB
#define CAL_BTN_INPUT_PIN	PINB
#define CAL_BTN_PIN_NUM		PINB3


#define SENS_ADC_CHAN_NUM			(0x00)

static bool sampleFlag = false;
static bool alarmTrigFlag = false;

/* Upper and lower bound for soil moisture value */
static volatile double soilWetVal = 0; // lower bound
static volatile double soilDryVal = 0; // upper bound
static volatile double oldRange = 0; // Moisture Range

/* Water Content Symbol */
unsigned char soilSenSymb[] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x0E,0x00};
uint8_t soilSenSymLoc = 5;
/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/

static adc_irq_cb_t adcReadCompCB();
static void printSoilMoist(uint8_t moisture);
static void waitForBtnPress();


/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/

/* Turn on Relay to power moisture sensor */
void soilSenPwrRelay(bool relayState)
{
	if(relayState)
	{
		/* Turn On relay to power sensors */
		RELAY_PORT |= (1<< RELAY_OUTOUT_PIN);
	}
	else
	{
		/* Turn off sensor */
		RELAY_PORT &= ~(1<< RELAY_OUTOUT_PIN);
	}	
}

/* Initialize moisture sensor object's data members */
void soilSensInit(soil_moisture_sensor_t *sensorP)
{
	//sensorP->state = MS_UNCALIBRATED;
	//sensorP->sampleNum = sensorP->timeStamp = 0;
	//sensorP->calibrateFlag = false;
	///* Configure pin DDRx as output low for relay control */
	//RELAY_DDR |= (1 << RELAY_OUTOUT_PIN);
	//soilSenPwrRelay(false);
	///* Register callback for ADC sensor reading completion */
	//ADC_0_register_callback(adcReadCompCB);
	//
	///* Configure Calibration Button */
	//CAL_BTN_DDR &= ~(1 << CAL_BTN_PIN_NUM); // Intput pin 
	///* Build and print water content symbol*/
	//lcdBuildSym(soilSenSymLoc,soilSenSymb);
	//lcdSetDDRAMAdrr(1,11);
	//lcdWriteSymbol(soilSenSymLoc);
	//lcdWriteString("00%");
}

/* Retrieve sensor's soil moisure reading */
double soilSenGetMoisture(soil_moisture_sensor_t *sensorP)
{
	return sensorP->moisture;
}

/* Calibrate the upper and lower bound for soil moisture */
void soilSenCalibrate(soil_moisture_sensor_t *sensorP)
{
	///* Determine the value of the moisture, when soil is dry (upper bound) */ 
	//sensorP->calibrateFlag = true;
	//sensorP->state = MS_IDLE;
	//soilSenPwrRelay(true);
	//alarmTrigFlag = true;
	//
	///* Indicate to user now calibrating the value of the sensor when soil is dry */
	//lcdSetDDRAMAdrr(0,16);
	//lcdWriteString("Calibrating S-M:");
	//lcdSetDDRAMAdrr(1,16);
	//lcdWriteString("Maximum Dryness");
	//milli_delay(250);
	///* Wait for user to press the calibration button to begin measurement */
	//waitForBtnPress();
	//lcdSetDDRAMAdrr(0,16);
	//lcdWriteString("  Release Button  ");
	//lcdSetDDRAMAdrr(1,16);
	//lcdWriteString("To Begin Reading");
	///* Give user a second to release button */
	//milli_delay(1000);
	//
	///* Read sensor value */
	//readSens(sensorP);
	///* Store ADC value when soil is dry */
	//soilDryVal = soilSenGetMoisture(sensorP);
	//
	//
	///* Determine the value of the moisture. when soil is wet (lower bound) */
	//soilSenPwrRelay(true);
	//alarmTrigFlag = true;
	//
	///* Indicate to user now calibrating the value of the sensor when soil is wet */
	//lcdSetDDRAMAdrr(0,16);
	//lcdWriteString("Calibrating S-M:");
	//lcdSetDDRAMAdrr(1,16);
	//lcdWriteString("Maximum Wetness ");
	//milli_delay(250);
	///* Wait for user to press the calibration button to begin measurement */
	//waitForBtnPress();
	//lcdSetDDRAMAdrr(0,16);
	//lcdWriteString("  Release Button  ");
	//lcdSetDDRAMAdrr(1,16);
	//lcdWriteString("To Begin Reading");
	///* Give user a second to release button */
	//milli_delay(1000);
	//
	///* Read sensor value */
	//readSens(sensorP);
	///* Store ADC value when soil is wet */
	//soilWetVal = soilSenGetMoisture(sensorP);
	//
	///* Update range for linear transformation */
	//oldRange = soilDryVal-soilWetVal;
	//if (oldRange == 0)
	//{
		///* Indicate to user there was an error calibrating sensor and set state to "Uncalibrated" to prevent invalid math operations (e.g 0/0) */
		//lcdSetDDRAMAdrr(0,16);
		//lcdWriteString("Calibrating S-M:");
		//lcdSetDDRAMAdrr(1,16);
		//lcdWriteString("Error: Bad Range");
		//sensorP->state = MS_UNCALIBRATED;
	//}
	//else
	//{
		///* Indicate successful calibration*/
		//lcdSetDDRAMAdrr(0,16);
		//lcdWriteString("Calibrating S-M:");
		//lcdSetDDRAMAdrr(1,16);
		//lcdWriteString("    Complete    ");
	//}
	//sensorP->calibrateFlag = false;
	//soilSenPwrRelay(false);
	//return;
}

void readSens(soil_moisture_sensor_t *sensorP)
{
	soilSenPwrRelay(true);
	milli_delay(10);
	/* Start to read */
	for (int i = 0; i < 10; i++)
	{
		/* Initiate an ADC Conversion */
		ADC_0_start_conversion(SENS_ADC_CHAN_NUM);
		sensorP->moisture += ADC_0_get_conversion(SENS_ADC_CHAN_NUM);
	}
	/* Turn off Sensor */
	soilSenPwrRelay(false);
	/* Take the average of the readings */

	sensorP->moisture /= 5;
	
	/* If we are calibrating, don't print and leave moisture value as an average */
	if(sensorP->calibrateFlag){return;}
	
	/* Convert average moisture calculated to a value between 0-100 */
	/* If value is closer to 0, soil is moist. If value is closer to 100, soil is dry */
	double newMin = 0, newMax = 100;
	double newRange = newMax - newMin; /* 0-100 */
	sensorP->moisture = ((sensorP->moisture-soilWetVal)/oldRange)*(newRange) + newMin;
	
	/* Print Moisture on LCD */
	printSoilMoist((uint8_t)sensorP->moisture);

}

/************************************************************************/
/*                     Private Functions Implementation                 */
/************************************************************************/


/* Callback function to indicate a sampling of the ADC has been complete */
static adc_irq_cb_t adcReadCompCB()
{
	sampleFlag = true;
}

static void printSoilMoist(uint8_t moisture)
{
	char buff[20];
	/* Print moisture */
	lcdSetDDRAMAdrr(1,12);
	if (moisture < 10){lcdWriteString("0");}
	snprintf(buff,20,"%d", moisture);
	lcdWriteString(buff);
	lcdWriteString("%");	
}

/* Wait for button to have been pressed for at least 25ms */
static void waitForBtnPress()
{
	
	startMillisTimer();
	uint16_t timePressed = 0;
	uint8_t counter = 0;
	while(counter < 5)
	{
		uint16_t x = getMillis();
		if (x-timePressed >= 5)
		{
			if (!(CAL_BTN_INPUT_PIN & (1<<CAL_BTN_PIN_NUM)))
			{
				counter++;
				timePressed = x;
			}
		}
	}
	stopMillisTimer();
}