/*
 * timer.c
 *
 * Created: 4/11/2020 12:44:38 PM
 *  Author: Davo Pleteau
 */ 

/************************************************************************/
/*                     Includes/Constants                               */
/************************************************************************/
#include "timer.h"

#define MAX_INT32_VAL	0xFFFFFFFF
static uint32_t milliSecond;

/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/

/* Add a timer init function */

/* Start timer */
void startMillisTimer()
{
	/* Reset millisecond */
	milliSecond = 0;
	/*prescale by 1024 */
	TCCR1B = 0x05;
	/* Output compare A: 0x12 == approx 1.5sec*/
	OCR1A = 15;
	/* Enable Output compare match interrupt */
	TIMSK1 |= (1 << OCIE1A);
	/* Restart tick count to 0 */
	TCNT1 = 0;
}

/* Stop timer */
void stopMillisTimer()
{
	/* Disable Output compare match interrupt */
	TIMSK1 &= ~(1 << OCIE1A);
	/* Restart tick count to 0 */
	TCNT1 = 0;
}

/* Interrupt Callback to update milliseconds count */
void updateMillis()
{
	milliSecond = milliSecond < MAX_INT32_VAL ? milliSecond + 1 : 0;

	/*reset tick count*/
	TCNT1 = 0;
}

/* Retrieve milliseconds count */
uint32_t getMillis()
{
	return milliSecond;
}

/* Execute a delay in milliseconds */
void milli_delay(uint32_t milliseconds)
{
	
	/* Prescale by 1024 */
	TCCR1B = 0x05;
	/* Reset tick */
	TCNT1 = 0;
	
	short currTime = 0;
	while(currTime < milliseconds)
	{
		while(TCNT1 < 15);
		TCNT1 = 0;
		currTime++;
	}
}

/* Execute a delay in microseconds */
void micro_delay(uint32_t micro)
{
	/* No prescaler */
	TCCR1B = 0x02;
	/* Reset tick */
	TCNT1 = 0;

	short currTime = 0;
	while(currTime < micro)
	{
		while(TCNT1 < 2);
		TCNT1 = 0;
		currTime++;
	}
}
///*
 //* timer.c
 //*
 //* Created: 4/11/2020 12:44:38 PM
 //*  Author: Davo Pleteau
 //*/ 
//
///************************************************************************/
///*                     Includes/Constants                               */
///************************************************************************/
//#include "timer.h"
//static uint16_t milliSecond;
//
///************************************************************************/
///*                      Public Functions Implementations                */
///************************************************************************/
//
///* Add a timer init function */
//
///* Start timer */
//void startMillisTimer()
//{
	///* Reset millisecond */
	//milliSecond = 0;
	///*prescale by 1024 */
	//TCCR1B = 0x05;
	///* Output compare A: 0x12 == approx 1.5sec*/
	//OCR1A = 0x15;
	///* Enable Output compare match interrupt */
	//TIMSK1 |= (1 << OCIE1A);
	///* Restart tick count to 0 */
	//TCNT1 = 0;
//}
//
///* Stop timer */
//void stopMillisTimer()
//{
	///* Disable Output compare match interrupt */
	//TIMSK1 &= ~(1 << OCIE1A);
	///* Restart tick count to 0 */
	//TCNT1 = 0;
//}
//
///* Interrupt Callback to update milliseconds count */
//void updateMillis()
//{
	//milliSecond++;
	///*reset tick count*/
	//TCNT1 = 0;
//}
//
///* Retrieve milliseconds count */
//uint16_t getMillis()
//{
	//return milliSecond;
//}
//
///* Execute a delay in milliseconds */
//void milli_delay(int milliseconds)
//{
	//
	///* Prescale by 1024 */
	//TCCR1B = 0x05;
	///* Reset tick */
	//TCNT1 = 0;
	//
	//short currTime = 0;
	//while(currTime < milliseconds)
	//{
		//while(TCNT1 < 15);
		//TCNT1 = 0;
		//currTime++;
	//}
//}
//
///* Execute a delay in microseconds */
//void micro_delay(int micro)
//{
	///* No prescaler */
	//TCCR1B = 0x02;
	///* Reset tick */
	//TCNT1 = 0;
//
	//short currTime = 0;
	//while(currTime < micro)
	//{
		//while(TCNT1 < 2);
		//TCNT1 = 0;
		//currTime++;
	//}
//}