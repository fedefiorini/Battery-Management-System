/*
 * bms_can.hpp
 *
 *  Created on: Dec 5, 2018
 *      Author: @fedefiorini
 */

/*
 * This header contains the declarations for the CAN driver
 * to work in the LPC11Cxx environment.
 * It's directly based on the DUT17 sensornode's CAN implementation.
 *
 * It also contains the required functionalities to enable CAN communication
 * between the BMS and the ECU, using the existing protocol (in /libs/protocol)
 */

#ifndef BMS_CAN_HPP_
#define BMS_CAN_HPP_

/* CAN-specific includes */
#include "chip.h"
#include "ccand_11xx.h"
/* Communication-specific includes */
#include "configuration.hpp"
#include "protocol.hpp"
#include "bms.hpp"
#include "pins.hpp"

namespace can
{
	/*
	 * CAN message structure
	 */
	struct message
	{
		uint16_t id;
		uint8_t data[8];
		uint8_t length;
	};
	/*
	 * Initialize the CAN peripheral
	 */
	void init_can();
	/*
	 * Send a message over the CAN bus
	 */
	void send(message *msg);
	/*
	 * Receive a message from the CAN bus
	 * It's going to be used by the BMS for ACK reception
	 * from the ECU during registration process
	 *
	 * BMS doesn't receive commands from the ECU
	 */
	bool receive(message *msg);
}

namespace communication
{
	/* Sends CAN packages over the CAN bus (updates on BMS status) */
	void send_data(protocol::CanPackage *data_package);
}


#endif /* BMS_CAN_HPP_ */
