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
static bool updateReg(mcp23017_t *deviceP, 
				const uint8_t regAddr, 
				uint8_t pin, 
				const bool level, 
				bool updateType, 
				const uint8_t newVal);

static bool readReg(const uint8_t deviceAddr, const uint8_t regAddr, uint8_t *resp);
static bool readAllRegs(mcp23017_t *deviceP);
static void setupRegAddrs(uint8_t mcpRegAddr[], const bool bank);
static uint8_t mcpGetAddr(uint8_t pin, uint8_t portAaddr, uint8_t portBaddr);

/************************************************************************/
/*                      Public Functions Implementations                */
/************************************************************************/
/**
*	Initialize mcp object and the configure the control pin's port
*	@param	deviceP: mcp23017 instance.
*	@param	mcpAddr: device address (max value is 7: A2=1, A1=1, A0=1).
*	@param	rstDdr: reset pin data direction port
*	@param	rstPort: reset pin port
*	@param	rsPin: reset pin number
*/
void mcp23017Init(mcp23017_t *deviceP, const uint8_t mcpAddr, volatile uint8_t *rstDdr, volatile uint8_t *rstPort, uint8_t rstPin)
{
	// configure appropriate bit of ddr as output
	*rstDdr |= (1 << rstPin);
	
	// Store ddr and pinnum in mcp object 
	deviceP->rstDdr = rstDdr;
	deviceP->rstPort = rstPort;
	deviceP->rstPin = rstPin;
	
	// reset mcp
	mcp23017Reset(deviceP);
	
	// Update mcp address
	if (mcpAddr > 7)
		deviceP->addr = MCP23017_DEF_ADDR | 7;
	else
		deviceP->addr = MCP23017_DEF_ADDR | mcpAddr;
	
	// configure the address of each register according to iocon.bank = 0 (default val)
	setupRegAddrs(deviceP->mcpRegAddrs, false);
	// Read the default register values of mcp23017 device
	readAllRegs(deviceP);
}

/**
*	Reset mcp23017
*	@param	deviceP: mcp23027 instance.
*/
void mcp23017Reset(mcp23017_t *deviceP)
{
	// set reset pin low for 10 us
	*deviceP->rstPort &= ~(1 << deviceP->rstPin);
	micro_delay(10);
	*deviceP->rstPort |= (1 << deviceP->rstPin);
}

// Set a pin's data direction
bool mcp23017SetPinDir(mcp23017_t *deviceP, 
					const uint8_t port, 
					const uint8_t pin, 
					const bool level)
{	
		
	return updateReg(deviceP, port ? deviceP->mcpRegAddrs[MCP23017_IODIRB] : deviceP->mcpRegAddrs[MCP23017_IODIRA], pin, level, false, 0);
}

// Set a port's data direction
bool mcp23017SetPortDir(mcp23017_t *deviceP,
					const uint8_t port,
					const uint8_t portVal)
{
	return updateReg(deviceP, port ? deviceP->mcpRegAddrs[MCP23017_IODIRB] : deviceP->mcpRegAddrs[MCP23017_IODIRA], 0, 0, true, portVal);
}

// Set a pin's polarity
bool mcp23017SetPinPol(mcp23017_t *deviceP, 
					const uint8_t port, 
					const uint8_t pin, 
					const bool level)
{
	return updateReg(deviceP, port ? deviceP->mcpRegAddrs[MCP23017_IPOLB] : deviceP->mcpRegAddrs[MCP23017_IPOLA], pin, level, false,0);
}

// Set a pin's level
bool mcp23017SetPinLevel(mcp23017_t *deviceP, 
					const uint8_t port,
					const uint8_t pin,
					const bool level)
{
	return updateReg(deviceP, port ? deviceP->mcpRegAddrs[MCP23017_GPIOB] : deviceP->mcpRegAddrs[MCP23017_GPIOA], pin, level, false, 0);
}

// Set a port's level
bool mcp23017SetPortLevel(mcp23017_t *deviceP,
						const uint8_t port,
						const uint8_t portVal)
{
	return updateReg(deviceP, port ? deviceP->mcpRegAddrs[MCP23017_GPIOB] : deviceP->mcpRegAddrs[MCP23017_GPIOA], 0, 0, true, portVal);
}

// Set whether pullup on a pin is activated or not
bool mcp23017SetPinPull(mcp23017_t *deviceP, 
					const uint8_t port, 
					const uint8_t pin, 
					const bool level)
{
	return updateReg(deviceP, port ? deviceP->mcpRegAddrs[MCP23017_GPPUB] : deviceP->mcpRegAddrs[MCP23017_GPPUA], pin, level, false, 0);
}

// Read a port's value
bool mcp23017ReadPortLevel(mcp23017_t *deviceP, 
						const uint8_t port, 
						uint8_t *dataBuff)
{
	return readReg(deviceP->addr, port ? deviceP->mcpRegAddrs[MCP23017_GPIOB] : deviceP->mcpRegAddrs[MCP23017_GPIOA], dataBuff);
}

// Change the Iocon register of device
bool mcp23017SetIocon(mcp23017_t *deviceP,
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

	return updateReg(deviceP, port ? deviceP->mcpRegAddrs[MCP23017_IOCONB] : deviceP->mcpRegAddrs[MCP23017_IOCONA], 0, 0, true, newIocon);
}


/************************************************************************/
/*                     Private Functions Implementation                 */
/************************************************************************/

// Update register using I2C
static bool updateReg(mcp23017_t *deviceP, const uint8_t regAddr, uint8_t pin, const bool level, 
					  bool updateType, const uint8_t newVal)
{
	bool status = false;
	
	uint8_t oldReg = deviceP->registers[regAddr];
	
	//Updating byte
	if (updateType)
		deviceP->registers[regAddr] = newVal;
	
	// Update bit
	else
		deviceP->registers[regAddr] = level ? oldReg | (1 << pin) : oldReg & ~(1 << pin); 
	
	uint8_t dataP[2] = { regAddr, deviceP->registers[regAddr] };
	
	// If transmission fails, return mcp register back to previous value
	if (i2cMasterTransmit(deviceP->addr, dataP, 2)) 
		status = true;
	else 
		deviceP->registers[regAddr] = oldReg;
	
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
static bool readAllRegs(mcp23017_t *deviceP)
{
	for (int i = MCP23017_IODIRA; i < NUMBER_OF_REGS; i++)
	{
		uint8_t regAddr =  deviceP->mcpRegAddrs[i]; // acquire the address of specific register
		uint8_t *dataP = &deviceP->registers[regAddr];
		if(!readReg(deviceP->addr, regAddr, dataP))
			return false; 
	}
	return true;
}

//get port address based on pin value 
static uint8_t mcpGetAddr(uint8_t pin, uint8_t portAaddr, uint8_t portBaddr)
{
	return (pin > P7 ? portBaddr : portAaddr);
}