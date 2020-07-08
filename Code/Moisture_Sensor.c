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


/* Setup relay port */
#define RELAY_DDR	DDRB
#define RELAY_PORT PORTB
#define RELAY_OUTOUT_PIN PB1

static uint8_t sampleFlag = 0;
/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/

static bool poll(soil_moisture_sensor_t *sensorP);
static adc_irq_cb_t adcReadCompCB();

/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/

/* Turn on Relay to power moisture sensor */
void sensorPower(bool relayState)
{
	if(relayState)
	{
		/* Send turn on relay to power sensors */
		RELAY_PORT |= (1<< RELAY_OUTOUT_PIN);
	}
	else
	{
		/* Send turn on relay to power sensors */
		RELAY_PORT &= ~(1<< RELAY_OUTOUT_PIN);
	}	
}

/* Initialize moisture sensor object's data members */
void sensorInit(soil_moisture_sensor_t *sensorP,uint8_t chann)
{
	sensorP->adcChannel = chann;
	sensorP->state = MS_IDLE;
	sensorP->sampleNum = sensorP->timeStamp = 0;
	/* Configure pin DDRx as output low for relay control */
	RELAY_DDR |= (1 << RELAY_OUTOUT_PIN);
	sensorPower(false);
	/* Register callback for ADC sensor reading completion */
	ADC_0_register_callback(adcReadCompCB);
}

/* Initiate a moisture sensor read */
/* If return is false, reading procedure isn't complete*/
/* If true, the sensor's moisture value is updated */
bool sensorRead(soil_moisture_sensor_t *sensorP)
{
	return poll(sensorP);
}

/* Retrieve sensor's soil moisure reading */
double sensorGet(soil_moisture_sensor_t *sensorP)
{
	return sensorP->moisture;
}



/************************************************************************/
/*                     Private Functions Implementation                 */
/************************************************************************/

/* Control and monitor communication procedure between chip and DHT11 */
/* If return is false, reading procedure isn't complete*/
/* If true, the sensor's moisture value is updated */
static bool poll(soil_moisture_sensor_t *sensorP)
{
	bool status = false;
	switch (sensorP->state)
	{
		case MS_IDLE:
		{
			/* Enable ADC */
			ADC_0_enable();
			/* Wait 5 ms to allow sensor to stabilize */
			sensorP->timeStamp = 0;
			startMillisTimer();
			sensorP->state = MS_STABLILIZING;
			break;
		}
		case MS_STABLILIZING:
		{
			/* Start to read sensor if we're done stabilizing */
			if (getMillis()-sensorP->timeStamp>=5)
			{
				sensorP->timeStamp=getMillis();
				/* Initiate an ADC Conversion */
				ADC_0_start_conversion(sensorP->adcChannel);
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
				sampleFlag = 0;
				ADC_0_start_conversion(sensorP->adcChannel);
			}
				
			break;
		}
		case MS_READ_COMPLETE:
		{
			/* Take the average of the readings and convert to percentage */
			sensorP->moisture /= 5;
			/* Moisture of plant = moisture_read/moisture_fully_wet */
			status = true;
			/* Stop timer and reset timer */
			stopMillisTimer();
			sensorP->timeStamp=0;
			/* Change state back to IDLE. If a new reading needs to be done process will start again */
			sensorP->state = MS_IDLE;
			break;
		}
		default:
		break;
	}
	return status;
}
/* Callback function to indicate a sampling of the ADC has been complete */
static adc_irq_cb_t adcReadCompCB()
{
	sampleFlag = 1;
}