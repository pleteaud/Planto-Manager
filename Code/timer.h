/*
 * timer.h
 *
 * Created: 4/10/2020 12:09:28 PM
 *  Author: plete
 */ 


#ifndef TIMER_H_
#define TIMER_H_

#include "avr\io.h"


void startMillisTimer();
void stopMillisTimer();

void updateMillis();
uint16_t getMillis();

void milli_delay(int milliseconds);

void micro_delay(int micro);

#endif /* TIMER_H_ */