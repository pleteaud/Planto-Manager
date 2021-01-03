/*
 * mcp23017.h
 *
 * Created: 12/24/2020 4:06:06 PM
 *  Author: plete
 */ 


#ifndef MCP23017_H_
#define MCP23017_H_

/************************************************************************/
/*							Includes/Constants	 	                    */
/************************************************************************/
#include <stdbool.h>
#include <stdint.h>

#define MCP23017_DEF_ADDR		(0x20)	// MCP23017 default address A2 = A1 = A0 = 0, so address is 0010 0000

// Port A & Port B pin alias
#define MCP23017_PORTB			(0x01)	// PORTB alias
#define MCP23017_PORTA			(0x00)	// PORTA alias
// Pin numbers. each port has 8 pins
#define P0						(0x00)
#define P1						(0x01)
#define P2						(0x02)
#define P3						(0x03)
#define P4						(0x04)
#define P5						(0x05)
#define P6						(0x06)
#define P7						(0x07)
#define P8						(0x08)
#define P9						(0x09)
#define P10						(0x0A)
#define P11						(0x0B)
#define P12						(0x0C)
#define P13						(0x0D)
#define P14						(0x0E)
#define P15						(0x0F)

/************************************************************************/
/*						Enums Definition				                */
/************************************************************************/

enum 
{
	// Port A and Port B are seperated into two banks 
	
	// Bank 1: Port A registers
	MCP23017_IODIRA = 0,		// I/O direction register A
	MCP23017_IPOLA,				// Input polarity port register A
	MCP23017_GPINTENA,			// Interrupt-on-change pins A
	MCP23017_DEFVALA,			// Default value register A
	MCP23017_INTCONA,			// Interrupt-on-change control register A
	MCP23017_IOCONA,			// I/O expander configuration register A
	MCP23017_GPPUA,				// GPIO pull-up resistor register A
	MCP23017_INTFA,				// Interrupt flag register A
	MCP23017_INTCAPA,			// Interrupt captured value for port register A
	MCP23017_GPIOA,				// General purpose I/O port register A
	MCP23017_OLATA,				// Output latch register 0 A
	// Bank 2: Port B  registers
	MCP23017_IODIRB,			// I/O direction register B
	MCP23017_IPOLB,				// Input polarity port register B
	MCP23017_GPINTENB,			// Interrupt-on-change pins B
	MCP23017_DEFVALB,			// Default value register B
	MCP23017_INTCONB,			// Interrupt-on-change control register B
	MCP23017_IOCONB,			// I/O expander configuration register B
	MCP23017_GPPUB,				// GPIO pull-up resistor register B
	MCP23017_INTFB,				// Interrupt flag register B
	MCP23017_INTCAPB,			// Interrupt captured value for port register B
	MCP23017_GPIOB,				// General purpose I/O port register B
	MCP23017_OLATB,				// Output latch register 0 B
	NUMBER_OF_REGS	
};


/************************************************************************/
/*				Type Defs + Struct Declaration							*/
/************************************************************************/


typedef struct mcp23017_s 
{
	uint8_t addr;
	uint8_t mcpRegAddrs[NUMBER_OF_REGS];
	uint8_t registers[NUMBER_OF_REGS];
	volatile uint8_t *rstDdr;
	volatile uint8_t *rstPort;
	uint8_t	rstPin;
	
} mcp23017_t;

/************************************************************************/
/*							Public Interfaces    	                    */
/************************************************************************/
void mcpInit(mcp23017_t *ioExpander, const uint8_t mcpAddr, volatile uint8_t *rstDdr, volatile uint8_t *rstPort, uint8_t rstPin);

void mcpReset(mcp23017_t *ioExpander);

bool mcpSetPinDir(mcp23017_t *ioExpander, const uint8_t port, const uint8_t pin, const bool level);

bool mcpSetPortDir(mcp23017_t *ioExpander, const uint8_t port, const uint8_t portVal);

bool mcpSetPinLevel(mcp23017_t *ioExpander, const uint8_t port, const uint8_t pin, const bool level);

bool mcpSetPortLevel(mcp23017_t *ioExpander, const uint8_t port, const uint8_t portVal);

bool mcpSetPinPol(mcp23017_t *ioExpander, const uint8_t port, const uint8_t pin, const bool level);

bool mcpSetPinPull(mcp23017_t *ioExpander, const uint8_t port, const uint8_t pin, const bool level);

bool mcpReadPortLevel(mcp23017_t *ioExpander, const uint8_t port, uint8_t *dataBuff);

bool mcpSetIocon(mcp23017_t *ioExpander, 
					const uint8_t port, 
					const bool bank, 
					const bool mirror,
					const bool seqop, 
					const bool disslw, 
					const bool odr, 
					const bool intpol);
					
// Need to add interrupt functionality in the future
// need to add function to modify/read output latch register


#endif /* MCP23017_H_ */