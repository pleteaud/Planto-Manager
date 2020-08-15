/*
 * DHT11.c
 *
 * Created: 3/26/2020 6:38:38 PM
 *  Author: Davo Pleteau
 */ 

/* The process for the timing critical portion of the DHT11 was influenced by ADAFRUIT's DHT11/22 code: */
/*	https://github.com/adafruit/DHT-sensor-library */ 

/************************************************************************/
/*                     Includes/Constants                               */
/************************************************************************/
#include "DHT11.h"
#include "avr/io.h"
#include "stdbool.h"
#include "LCD.h"
#include "stdio.h"

/* setup port */
#define DHT_DDR							DDRB
#define DHT_PORT						PORTB
#define DHT_PIN							PINB
#define DHT_PIN_NUM						PB3

/* Bits/Bytes info */
#define NUM_BITS_TX						(0x28) /* 40 bits */
#define BYTE_BIT_COUNT					(0x08)
#define NUM_OF_BYTES					(NUM_BITS_TX / BYTE_BIT_COUNT)

/* Timeout and cycle info */ 
#define START_RESPONSE_TIMEOUT			0x320 
#define MAXCYLCES						microsecondsToClockCycles(1000) /* At 16MHz (FCPU), that's about 16 cycles (16Mhz/1us) which is longer than the DHT start response time of 40us/80us/80us */
#define CYCLE_ARR_SIZE					(NUM_BITS_TX * 2) /* 80 */

unsigned char thermoSym[] = {0x04,0x0A,0x0A,0x0E,0x0E,0x1F,0x1F,0x0E};
uint8_t thermSymLoc = 2;
unsigned char degreeSym[] = {0x0C,0x12,0x12,0x0C,0x00,0x00,0x00,0x00};
uint8_t degreeSymLoc = 3;
unsigned char humiditySym[] = {0x04,0x04,0x0A,0x0A,0x11,0x11,0x11,0x0E};
uint8_t humiditySymLoc = 4;

/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/
static comm_status_t getdata(uint8_t *temperature, uint8_t *humidity);
static uint32_t expectPulse(bool level);
static void printTempRH(uint8_t temp, uint8_t humidity);


/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/

/* Initializes the data members of a dht11 sensor object */
void dht11Init(dht11_sensor_t *sensorP)
{
	sensorP->temperature = sensorP->humidity=0;
	sensorP->errorCounter = 0;
	sensorP->state = DHT11_IDLE;
	/* Delay a few seconds to allow dht11 to stabilize */
	milli_delay(4000);
	
	/* Build and print themometer and degree symbol*/
	lcdBuildSym(thermSymLoc,thermoSym);
	lcdBuildSym(degreeSymLoc,degreeSym);
	lcdSetDDRAMAdrr(1,0);
	lcdWriteSymbol(thermSymLoc);
	lcdSetDDRAMAdrr(1,3);
	lcdWriteSymbol(degreeSymLoc);
	lcdWriteString("F");
	
	/* Build and print humidity symbol*/
	lcdBuildSym(humiditySymLoc,humiditySym);
	lcdSetDDRAMAdrr(1,6);
	lcdWriteSymbol(humiditySymLoc);
	lcdSetDDRAMAdrr(1,9);
	lcdWriteString("%");
	
	/* Print initial values of Temp and Humidity. Will both be 0 */
	printTempRH(sensorP->temperature,sensorP->humidity);
}

/* Retrieve sensor's temperature reading */
uint8_t dht11GetTemp(dht11_sensor_t *sensorP)
{
	return sensorP->temperature;
}

/* Retrieve sensor's relative humidity reading */
uint8_t dht11GetRH(dht11_sensor_t *sensorP)
{
	return sensorP->humidity;
}

