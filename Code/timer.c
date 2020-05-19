/*
 * timer.c
 *
 * Created: 4/11/2020 12:44:38 PM
 *  Author: plete
 */ 

#include "timer.h"
static uint16_t milliSecond;
/* Add a timer init function */
void startMillisTimer()
{
	/* Reset millisecond */
	milliSecond = 0;
	/*prescale by 1024 */
	TCCR1B = 0x05;
	/* Output compare A: 0x12 == approx 1.5sec*/
	OCR1A = 0x12;
	/* Enable Output compare match interrupt */
	TIMSK1 |= (1 << OCIE1A);
	/* Restart tick count to 0 */
	TCNT1 = 0;
}
void stopMillisTimer()
{
	/* Disable Output compare match interrupt */
	TIMSK1 &= ~(1 << OCIE1A);
	/* Restart tick count to 0 */
	TCNT1 = 0;
}


void updateMillis()
{
	milliSecond++;
	/*reset tick count*/
	TCNT1 = 0;
}
uint16_t getMillis()
{
	return milliSecond;
}

void milli_delay(int milliseconds)
{
	
	/* Prescale by 1024 */
	TCCR1B = 0x05;
	/* Reset tick */
	TCNT1 = 0;
	
	short currTime = 0;
	while(currTime < milliseconds)
	{
		while(TCNT1 < 12);
		TCNT1 = 0;
		currTime++;
	}
}

void micro_delay(int micro)
{
	/* No prescaler */
	TCCR1B = 0x01;
	/* Reset tick */
	TCNT1 = 0;

	short currTime = 0;
	while(currTime < micro)
	{
		while(TCNT1 < 12);
		TCNT1 = 0;
		currTime++;
	}
}