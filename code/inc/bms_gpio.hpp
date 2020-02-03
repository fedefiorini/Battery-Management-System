/*
 * bms_gpio.hpp
 *
 *  Created on: Nov 30, 2018
 *      Author: @fedefiorini
 */

/*
 * This header defines the required operations for the GPIO in the BMS
 * It overrides /libs/drivers/gpio because it has to be adapted to the
 * LPC 11Cxx specifications
 */

#ifndef PINS_BMS_GPIO_HPP_
#define PINS_BMS_GPIO_HPP_

#include "chip.h"
#include "gpio_11xx_2.h"
#include "iocon_11xx.h"

namespace gpio
{
	struct pin
	{
		uint8_t port;
		uint8_t pin;
	};

	/* Sets the output pin high (toggles LEDs) */
	inline void set(pin output)
	{
		Chip_GPIO_SetPinOutHigh(LPC_GPIO, output.port, output.pin);
	}

	/* Sets the output pin low (disables LEDs) */
	inline void clear(pin output)
	{
		Chip_GPIO_SetPinOutLow(LPC_GPIO, output.port, output.pin);
	}

	/*
	 *	Initialize the selected pin as output.
	 *
	 *	CHIP_IOCON_PIO_T is the enumeration containing all the LPC11Cxx GPIO pins
	 *	Select the right one from the list
	 */
	inline pin initialize_output(uint8_t port, uint8_t pin, CHIP_IOCON_PIO_T pin_name)
	{
		Chip_IOCON_PinMuxSet(LPC_IOCON, pin_name, IOCON_FUNC0 | IOCON_MODE_INACT);
		Chip_GPIO_SetPinDIROutput(LPC_GPIO, port, pin);
		Chip_GPIO_SetPinOutLow(LPC_GPIO, port, pin);
		Chip_IOCON_PinMuxSet(LPC_IOCON, pin_name, (IOCON_FUNC0 | IOCON_MODE_INACT));

		return {port, pin};
	}

	/*
	 *	Initialize the selected pin as input.
	 *	Remember: Pins sense_cur_pos (0.11) and sense_cur_neg (1.0)
	 *	have IOCON_FUNC0 marked as "R", so we need to use IOCON_FUNC1 instead
	 *
	 *	CHIP_IOCON_PIO_T is the enumeration containing all the LPC11Cxx GPIO pins
	 *	Select the right one from the list
	 */
	inline pin initialize_input(uint8_t port, uint8_t pin, CHIP_IOCON_PIO_T pin_name)
	{
		//Those pins have R as IOCON_FUNC0, while GPIO function is on IOCON_FUNC1
		if (pin_name == IOCON_PIO0_11 || pin_name == IOCON_PIO1_0)
		{
			Chip_IOCON_PinMuxSet(LPC_IOCON, pin_name, IOCON_FUNC1 | IOCON_MODE_INACT);
			Chip_GPIO_SetPinDIRInput(LPC_GPIO, port, pin);
			Chip_IOCON_PinMuxSet(LPC_IOCON, pin_name, (IOCON_FUNC1 | IOCON_MODE_INACT));
		}
		else
		{
			Chip_IOCON_PinMuxSet(LPC_IOCON, pin_name, IOCON_FUNC0 | IOCON_MODE_INACT);
			Chip_GPIO_SetPinDIRInput(LPC_GPIO, port, pin);
			Chip_IOCON_PinMuxSet(LPC_IOCON, pin_name, (IOCON_FUNC0 | IOCON_MODE_INACT));
		}

		return {port, pin};
	}

	/*
	 * Get the status of a pin.
	 * TRUE if pin is HIGH, FALSE otherwise
	 */
	inline bool get_state(pin input)
	{
		return Chip_GPIO_GetPinState(LPC_GPIO, input.port, input.pin);
	}
}
#endif /* PINS_BMS_GPIO_HPP_ */
