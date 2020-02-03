/*
 * bms_uart.cpp
 *
 *  Created on: Mar 25, 2019
 *      Author: @fedefiorini
 */
#include "bms_uart.hpp"
#include "pins.hpp"

namespace uart
{
	/* Transmit and Receive ring buffers */
	RINGBUFF_T send_buffer;
	RINGBUFF_T recv_buffer;

	/* Transmit and Receive buffers associated with the ring buffers */
	uint16_t tx_buffer[send_fifo_size];
	uint16_t rx_buffer[recv_fifo_size];

	/* Maximum size of UART messages */
	const int msg_length		= 64;

	void init()
	{
		/* Init UART peripheral */
		Chip_UART_Init(LPC_USART);

		/* UART Pins initialization */
		Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_6, IOCON_FUNC1 | IOCON_MODE_INACT);	//RXD
		Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_7, IOCON_FUNC1 | IOCON_MODE_INACT);	//TXD

		/* Init ring buffers */
		RingBuffer_Init(&send_buffer, tx_buffer, msg_length, send_fifo_size);
		RingBuffer_Init(&recv_buffer, rx_buffer, msg_length, recv_fifo_size);

		/* Sets Baud rate to default: 115200 */
		Chip_UART_SetBaud(LPC_USART, 115200);

		/*
		 * Defines options on data exchanged through UART
		 *
		 * UART_LCR_WLEN8			Word length is 8 bits
		 * UART_LCR_SBS_1BIT		1 Stop bit
		 * UART_LCR_PARITY_DIS		Parity check disabled
		 */
		Chip_UART_ConfigData(LPC_USART, UART_LCR_WLEN8 | UART_LCR_SBS_1BIT | UART_LCR_PARITY_DIS);

		/*
		 * Defines options for the FIFO queues
		 *
		 * UART_FCR_FIFO_EN			UART FIFO Enable
		 * UART_FCR_RX_RS			Receive FIFO reset
		 * UART_FCR_TX_RS			Transmit FIFO reset
		 * UART_FCR_TRG_LEV3		FIFO trigger level 3 (14 characters)
		 */
		Chip_UART_SetupFIFOS(LPC_USART, UART_FCR_FIFO_EN | UART_FCR_RX_RS | UART_FCR_TX_RS | UART_FCR_TRG_LEV3);

		/* Enables transmission on TX pin */
		Chip_UART_TXEnable(LPC_USART);

		/*
		 * Enables two specific interrupts
		 *
		 * UART_IER_RBRINT			RBR Interrupt enabled	(RBR = Receiver Buffer Register , contains the next character received to be read)
		 * UART_IER_THREINT			THR Interrupt enabled	(THR = Transmit Holding Register , contains the next character to be written)
		 */
		Chip_UART_IntEnable(LPC_USART, (UART_IER_RBRINT | UART_IER_RLSINT));

		/* Enables the UART interrupt */
		NVIC_EnableIRQ(UART0_IRQn);
	}

	void send(uint16_t *tx_data, int length)
	{
		/* Enables UART transmission */
		gpio::set(pin::UART_EN);

		/* If try to send more than maximum, do nothing */
		if (length > msg_length) return;

		/* Populates the send_buffer and then sends it via UART */
		Chip_UART_SendRB(LPC_USART, &send_buffer, tx_data, length);
	}

	void receive()
	{
		/* Enables UART receiving */
		gpio::set(pin::UART_EN);

		Chip_UART_ReadRB(LPC_USART, &recv_buffer, rx_buffer, recv_fifo_size);
	}
}

extern "C" __attribute__((interrupt)) void UART_IRQHandler ( void )
{
	/* Some error handling here?? */

	/* Uses the default IRQ Handler for ring buffers */
	/*
	 * It will be called upon any send() or receive() call, and first it will send anything
	 * inside the send_buffer, then it will handle receiving any message present in recv_buffer
	 */
	Chip_UART_IRQRBHandler(LPC_USART, &uart::recv_buffer, &uart::send_buffer);
}

