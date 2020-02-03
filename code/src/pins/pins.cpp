/*
 * pins.cpp
 *
 *  Created on: Nov 29, 2018
 *      Author: @fedefiorini
 */

#include "pins.hpp"

namespace pin
{
	gpio::pin OK_LED;
	gpio::pin ERROR_LED;
	gpio::pin OV_ERROR;
	gpio::pin UV_ERROR;
	gpio::pin OC_ERROR;
	gpio::pin OT_ERROR;
	gpio::pin UT_ERROR;

	gpio::pin CAN_EN;

	gpio::pin UART_EN;

	gpio::pin temp_EN;

	gpio::pin sense_neg;
	gpio::pin sense_pos;
	gpio::pin ALERT;
	gpio::pin push;
	gpio::pin wakeup;

	void initialize_peripheral_pins()
	{
		Chip_GPIO_Init(LPC_GPIO);

		/* OUTPUT PINS */
		//LEDs
		OK_LED 		= gpio::initialize_output(2, 7, IOCON_PIO2_7);
		ERROR_LED	= gpio::initialize_output(2, 8, IOCON_PIO2_8);
		OV_ERROR	= gpio::initialize_output(2, 0, IOCON_PIO2_0);
		OT_ERROR	= gpio::initialize_output(2, 6, IOCON_PIO2_6);
		OC_ERROR	= gpio::initialize_output(0, 3, IOCON_PIO0_3);
		UV_ERROR	= gpio::initialize_output(2, 5, IOCON_PIO2_5);
		UT_ERROR	= gpio::initialize_output(0, 7, IOCON_PIO0_7);

		//Enable pins
		UART_EN		= gpio::initialize_output(3, 3, IOCON_PIO3_3);
		CAN_EN		= gpio::initialize_output(2, 4, IOCON_PIO2_4);
		temp_EN		= gpio::initialize_output(3, 0, IOCON_PIO3_0);


		/* INPUT PINS */
		wakeup 		= gpio::initialize_input(0, 6, IOCON_PIO0_6);
		push		= gpio::initialize_input(1, 4, IOCON_PIO1_4);
		ALERT 		= gpio::initialize_input(0, 2, IOCON_PIO0_2);
		sense_pos	= gpio::initialize_input(0, 8, IOCON_PIO0_8);
		sense_neg	= gpio::initialize_input(0, 9, IOCON_PIO0_9);
	}
}

