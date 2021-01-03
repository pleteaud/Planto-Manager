/*
 * mcp23017.c
 *
 * Created: 12/24/2020 6:25:43 PM
 *  Author: plete
 */ 

/************************************************************************/
/*                     Includes/Constants                               */
/************************************************************************/
#include "mcp23017.h"
#include "i2cMasterControl.h"
#include "timer.h"

/************************************************************************/
/*                      Private Function Declaration                    */
/************************************************************************/
static bool updateReg(mcp23017_t *ioExpander, 
				const uint8_t regAddr, 
				uint8_t pin, 
				const bool level, 
				bool updateType, 
				const uint8_t newVal);

static bool readReg(const uint8_t deviceAddr, const uint8_t regAddr, uint8_t *resp);
static bool mcpReadAllRegs(mcp23017_t *ioExpander);
static void setupRegAddrs(uint8_t mcpRegAddr[], const bool bank);
static uint8_t mcpGetAddr(uint8_t pin, uint8_t portAaddr, uint8_t portBaddr);

/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/
/**
*	Initialize mcp object and the configure the control pin's port
*	@param	ioExpander: mcp23017 instance.
*	@param	mcpAddr: device address (max value is 7: A2=1, A1=1, A0=1).
*	@param	rstDdr: reset pin data direction port
*	@param	rstPort: reset pin port
*	@param	rsPin: reset pin number
*/
void mcpInit(mcp23017_t *ioExpander, const uint8_t mcpAddr, volatile uint8_t *rstDdr, volatile uint8_t *rstPort, uint8_t rstPin)
{
	// configure appropriate bit of ddr as output
	*rstDdr |= (1 << rstPin);
	
	// Store ddr and pinnum in mcp object 
	ioExpander->rstDdr = rstDdr;
	ioExpander->rstPort = rstPort;
	ioExpander->rstPin = rstPin;
	
	// reset mcp
	mcpReset(ioExpander);
	
	
	// Update mcp address
	if (mcpAddr > 7)
		ioExpander->addr = MCP23017_DEF_ADDR | 7;
	else
		ioExpander->addr = MCP23017_DEF_ADDR | mcpAddr;
	
	// configure the address of each register according to iocon.bank = 0 (default val)
	setupRegAddrs(ioExpander->mcpRegAddrs, false);
	// Read the default register values of mcp23017 device
	mcpReadAllRegs(ioExpander);
}

/**
*	Reset mcp23017
*	@param	ioExpander: mcp23027 instance.
*/
void mcpReset(mcp23017_t *ioExpander)
{
	// set reset pin low for 10 us
	*ioExpander->rstPort &= ~(1 << ioExpander->rstPin);
	micro_delay(10);
	*ioExpander->rstPort |= (1 << ioExpander->rstPin);
}

// Set a pin's data direction
bool mcpSetPinDir(mcp23017_t *ioExpander, 
					const uint8_t port, 
					const uint8_t pin, 
					const bool level)
{	
		
	return updateReg(ioExpander, port ? ioExpander->mcpRegAddrs[MCP23017_IODIRB] : ioExpander->mcpRegAddrs[MCP23017_IODIRA], pin, level, false, 0);
}

// Set a port's data direction
bool mcpSetPortDir(mcp23017_t *ioExpander,
					const uint8_t port,
					const uint8_t portVal)
{
	return updateReg(ioExpander, port ? ioExpander->mcpRegAddrs[MCP23017_IODIRB] : ioExpander->mcpRegAddrs[MCP23017_IODIRA], 0, 0, true, portVal);
}

// Set a pin's polarity
bool mcpSetPinPol(mcp23017_t *ioExpander, 
					const uint8_t port, 
					const uint8_t pin, 
					const bool level)
{
	return updateReg(ioExpander, port ? ioExpander->mcpRegAddrs[MCP23017_IPOLB] : ioExpander->mcpRegAddrs[MCP23017_IPOLA], pin, level, false,0);
}

// Set a pin's level
bool mcpSetPinLevel(mcp23017_t *ioExpander, 
					const uint8_t port,
					const uint8_t pin,
					const bool level)
{
	return updateReg(ioExpander, port ? ioExpander->mcpRegAddrs[MCP23017_GPIOB] : ioExpander->mcpRegAddrs[MCP23017_GPIOA], pin, level, false, 0);
}

// Set a port's level
bool mcpSetPortLevel(mcp23017_t *ioExpander,
						const uint8_t port,
						const uint8_t portVal)
{
	return updateReg(ioExpander, port ? ioExpander->mcpRegAddrs[MCP23017_GPIOB] : ioExpander->mcpRegAddrs[MCP23017_GPIOA], 0, 0, true, portVal);
}

