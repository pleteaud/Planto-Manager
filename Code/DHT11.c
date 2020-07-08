/*
 * DHT11.c
 *
 * Created: 3/26/2020 6:38:38 PM
 *  Author: Davo Pleteau
 */ 

/************************************************************************/
/*                     Includes/Constants                               */
/************************************************************************/
#include "DHT11.h"
#include "avr/io.h"

//setup port
#define DHT_DDR	DDRD
#define DHT_PORT PORTD
#define DHT_PIN	PIND
#define DHT_INPUTPIN PD6

#define NUM_OF_BYTES			0x05
#define NUM_BITS_IN_BYTE		0x08

#define TIMEOUT_VALUE			0x190 /* At 8MHz, that's about 50 microseconds which is the longest time signal is 0 when sending data*/
#define START_RESPONSE_TIMEOUT	0x320 /* At 8MHz, that's about 100 microseconds which is longer than the DHT start response time of 40us/80us/80us*/

/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/
static comm_status_t getdata(uint8_t *temperature, uint8_t *humidity, uint8_t pinNum);
static bool dht11Poll(dht11_sensor_t *sensorP);

/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/

/* Initializes the data members of a dht11 sensor object */
void dht11Init(dht11_sensor_t *sensorP, uint8_t pinNum)
{
	sensorP->temperature = sensorP->humidity=0;
	sensorP->pin = pinNum;
	sensorP->errorCounter = 0;
	sensorP->state = DHT11_IDLE;
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

/* Initiate a temperature and relative humidity read */
/* If return is false, reading procedure isn't complete or an error has occurred */
/* If true, sensor's temperature and moisture values are updated */
bool dht11ReadTempRH(dht11_sensor_t *sensorP)
{
	return dht11Poll(sensorP);
}
/************************************************************************/
/*                     Private Functions Implementation                 */
/************************************************************************/

/* Control and monitor communication procedure between chip and DHT11 */
/* If return is false, reading procedure isn't complete or an error has occurred */
/* If true, sensor's temperature and moisture values are updated */
static bool dht11Poll(dht11_sensor_t *sensorP)
{
	bool status = false;
	switch(sensorP->state)
	{
		/* Initiate poll */
		/* Idle: DHT11 can initiate a sensor read */
		case DHT11_IDLE:
		{
			/* Debugging purposes */
			DDRB |= 0x00;
			PORTB = 0x00;
			/* Change state to read since, we are not cooling down nor are we idle anymore */
			sensorP->state = DHT11_READ;
			/* Reset pin to output high */
			DHT_DDR |= (1<<sensorP->pin);
			DHT_PORT |= (1<<sensorP->pin);
			
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
				enum comm_status_e rxStatus = getdata(&sensorP->temperature,&sensorP->humidity,sensorP->pin);
				/* If read wasn't successful, log error */
				if (rxStatus != READ_SUCCESS)
				{
					sensorP->errorList[sensorP->errorCounter++]= rxStatus;
					status = false;
				}
				
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

/* Blocking procedure that extracts the temperature and humidity from DHT11 */
/* Return status of communication:
		READ_SUCCESS == Temp and Humidity was succesfully extracted from DHT11 sensor,
		NO_RESPONSE_ERROR == unable to establish an initial response from DHT11 sensor (before transmitting temp/rh data),
		TIME_OUT_ERROR == no response was detected during temp/rh bit transmission,
		BAD_CHECKSUM_ERROR == There is an error in the temp and rh data
*/
static comm_status_t getdata(uint8_t *temperature, uint8_t *humidity, uint8_t pinNum)
{
	/* Check if DHT is still cooling down from previous sample */
		
	uint8_t bits[NUM_OF_BYTES];
	uint8_t i,j = 0;

	//send request
	DHT_PORT &= ~(1<<pinNum); //low
	milli_delay(20);
	
	DHT_PORT |= (1<<pinNum); //high
	DHT_DDR &= ~(1<<pinNum); //input
	
	//wait for dht11 to pull signal low. Should take (~20-40uS)
	uint16_t noResponseCounter = 0;
	while(PIND & (1<<pinNum))
	{
		if (noResponseCounter++ >= START_RESPONSE_TIMEOUT)
		{
			return NO_RESPONSE_ERROR;
		}	
	}
	
	/* Wait for 80us low response */
	noResponseCounter = 0;
	while(!(PIND & (1<<pinNum)))
	{
		if (noResponseCounter++ >= START_RESPONSE_TIMEOUT)
		{
			return NO_RESPONSE_ERROR;
		}
	}
	
	/* Wait for 80us high response */
	noResponseCounter = 0;
	while(PIND & (1<<pinNum))
	{
		if (noResponseCounter++ >= START_RESPONSE_TIMEOUT)
		{
			return NO_RESPONSE_ERROR;
		}
	}
	
	/* Read and parse the data */
	uint16_t timeoutcounter = 0;
	for (j=0; j<NUM_OF_BYTES; j++)
	{
		uint8_t result=0;
		/*Read every bit */
		for(i=0; i<NUM_BITS_IN_BYTE; i++) 
		{
			timeoutcounter = 0;
			/* Wait for an high input */
			while(!(DHT_PIN & (1<<pinNum))) 
			{ 
				timeoutcounter++;
				/* timeout error */
				if(timeoutcounter > TIMEOUT_VALUE) 
				{
					return TIME_OUT_ERROR; //timeout
				}
			}
			/* Differentiating between a 1 and a 0 from the dht11 depends on how long the high pulse is on.*/
			/* ~28us is a 0, and `70us is a 1. */
			/* If input is still high after 30 us delay, then it must be a 1 */
			micro_delay(30);
			if(DHT_PIN & (1<<pinNum)) 
			result |= (1<<((NUM_BITS_IN_BYTE-1)-i));
			timeoutcounter = 0;
			while(DHT_PIN & (1<<pinNum)) { //wait until input get low (non blocking)
				timeoutcounter++;
				if(timeoutcounter > TIMEOUT_VALUE) {
					return TIME_OUT_ERROR; //timeout
				}
			}
		}
		bits[j] = result;
	}

	/* reset pin to output high */ 
	DHT_DDR |= (1<<pinNum); 
	DHT_PORT |= (1<<pinNum); 

	/*check checksum*/
	if ((uint8_t)(bits[0] + bits[1] + bits[2] + bits[3]) == bits[4]) 
	{
		*temperature = bits[2];
		*humidity = bits[0];
		return READ_SUCCESS;
	}
	else
	{
		return BAD_CHECKSUM_ERROR;
	}
}

