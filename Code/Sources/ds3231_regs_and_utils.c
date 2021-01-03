/*
 * ds3231_regs_and_utils.c
 *
 * Created: 8/9/2019 10:54:29 AM
 *  Author: plete
 */ 

/************************************************************************/
/*						Includes/Constants					            */
/************************************************************************/
#include "ds3231_regs_and_utils.h"
#include "stdlib.h"
#include "stddef.h"

/************************************************************************/
/*					Public Functions Implementation			            */
/************************************************************************/

/* Extract the digits values from a given number */
//bool getDigits(uint8_t timeUnit, uint8_t *digitP, uint8_t maxNumDigits)
//{
	//bool status = false;
	//
	///* Allocate the specified number of digits place value */	
	//uint8_t *tempDigArr = (uint8_t*) calloc(maxNumDigits, sizeof(uint8_t)); 
	//if (tempDigArr == NULL) {
		//free(tempDigArr);
		//return status;
	//}
	//
	///* Successfully allocated memory for array */
	//status = true;
	//
	///* Start extracting digits */
	//int arrIndex = 0;
	//while(timeUnit > 0)
	//{
		//tempDigArr[arrIndex]= timeUnit % 10;
		//timeUnit /= 10;
		//arrIndex++;
	//}
	///* The digits in tempDigArr are in reverse. Must store the reverse of tempDigArr for correct digit order */
	//arrIndex = maxNumDigits - 1; //Reinitialize array index to avoid going out of range of tempDigArr.
	//for (int j = 0; j < maxNumDigits; j++)
	//{
		//digitP[j] = tempDigArr[arrIndex--];
	//}
	//
	///* De-allocate memory */
	//free(tempDigArr);
	//tempDigArr = NULL;
	//return status;
//}
//
///* Configures the time bits in a way that satisfies the DS3231 registers */
//uint64_t configTime(uint8_t *time, uint8_t totTimeUnits, uint8_t maxNumDigits)
//{
	//uint8_t digits[TIME_UNITS_TOTAL*DIGITS_PER_TIME_UNIT];	
	//int n = totTimeUnits * maxNumDigits;
	//uint8_t *digits = NULL;
	//digits = calloc(n, sizeof(uint8_t)); // seconds/ minutes/ hours/ day/ date/ month(century)/ year
	//if(digits==NULL)
	//{
		//int x = 5;
	//}
	//int digitsIndex = 0;
	//for (int i = 0; i < totTimeUnits; i++)
	//{
		//if(!getDigits(*(time+i), (digits + digitsIndex), maxNumDigits))
		//{
			//free(digits);
			//return 0; // Returning zero indicates nothing was changed-- this is an error
		//}
		//digitsIndex += maxNumDigits;
	//}
	//
	///* Configure time bits as so: */
	///* Seconds {MSB 8 bits} + Minutes {8 bits} + Hours {8 bits} + Day  {8 bits} + Date {8 bits} + Month {8 bit} + Year {8 bit} + 0000 0000 */
	//int bitsOffSet = (totTimeUnits * BYTE_SHIFT) - BYTE_SHIFT; //starts at 48 because seconds byte takes up 8 bits
	//uint64_t timeBits = 0;
	//uint64_t tempDigit = 0;
	//for (int i = 0; i < totTimeUnits * maxNumDigits; i++)
	//{
		//if (i % maxNumDigits)
		//{
			//tempDigit |= digits[i];
			//timeBits = (tempDigit << bitsOffSet) | timeBits;
			//bitsOffSet -= BYTE_SHIFT;
			//tempDigit = 0;
		//}
		//else
		//{
			//tempDigit |= digits[i] << NIBBLE_SHIFT;
		//}
	//}
	//free(digits);
	//digits = NULL;
	//return timeBits;
//}