/*
 * timer.h
 *
 * Created: 4/10/2020 12:09:28 PM
 *  Author: Davo Pleteau
 */ 

#define clockCyclesPerMicrosecond() (16000000 / 1000000L)
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )
#define microsecondsToClockCycles(a) ( (a) * clockCyclesPerMicrosecond() )


#ifndef TIMER_H_
#define TIMER_H_

/************************************************************************/
/*							Includes/Constants	 	                    */
/************************************************************************/
#include "avr\io.h"

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
void startMillisTimer();
void stopMillisTimer();
void updateMillis();
uint16_t getMillis();
void milli_delay(int milliseconds);
void micro_delay(int micro);

#endif /* TIMER_H_ */