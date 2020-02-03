/*
 * bms_adc.hpp
 *
 *  Created on: Dec 5, 2018
 *      Author: @fedefiorini
 */

/*
 * This header contains the required operations for the analog inputs
 * of the BMS and the Current and Temperature measurements.
 *
 * It uses the library functions defined for the LPC11Cxx
 *
 */
#ifndef BMS_ADC_HPP_
#define BMS_ADC_HPP_

#include "chip.h"
#include "adc_11xx.h"
#include "configuration.hpp"
#include "bms_gpio.hpp"
#include "pins.hpp"
#include "BQ76930.hpp"
#include "bms_state.hpp"
#include <cmath>

/* LSB of ADC conversion (µV) */
#define LSB						3223

namespace adc
{
	/* Temperatures array (based on number of temperature sensors of the BMS) */
	extern int16_t temperature_readings[bms_config::n_temperature_sensors];
	/* Current measurement readout */
	extern int16_t current_sense;
	/* Temperatures samples not in range, one for each sensor (counters) */
	extern int temperature_counters[bms_config::n_temperature_sensors];
	/*
	 * Initializes the ADC pins and the peripheral
	 */
	void init_adc();

	/*
	 * Read ADC data from channel adc_channel [ADC0..ADC7]
	 *
	 * "Blocking" function: gets stuck in the while loop up until
	 * the A/D conversion is done, then reads the digital result
	 */
	uint16_t read(ADC_CHANNEL_T adc_channel);

	/*
	 * This function measures the current flowing in the Sense- and Sense+
	 * pins, connected directly to the MCU from the cells.
	 * The measured current is the one flowing through the PCB.
	 */
	void measure_current();

	/*
	 * This function measures the temperature from the three thermistors.
	 * (Temp0, Temp1, Temp2)
	 * If the temperature exceeds the maximum required it triggers an OVERTEMPERATURE error
	 * and opens the DSG MOSFET.
	 *
	 * The measured temperatures are stored in the array defined above.
	 */
	void measure_temperature();

	/*
	 * Check that temperature measurements are in the permitted range.
	 * If error syndrome persists (OT, UT) for any given sensor,
	 * disconnect LVB.
	 */
	void check(int sensor, int16_t temperature);
}



#endif /* BMS_ADC_HPP_ */