// Set whether pullup on a pin is activated or not
bool mcpSetPinPull(mcp23017_t *ioExpander, 
					const uint8_t port, 
					const uint8_t pin, 
					const bool level)
{
	return updateReg(ioExpander, port ? ioExpander->mcpRegAddrs[MCP23017_GPPUB] : ioExpander->mcpRegAddrs[MCP23017_GPPUA], pin, level, false, 0);
}

// Read a port's value
bool mcpReadPortLevel(mcp23017_t *ioExpander, 
						const uint8_t port, 
						uint8_t *dataBuff)
{
	return readReg(ioExpander->addr, port ? ioExpander->mcpRegAddrs[MCP23017_GPIOB] : ioExpander->mcpRegAddrs[MCP23017_GPIOA], dataBuff);
}

// Change the Iocon register of device
bool mcpSetIocon(mcp23017_t *ioExpander,
					const uint8_t port,
					const bool bank,
					const bool mirror,
					const bool seqop,
					const bool disslw,
					const bool odr,
					const bool intpol)
{
	/* Add functionality to change icon.bank bit */
	// Configure IOCON register
	uint8_t haen = (1 << 3); // Note bit 3 (HAEN) is always on in mcp23017 devices
	uint8_t newIocon =	(bank << 7) | (mirror << 6) | (seqop << 5) | (disslw << 4) |
						haen | (odr << 2) | (intpol << 1);

	return updateReg(ioExpander, port ? ioExpander->mcpRegAddrs[MCP23017_IOCONB] : ioExpander->mcpRegAddrs[MCP23017_IOCONA], 0, 0, true, newIocon);
}


/************************************************************************/
/*                     Private Functions Implementation                 */
/************************************************************************/

// Update register using I2C
static bool updateReg(mcp23017_t *ioExpander, 
						const uint8_t regAddr, 
						uint8_t pin, 
						const bool level, 
						bool updateType, 
						const uint8_t newVal)
{
	bool status = false;
	
	uint8_t oldReg = ioExpander->registers[regAddr];
	
	//Updating byte
	if (updateType)
		ioExpander->registers[regAddr] = newVal;
	
	// Update bit
	else
		ioExpander->registers[regAddr] = level ? oldReg | (1 << pin) : oldReg & ~(1 << pin); 
	
	uint8_t dataP[2] = { regAddr, ioExpander->registers[regAddr] };
	
	// If transmission fails, return mcp register back to previous value
	if (i2cMasterTransmit(ioExpander->addr, dataP, 2)) 
		status = true;
	else 
		ioExpander->registers[regAddr] = oldReg;
	
	return status;
}

// Read a register using i2c
static bool readReg(const uint8_t deviceAddr, const uint8_t regAddr, uint8_t *resp)
{
	bool status = false;
	uint8_t dataP = regAddr;
	
	/* Configure I2C to transmit register address */
	/* If we don't have access to I2C indicate error */
	if(!i2cMasterTransmit(deviceAddr, &dataP, 1))
		return status;
	/* Block till i2c is finished sending register address */
	if(i2cMasterRead(deviceAddr, resp, 1))
		status = true;

	return status;
}

// Configure the register addresses based on whether bank = 1 or not
static void setupRegAddrs(uint8_t mcpRegAddr[], const bool bank)
{
	if (bank)
	{
		for (int i = 0; i < NUMBER_OF_REGS; i++)
		mcpRegAddr[i] = i;
	}
	else
	{
		for (int i = 0; i < NUMBER_OF_REGS; i++)
		{
			if (i < MCP23017_IODIRB)
			mcpRegAddr[i] = 2 * i;
			else
			mcpRegAddr[i] = mcpRegAddr[i - MCP23017_IODIRB] + 1;
		}
	}
}

// REad all registers
static bool mcpReadAllRegs(mcp23017_t *ioExpander)
{
	for (int i = MCP23017_IODIRA; i < NUMBER_OF_REGS; i++)
	{
		uint8_t regAddr =  ioExpander->mcpRegAddrs[i]; // acquire the address of specific register
		uint8_t *dataP = &ioExpander->registers[regAddr];
		if(!readReg(ioExpander->addr, regAddr, dataP))
			return false; //add LED indicator of an error
	}
	return true;
}

//get port address based on pin value 
static uint8_t mcpGetAddr(uint8_t pin, uint8_t portAaddr, uint8_t portBaddr)
{
	return (pin > P7 ? portBaddr : portAaddr);
}