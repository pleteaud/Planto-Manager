#include <atmel_start.h>
#include "alarm.h"
#include "buffer.h"
#include "rtc.h"
#include "ds3231_regs_and_utils.h"
uint8_t numAlarms = 1;
alarm_t list[1];
int main(void)
{
	/* Initializes MCU, drivers and middleware */
	atmel_start_init();
	bufferInit(retrieveActiveBuffer());
	alarmsInit(list,numAlarms);
	alarmSet(&list[0], 3, 19, 25,35);
	bufferClear(retrieveActiveBuffer());
	alarmSet(&list[0], 5, 9, 36,25);
	bufferClear(retrieveActiveBuffer());
	
	uint8_t controlBit = 0;
	controlBit |= INTCN_FLAG | AI2E_FLAG | AI1E_FLAG;
	uint8_t statusBit = 0;
	rtcInit(retrieveActiveRTC(), controlBit, statusBit);
	uint8_t t[7] = {37,30, 23, 5,31, 6, 89};
	rtcSetTime(retrieveActiveRTC(), t);
	/* Replace with your application code */
	while (1) {
	}
}