/* Control and monitor communication procedure between chip and DHT11 */
/* If return is false, reading procedure isn't complete or an error has occurred */
/* If true, sensor's temperature and moisture values are updated */
bool dht11Poll(dht11_sensor_t *sensorP)
{
	bool status = false;
	switch(sensorP->state)
	{
		/* Initiate poll */
		/* Idle: DHT11 can initiate a sensor read */
		case DHT11_IDLE:
		{
			/* Change state to read since, we are not cooling down nor are we idle anymore */
			sensorP->state = DHT11_READ;
			/* Reset pin to output high */
			DHT_DDR |= (1<<DHT_PIN_NUM);
			DHT_PORT |= (1<<DHT_PIN_NUM);
			
			/* Reset timer, and wait 100ms before sending start signal to allow dht11 to sync to pin*/
			/* May not need to be done */
			startMillisTimer();
			break;
		}
		/* DHT11_READ: Extract temperature from DHT11 */
		case DHT11_READ:
		{
			/* Check that 100ms has passed to initiate read*/
			if (getMillis()-0 >= 100)
			{
				stopMillisTimer();
				status = true;
				enum comm_status_e rxStatus = getdata(&sensorP->temperature,&sensorP->humidity);
				/* If read wasn't successful, log error */
				if (rxStatus != READ_SUCCESS)
				{
					sensorP->errorList[sensorP->errorCounter++]= rxStatus;
					status = false;
				}
				/* Print Time and Humidity on LCD Screen */
				printTempRH(sensorP->temperature,sensorP->humidity);
				/* Start timer for 2 second cool down */
				sensorP->state = DHT11_COOLDOWN;
				startMillisTimer();
			}
			break;
		}
		/* DHT11_COOLDOWN: Let dht11 cool down for 2 seconds before sensor available to read */
		case DHT11_COOLDOWN:
		{
			if(getMillis()-0 > 2000)
			{
				/* Stop timer */
				stopMillisTimer();
				sensorP->state = DHT11_IDLE;
			}
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

/* Blocking procedure that extracts the temperature and humidity from DHT11 */
/* Return status of communication: */
/*	READ_SUCCESS == Temp and Humidity was succesfully extracted from DHT11 sensor, */
/*	NO_RESPONSE_ERROR == unable to establish an initial response from DHT11 sensor (before transmitting temp/rh data), */
/*	TIME_OUT_ERROR == no response was detected during temp/rh bit transmission, */
/*	BAD_CHECKSUM_ERROR == There is an error in the temp and rh data  */
static comm_status_t getdata(uint8_t *temperature, uint8_t *humidity)
{
	uint8_t data[NUM_OF_BYTES];
	uint32_t cycles[CYCLE_ARR_SIZE];

	/* Turn off interrupt to prevent interrpution from timing critical code */
	
	/* Send Read Request to DHT11: low for at least 20ms */
	DHT_PORT &= ~(1 << DHT_PIN_NUM);
	milli_delay(20);
	
	/* Pull pin up high to transfer control of data pin to dht11 */
	DHT_PORT |= (1 << DHT_PIN_NUM); 
	DHT_DDR &= ~(1 << DHT_PIN_NUM); 
	
	/* Wait for dht11 to pull signal low. Should take (~20-40uS) */
	uint16_t noResponseCounter = 0;
	while(DHT_PIN & (1 << DHT_PIN_NUM))
	{
		if (noResponseCounter++ >= START_RESPONSE_TIMEOUT)
			return NO_RESPONSE_ERROR;
	}
	/* Wait for 80us low response */
	noResponseCounter = 0;
	while(!(DHT_PIN & (1 << DHT_PIN_NUM)))
	{
		if (noResponseCounter++ >= START_RESPONSE_TIMEOUT)
			return NO_RESPONSE_ERROR;
	}
	/* Wait for 80us high response */
	noResponseCounter = 0;
	while(DHT_PIN & (1 << DHT_PIN_NUM))
	{
		if (noResponseCounter++ >= START_RESPONSE_TIMEOUT)
		{
			return NO_RESPONSE_ERROR;
		}
	}
	
	 /* Read 40 bits of data sent by dht11 */
	 /* We differentiate between 1's and 0's by measuring the pulse cycle length */
	 /* Each bit sent starts with a ~50uS low signal followed by a ~28uS (0) or ~70uS (1) */
	 /* Even elements stores time pin was low (~50uS) and odd elements holds time pin was high (~28uS/~70uS) */
	 for (int i = 0; i < CYCLE_ARR_SIZE; i += 2) { 
		 cycles[i] = expectPulse(false);
		 cycles[i + 1] = expectPulse(true);
	 }
	
	/* Timing critical code is now complete */

	/* Inspect pulses and determine which ones are 0 or 1 */
	for (int i = 0; i < NUM_BITS_TX; ++i) {
		uint32_t lowCycles = cycles[2 * i];
		uint32_t highCycles = cycles[2 * i + 1];
		if ((lowCycles == TIME_OUT_ERROR) || (highCycles == TIME_OUT_ERROR)) {
			return TIME_OUT_ERROR;
		}
		data[i / BYTE_BIT_COUNT] <<= 1;
		// Now compare the low and high cycle times to see if the bit is a 0 or 1.
		if (highCycles > lowCycles) {
			// High cycles are greater than 50us low cycle count, must be a 1.
			data[i / BYTE_BIT_COUNT] |= 1;
		}
		// Else high cycles are less than (or equal to, a weird case) the 50us low
		// cycle count so this must be a zero.  Nothing needs to be changed in the
		// stored data.
	}
	
	/* Check we read 40 bits and that the checksum matches */
	if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
		*temperature = data[2];
		*humidity = data[0];
		return READ_SUCCESS;
		} else {
		return BAD_CHECKSUM_ERROR;
	}
}
/* Expect the signal line to be at the specified level for a period of time and */
/* return a count of loop cycles spent at that level. */
/* If more than 1ms passes without change in level, them return timeout error */
static uint32_t expectPulse(bool level) {
	uint32_t count = 0;
	uint8_t portState = level ? (1 << DHT_PIN_NUM) : 0;
	while ((DHT_PIN & (1<<DHT_PIN_NUM)) == portState) 
	{
		if (count++ >= MAXCYLCES) { return TIME_OUT_ERROR; }
	}
	return count;
}

/* Function to print Temperature and Humidity */
static void printTempRH(uint8_t temp, uint8_t humidity)
{
	char buff[20];
	/* Print temperature */
	lcdSetDDRAMAdrr(1,1);
	snprintf(buff,20,"%d",(uint8_t)((temp * 1.8) + 32));
	lcdWriteString(buff);
	/* Print humidity */
	lcdSetDDRAMAdrr(1,7);
	snprintf(buff,20,"%d",humidity);
	lcdWriteString(buff);
}