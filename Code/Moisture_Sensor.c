/*
 * Moisture_Sensor.c
 *
 * Created: 5/17/2020 6:07:41 PM
 *  Author: plete
 */ 
#include "Moisture_Sensor.h"
#include "adc_basic.h"

void powerSensor();
static bool poll(moisture_sensor_t *sensorP);

static uint8_t sampleFlag = 0;



adc_irq_cb_t adcReadCompCB()
{
	sampleFlag = 1;
}
void msInit(moisture_sensor_t *sensorP,uint8_t chann)
{
	sensorP->adcChannel = chann;
	sensorP->state = MS_IDLE;
	sensorP->sampleNum = sensorP->timeStamp = 0;
	ADC_0_register_callback(adcReadCompCB);
}
bool readMoisture(moisture_sensor_t *sensorP)
{
	return poll(sensorP);
}
double msGetMoisture(moisture_sensor_t *sensorP)
{
	return sensorP->moisture;
}

static bool poll(moisture_sensor_t *sensorP)
{
	bool status = false;
	switch (sensorP->state)
	{
		case MS_IDLE:
		{
			/* Turn on Relay to power moisture sensor */
			//powerSensor(sensorP->pinNum,sensorP->ddrx, sensorP->portx);
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
			break;
		}
		default:
		break;
	}
	return status;
}
