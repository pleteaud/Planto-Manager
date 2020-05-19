/*
 * ds3231_regs.h
 *
 * Created: 8/8/2019 10:40:01 AM
 *  Author: plete
 */ 

#ifndef DS3231_REGS_AND_UTILS_H_
#define DS3231_REGS_AND_UTILS_H_

/************************************************************************/
/*							Includes	                                */
/************************************************************************/
#include "stdint.h"
#include "stdbool.h"

/************************************************************************/
/*							Constants					                */
/************************************************************************/
/*** Address Map for DS3231 Registers ***/
#define RTC_SEC_ADDR			(0x00)
#define RTC_MIN_ADDR			(0x01)
#define RTC_HRS_ADDR			(0x02)
#define RTC_DY_ADDR				(0x03)
#define RTC_DT_ADDR				(0x04)
#define RTC_M_CEN_ADDR			(0x05)
#define RTC_YR_ADDR				(0x06)

#define A1_SEC_ADDR				(0x07)
#define A1_MIN_ADDR				(0x08)
#define A1_HR_ADDR				(0x09)
#define A1_DY_DT_ADDR			(0x0a)
#define A2_MIN_ADDR				(0x0b)
#define A2_HR_ADDR				(0x0c)
#define A2_DY_DT_ADDR			(0x0d)

#define RTC_CTRL_ADDR			(0x0e)
#define RTC_CTRL_STAT_ADDR		(0x0f)
#define RTC_AO_ADDR				(0x10)
#define RTC_MSB_TEMP_ADDR		(0x11)
#define RTC_LSB_TEMP_ADDR		(0x12)	

/* Alarm Matching Bits */
#define AxMx_FLAG_MASK			(0x01UL)
#define AxMx_FLAG_POS			(0x07)
#define AxM1_FLAG_SCALOR		(0x01)
#define AxM2_FLAG_SCALOR		(0x2)
#define AxM3_FLAG_SCALOR		(0x03)
#define AxM4_FLAG_SCALOR		(0x04)

#define DY_DT_FLAG_MASK			(0x01)
#define DY_DT_FLAG_POS			(0x06)
#define DY_DT_FLAG				(DY_DT_FLAG_MASK << DY_DT_FLAG_POS)



/*** DS3231 Control Register ***/

/* Enable Oscillator (EOSC) */
#define EOSC_FLAG_MASK			(0x01)
#define EOSC_FLAG_POS			(0x07)
#define EOSC_FLAG				(EOSC_FLAG_MASK << EOSC_FLAG_POS)

/* Battery-Backed Square-Wave Enable (BBSQW) */
#define BBSQW_FLAG_MASK			(0x01)
#define BBSQW_FLAG_POS			(0x06)
#define BBSQW_FLAG				(BBSQW_FLAG_MASK << BBSQW_FLAG_POS)

/* Convert Temperature (CONV) */
#define CONV_FLAG_MASK			(0x01)
#define CONV_FLAG_POS			(0x05)
#define CONV_FLAG				(CONV_FLAG_MASK << CONV_FLAG_POS)

/* Rate Select (RS2 and RS1) */
#define RS1_FLAG_MASK			(0x01)
#define RS1_FLAG_POS			(0x04)
#define RS1_FLAG				(RS1_FLAG_MASK << RS1_FLAG_POS)
#define RS2_FLAG_MASK			(0x01)
#define RS2_FLAG_POS			(0x03)
#define RS2_FLAG				(RS2_FLAG_MASK << RS2_FLAG_POS)

/* Interrupt Control (INTCN) */
#define INTCN_FLAG_MASK			(0x01)
#define INTCN_FLAG_POS			(0x02)
#define INTCN_FLAG				(INTCN_FLAG_MASK << INTCN_FLAG_POS)

/* Alarm 2 Interrupt Enable (A2IE) */
#define AI2E_FLAG_MASK			(0x01)
#define AI2E_FLAG_POS			(0x01)
#define AI2E_FLAG				(AI2E_FLAG_MASK << AI2E_FLAG_POS)

/* Alarm 1 Interrupt Enable (A1IE) */
#define AI1E_FLAG_MASK			(0x01)
#define AI1E_FLAG_POS			(0x00)
#define AI1E_FLAG				(AI1E_FLAG_MASK << AI1E_FLAG_POS)


/*** DS3231 Status Register ***/

/* Oscillator Stop Flag (OSF) */
#define OSC_FLAG_MASK			(0x01)
#define OSC_FLAG_POS			(0x07)
#define OSC_FLAG				(OSC_FLAG_MASK << OSC_FLAG_POS)

/* Enable 32kHz Output (EN32kHz) */
#define EN32KHZ_FLAG_MASK		(0x01)
#define EN32KHZ_FLAG_POS		(0x03)
#define EN32KHZ_FLAG			(EN32KHZ_FLAG_MASK << EN32KHZ_FLAG_POS)

/* Busy (BSY) */
#define BSY_FLAG_MASK			(0x01)
#define BSY_FLAG_POS			(0x02)
#define BSY_FLAG				(BSY_FLAG_MASK << BSY_FLAG_POS)

/* Alarm 2 Flag (A2F) */
#define A2I_FLAG_MASK			(0x01)
#define A2I_FLAG_POS			(0x01)
#define A2I_FLAG				(A2I_FLAG_MASK << A2I_FLAG_POS)

/* Alarm 1 Flag (A2F) */
#define A1I_FLAG_MASK			(0x01)
#define A1I_FLAG_POS			(0x00)
#define A1I_FLAG				(A1I_FLAG_MASK << A1I_FLAG_POS)

#define BYTE_SHIFT				(sizeof(uint64_t))
#define NIBBLE_SHIFT			(BYTE_SHIFT/2)
#define START_ADDRESS_OFFSET	(0x01)

/* Slave Address for I2C */
#define DS3231_SLAVE_ADDR       (0x68)
// Function to get an alarm match flag 
#define getAxMxFlag(a)  (AxMx_FLAG_MASK << (AxMx_FLAG_POS * a))

#endif /* DS3231_REGS_H_ */