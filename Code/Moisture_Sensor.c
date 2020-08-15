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
#include "stdbool.h"
#include "stdio.h"


/* Setup relay port */
#define RELAY_DDR					DDRB
#define RELAY_PORT					PORTB
#define RELAY_OUTOUT_PIN			PB4

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
	sensorP->state = MS_IDLE;
	sensorP->sampleNum = sensorP->timeStamp = 0;
	sensorP->calibrateFlag = false;
	/* Configure pin DDRx as output low for relay control */
	RELAY_DDR |= (1 << RELAY_OUTOUT_PIN);
	soilSenPwrRelay(false);
	/* Register callback for ADC sensor reading completion */
	ADC_0_register_callback(adcReadCompCB);
	
	/* Build and print water content symbol*/
	lcdBuildSym(soilSenSymLoc,soilSenSymb);
	lcdSetDDRAMAdrr(1,11);
	lcdWriteSymbol(soilSenSymLoc);
	lcdWriteString("00%");
}

/* Retrieve sensor's soil moisure reading */
double soilSenGetMoisture(soil_moisture_sensor_t *sensorP)
{
	return sensorP->moisture;
}

/* Calibrate the upper and lower bound for soil moisture */
void soilSenCalibrate(soil_moisture_sensor_t *sensorP)
{
	sensorP->calibrateFlag = true;
	soilSenPwrRelay(true);
	/* Determine the value of the moisture, when soil is dry (upper bound) */
	alarmTrigFlag = true;
	while(!soilSenPoll(sensorP));
	/* Store ADC value when soil is dry */
	soilDryVal = soilSenGetMoisture(sensorP);
	/* Determine the value of the moisture. when soil is wet (lower bound) */
	soilSenPwrRelay(true);
	alarmTrigFlag = true;
	while(!soilSenPoll(sensorP));
	/* Store ADC value when soil is wet */
	soilWetVal = soilSenGetMoisture(sensorP);
	/* Update range for linear transformation */
	oldRange = soilDryVal-soilWetVal;
	sensorP->calibrateFlag = false;
	soilSenPwrRelay(false);
	return;
}

/* Control and monitor communication procedure between chip and DHT11 */
/* If return is false, reading procedure isn't complete*/
/* If true, the sensor's moisture value is updated */
bool soilSenPoll(soil_moisture_sensor_t *sensorP)
{
	bool status = false;
	switch (sensorP->state)
	{
		case MS_IDLE:
		{
			if (alarmTrigFlag)
			{
				soilSenPwrRelay(true);
				/* Enable ADC */
				ADC_0_enable();
				/* Wait 5 ms to allow sensor to stabilize */
				sensorP->timeStamp = 0;
				startMillisTimer();
				sensorP->state = MS_STABLILIZING;
				alarmTrigFlag = false;
			}
			break;
		}
		case MS_STABLILIZING:
		{
			/* Start to read sensor if we're done stabilizing */
			if (getMillis()-sensorP->timeStamp>=5)
			{
				sensorP->timeStamp=getMillis();
				/* Initiate an ADC Conversion */
				ADC_0_start_conversion(SENS_ADC_CHAN_NUM);
				/* Begin reading */
				sensorP->moisture = 0;
				sensorP->state = MS_READ;
			}
			break;
		}
		case MS_READ:
		{
			/* Sample for every 3ms, for 15ms */
			if (getMillis()-sensorP->timeStamp>=3 && sampleFlag)
			{
				sensorP->timeStamp = getMillis();
				
				/* Discard 0th sample since first read after enabling ADC may be inaccurate due to initializing analog circuitry */
				if(sensorP->sampleNum > 0)
				{
					sensorP->moisture += ADC_0_get_conversion_result();
				}
				
				/* If last sample, stop reading */
				if (sensorP->sampleNum == 5)
				{
					/* Disable ADC*/
					ADC_0_disable();
					/* Restart ADC value */
					sensorP->sampleNum = 0;
					sensorP->state = MS_READ_COMPLETE;
					break;
				}
				
				sensorP->sampleNum++;
				sampleFlag = false;
				ADC_0_start_conversion(SENS_ADC_CHAN_NUM);
			}
			
			break;
		}
		case MS_READ_COMPLETE:
		{
			/* Stop and reset timer */
			stopMillisTimer();
			sensorP->timeStamp=0;
			/* Turn off Sensor */
			soilSenPwrRelay(false);
			status = true;
			
			/* Take the average of the readings */
			sensorP->moisture /= 5;
			
			/* Set state to IDLE */
			sensorP->state = MS_IDLE;
				
			/* If we are calibrating, don't print and leave moisture value as an average */
			if(sensorP->calibrateFlag){break;}
			
			/* Convert average moisture calculated to a value between 0-100 if not calibrating */
			/* If value is closer to 0, soil is moist. If value is closer to 100, soil is dry */
			double newMin = 0, newMax = 100;
			double newRange = newMax - newMin; /* 0-100 */
			/* Translate old moisture reading to a number between 0-100 */
			sensorP->moisture = ((sensorP->moisture-soilWetVal)/oldRange)*(newRange) + newMin;
			
			/* Change state back to IDLE and power off sensor */
			sensorP->state = MS_IDLE;
			/* Print Moisture on LCD */
			printSoilMoist(sensorP->moisture);
			break;
		}
		default:
		break;
	}
	return status;
}

/************************************************************************/
/*                     Private Functions Implementation                 */
/************************************************************************/


/* Callback function to indicate a sampling of the ADC has been complete */
static adc_irq_cb_t adcReadCompCB()
{
	sampleFlag = true;
}
void alarmTiggerCB(void *p)
{
	alarmTrigFlag = true;
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