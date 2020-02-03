/*
 * bms_i2c.cpp
 *
 *  Created on: Nov 29, 2018
 *      Author: @fedefiorini
 */

#include "bms_i2c.hpp"

namespace i2c
{
	void init(I2C_ID_T id, uint32_t speed)
	{
		Chip_SYSCTL_PeriphReset(RESET_I2C0);
		/*
		 * Pins 0_4 and 0_5 have to be correctly initalized using their I2C functionality
		 *
		 * Use IOCON_SFI2C_EN for standard I2C mode, IOCON_FASTI2C_EN for fast mode
		 *
		 * SM: 100kHz	100000
		 * FM: 1MHz		1000000
		 */
		Chip_I2C_Init(id);
		Chip_I2C_SetClockRate(id, speed);

		Chip_IOCON_PinMuxSet(LPC_IOCON, I2C_SCL, IOCON_FUNC1 | IOCON_MODE_PULLUP | IOCON_OPENDRAIN_EN);
		Chip_IOCON_PinMuxSet(LPC_IOCON, I2C_SDA, IOCON_FUNC1 | IOCON_MODE_PULLUP | IOCON_OPENDRAIN_EN);

		Chip_I2C_SetMasterEventHandler(id, Chip_I2C_EventHandler);
		NVIC_EnableIRQ(I2C0_IRQn);
	}

	int send(I2C_ID_T id, uint8_t address, size_t send_size, uint8_t send_data[])
	{
		return Chip_I2C_MasterSend(id, address, send_data, send_size);
	}

	int receive(I2C_ID_T id, uint8_t address, size_t receive_size, uint8_t receive_data[])
	{
		return Chip_I2C_MasterRead(id, address, receive_data, receive_size);
	}

	int transceive(I2C_ID_T id, uint8_t address, size_t send_size, uint8_t send_data[], size_t receive_size, uint8_t receive_data[])
	{
		I2C_XFER_T xfer;
		xfer.slaveAddr = address;
		xfer.txBuff = send_data;
		xfer.rxBuff = receive_data;
		xfer.txSz = send_size;
		xfer.rxSz = receive_size;

		return Chip_I2C_MasterTransfer(id, &xfer);
	}

	int command_read(I2C_ID_T id, uint8_t address, uint8_t command, size_t size, uint8_t data[])
	{
		return Chip_I2C_MasterCmdRead(id, address, command, data, size);
	}
}

/*
 * I2C Interrupt Handler (needs extern "C" declaration to properly work)
 */
extern "C" __attribute__ ((interrupt)) void I2C_IRQHandler(void)
{
	Chip_I2C_MasterStateHandler(I2C0);
}
