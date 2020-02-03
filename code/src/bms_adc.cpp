/*
 * bms_adc.cpp
 *
 *  Created on: Jan 29, 2019
 *      Author: @fedefiorini
 */
#include "bms_adc.hpp"

#include "SEGGER_RTT.h"

namespace adc
{
	ADC_CLOCK_SETUP_T adc_setup;

	int16_t temperature_readings[bms_config::n_temperature_sensors] 	= {0};
	int16_t current_sense 												= 0;
	int temperature_counters[bms_config::n_temperature_sensors] 		= {0};

	void init_adc()
	{
		Chip_ADC_Init(LPC_ADC, &adc_setup);

		/*
		 * Pin initialization (multiplexing)
		 *
		 * Sense+		PIO0_11		AD0
		 * Sense-		PIO1_0		AD1
		 * Temp0		PIO1_1		AD2
		 * Temp1		PIO1_2		AD3
		 * Current_Amp	PIO1_10		AD6		v2 only
		 * Temp2		PIO1_11		AD7
		 */
		Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO0_11, (IOCON_ADMODE_EN | IOCON_FUNC2 | IOCON_MODE_INACT));
		Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_0, (IOCON_ADMODE_EN | IOCON_FUNC2 | IOCON_MODE_INACT));
		Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_1, (IOCON_ADMODE_EN | IOCON_FUNC2 | IOCON_MODE_INACT));
		Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_2, (IOCON_ADMODE_EN | IOCON_FUNC2 | IOCON_MODE_INACT));
		Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_10, (IOCON_ADMODE_EN | IOCON_FUNC1 | IOCON_MODE_INACT)); //IOCON1_10 has Analog mode as second IOCON function
		Chip_IOCON_PinMuxSet(LPC_IOCON, IOCON_PIO1_11, (IOCON_ADMODE_EN | IOCON_FUNC1 | IOCON_MODE_INACT)); //IOCON1_11 has Analog mode as second IOCON function

		/* Enable temperature readings */
		gpio::set(pin::temp_EN);
	}

	uint16_t read(ADC_CHANNEL_T adc_channel)
	{
		uint16_t data;

		Chip_ADC_EnableChannel(LPC_ADC, adc_channel, ENABLE);

		//A/D conversion
		Chip_ADC_SetStartMode(LPC_ADC, ADC_START_NOW, ADC_TRIGGERMODE_RISING);

		//Wait for A/D conversion to be finished (loop intentionally left void)
		while (Chip_ADC_ReadStatus(LPC_ADC, adc_channel, ADC_DR_DONE_STAT) != SET) {}

		//Read value
		Chip_ADC_ReadValue(LPC_ADC, adc_channel, &data);
		Chip_ADC_EnableChannel(LPC_ADC, adc_channel, DISABLE);

		return data;
	}

	void measure_current()
	{
		uint16_t sense_voltage;

		/* Read out Current_Amp digital value */
		sense_voltage = read(ADC_CH6);

		/* Obtain readable current value in mA
		 * The values of sense_resistor and LSB anre in µΩ and µV/LSB, respectively,
		 * so it's required to convert the result in mA before reading that out */
		/* Need to subtract 1.8V from the voltage readout as it's a threshold in the
		 * OPAMP that gives out such voltage (--see schematics) */
		current_sense = int16_t(((((sense_voltage * LSB) - 1800000) / 20) / bms_config::sense_resistor));
		if (bms_state == CHARGE || bms_state == CHARGE_AND_BAL)
		{
			current_sense -= bms_config::charge_current_offset;
		}
	}

	void measure_temperature()
	{
		int16_t temperature = 0;
		uint16_t adc_out = 0;

		/* Read out values from TempX pin */
		for (int i=0; i<bms_config::n_temperature_sensors; i++)
		{
			switch(i)
			{
			case 0:
				adc_out = read(ADC_CH2);
				break;

			case 1:
				adc_out = read(ADC_CH3);
				break;

			case 2:
				adc_out = read(ADC_CH7);
				break;

			default:
				/* Unknown value, but should never get here */
				break;
			}

			/* Retrieve the temperature value from the lookup table */
			temperature = thermistor_lookup_table[adc_out];

			check(i, temperature);
			temperature_readings[i] = int16_t(temperature / 3);
		}
	}

	void check(int sensor, int16_t temperature)
	{
		/* When charging, the maximum and high temperature threshold change (lowers),
		 * thus this variable gets written with the correct thresholds at any given state
		 * whenever performing measurements */
		int16_t temperature_threshold_high;
		int16_t temperature_threshold_max;
		if (bms_state == CHARGE || bms_state == CHARGE_AND_BAL)
		{
			temperature_threshold_max 	= bms_config::charging_temperature_max;
			temperature_threshold_high 	= bms_config::charging_temperature_high;
		}
		else
		{
			temperature_threshold_max 	= bms_config::temperature_max;
			temperature_threshold_high 	= bms_config::temperature_high;
		}

		if (temperature > temperature_threshold_high * bms_config::temperature_multiplier)
		{
			if (temperature > temperature_threshold_max * bms_config::temperature_multiplier - 1)
			{
				if (temperature_counters[sensor] < bms_config::max_wrong_temp)
				{
					temperature_counters[sensor]++;
				}
				else
				{
					/* Open DSG FET and raise an error */
					monitor.write_register(sys_ctrl2, monitor.FET_DISABLE);
					state::set_state(OVERTEMPERATURE);
					gpio::set(pin::OT_ERROR);
				}
			}
			else
			{
				/* High Temperature */
			}
		}
		else if (temperature < bms_config::temperature_low * bms_config::temperature_multiplier)
		{
			if (temperature < bms_config::temperature_min * bms_config::temperature_multiplier + 1)
			{
				if (temperature_counters[sensor] < bms_config::max_wrong_temp)
				{
					temperature_counters[sensor]++;
				}
				else
				{
					/* Open DSG FET and raise an error */
					monitor.write_register(sys_ctrl2, monitor.FET_DISABLE);
					state::set_state(UNDERTEMPERATURE);
					gpio::set(pin::UT_ERROR);
				}
			}
			else
			{
				/* Low Temperature */
			}
		}
	}
}

