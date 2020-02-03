/*
 * bms_uart.hpp
 *
 *  Created on: Mar 25, 2019
 *      Author: @fedefiorini
 */

/*
 * This header contains the required functionalities for UART communication in the BMS.
 * It overrides libs/drivers/uart as this driver has to be adapted to
 * work under LPC11xx device specifications
 */
#ifndef BMS_UART_HPP_
#define BMS_UART_HPP_

#include "chip.h"
#include "uart_11xx.h"
#include "ring_buffer.h"

#include "stdio.h"
#include "string.h"

namespace uart
{
	/* Transmit and Receive ring buffers */
	extern RINGBUFF_T send_buffer;
	extern RINGBUFF_T recv_buffer;

	/* Size of the two ring buffers */
	const int send_fifo_size	= 128;
	const int recv_fifo_size	= 32;

	/* Transmit and Receive buffers associated with the ring buffers */
	extern uint16_t tx_buffer[send_fifo_size];
	extern uint16_t rx_buffer[recv_fifo_size];

	/*
	 * Initializes UART peripheral and pins, as well as the two ring buffers used as FIFO queues
	 */
	void init();

	/*
	 * Sends a message over UART
	 *
	 * \parameters
	 * tx_data		buffer to be sent via UART
	 * length		length of the buffer
	 */
	void send(uint16_t *tx_data, int length);

	/*
	 * Receives a message over UART
	 */
	void receive();
}




#endif /* BMS_UART_HPP_ */
