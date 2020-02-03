/*
 * pins.hpp
 *
 *  Created on: Nov 29, 2018
 *      Author: @fedefiorini
 */

/*
 * This header contains the definition of all the pins used by the BMS MCU
 */

#ifndef PINS_PINS_HPP_
#define PINS_PINS_HPP_

#include "bms_gpio.hpp"

namespace pin
{
	//LEDs [0..6]	configured to represent different errors or situations that might occur
	extern gpio::pin OK_LED;					//Everything's fine (duh..)
	extern gpio::pin ERROR_LED; 				//AFE_ERROR
	extern gpio::pin OV_ERROR;
	extern gpio::pin UV_ERROR;
	extern gpio::pin OC_ERROR;
	extern gpio::pin OT_ERROR;
	extern gpio::pin UT_ERROR;

	//CAN enable
	extern gpio::pin CAN_EN;

	//UART enable
	extern gpio::pin UART_EN;

	//Temperature enable
	extern gpio::pin temp_EN;

	//Current (from BMS_current_sense --see schematics)
	//Those pins are used to wake up the chip
	extern gpio::pin sense_neg;
	extern gpio::pin sense_pos;

	//Alert Pin: used as output interrupt (TI->LPC) or as override input (LPC->TI)
	extern gpio::pin ALERT;

	//Other pins
	extern gpio::pin push;
	extern gpio::pin wakeup;

	/*
	 * This function initializes the GPIO pins
	 * according to their specification
	 *
	 * Every other peripheral pins (i.e. belonging to
	 * ADC, I2C and UART are initialized in the respective
	 * headers and/or classes
	 */
	void initialize_peripheral_pins();

}

#endif /* PINS_PINS_HPP_ */
