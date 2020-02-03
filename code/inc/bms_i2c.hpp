/*
 * i2c_bms.hpp
 *
 *  Created on: Nov 29, 2018
 *      Author: @fedefiorini
 */

/*
 * This header defines the required operations for I2C in the BMS
 * Overrides the definitions in /libs/drivers/i2c because they have
 * to be adapted to the LPC11Cxx specifications
 */

#ifndef I2C_BMS_I2C_HPP_
#define I2C_BMS_I2C_HPP_

#include "chip.h"
#include "i2c_11xx.h"
#include "stdio.h"

/*
 * SCL and SDA (CLock and DAta) pins in the BMS
 * (see schematics)
 */
#define I2C_SCL 		IOCON_PIO0_4
#define I2C_SDA 		IOCON_PIO0_5
/*
 * I2C interface
 * Default: I2C0 (the only one in LPC11Cxx)
 */
#define I2C_INTERFACE	I2C0
/*
 * I2C interface speed
 * Standard: 100kHz
 *
 */
#define I2C_SPEED		100000

namespace i2c
{
	/*
	 * Initializes the I2C bus and the master (board) pins
	 *
	 * Remember to set the speed according to the I2C mode selected
	 * (see bms_i2c.cpp)
	 */
	void init(I2C_ID_T id, uint32_t speed);
	/*
	 * Sets interrupt handling for Master device
	 */
	void set_intr(I2C_ID_T id);
	/*
	 * Master send
	 */
	int send(I2C_ID_T id, uint8_t address, size_t send_size, uint8_t send_data[]);
	/*
	 * Master receive
	 */
	int receive(I2C_ID_T id, uint8_t address, size_t receive_size, uint8_t receive_data[]);
	/*
	 * Master send and receive operation
	 *
	 * This function can be used instead of both send() and receive(), the type of operation is
	 * declared leaving as "0" the size and the buffer for the other operation.
	 * i.e: for reading operations, declare send_size and send_data as 0
	 */
	int transceive(I2C_ID_T id, uint8_t address, size_t send_size, uint8_t send_data[], size_t receive_size, uint8_t receive_data[]);
	/*
	 * This function is used to communicate with the slave registers directly.
	 * Useful in case of repeated start
	 */
	int command_read(I2C_ID_T id, uint8_t address, uint8_t command, size_t size, uint8_t data[]);
}


#endif /* I2C_BMS_I2C_HPP_ */
