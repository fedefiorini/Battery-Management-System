/*
 * main.cpp
 *
 *  Created on: Nov 29, 2018
 *      Author: @fedefiorini
 */
#include "chip.h"
#include <cr_section_macros.h>

#include "bms_gpio.hpp"
#include "bms_i2c.hpp"
#include "bms_state.hpp"
#include "bms_can.hpp"
#include "bms_adc.hpp"
#include "bms_uart.hpp"
#include "pins.hpp"
#include "BQ76930.hpp"

#include "SEGGER_RTT.h"

/********GLOBAL VARIABLES********************/
/* AFE global object used by the program */
BQ76930 monitor;
/* Current state of the BMS */
state_t bms_state 				= SETUP;
/* Sanity check used when closing DSG mosfet*/
bool check 						= true;
/* Doesn't mess up with the sense_pos logic to enter
 * deep-sleep mode */
bool lvb_sense 					= false;
/* Sort of clock used to enable deep sleep mode */
uint32_t deep_sleep_timer 		= 0;
/* Enables charging procedure to remain set until setpoint is reached */
bool in_charge 					= false;
/* Counter that enables when sending CAN data (avoids bus overflowing) */
int can_update_counter 			= 0;
/* Counter that helps debouncing balancing enable and wakeup using the same button */
int balancing_enabling_count 	= 0;
/* Counter that calls balancing update according to the timeout */
int balancing_enabler			= 0;
/* Counter that enables de-bouncing of the charging procedure */
int charging_enabling_count		= 0;
/* Counter that enables resetting the state of the BMS */
int status_reset				= 0;
/********************************************/

/* BMS entry point. Should never return */
int main(void)
{
	SystemCoreClockUpdate();

	/* Initializes all peripherals and the AFE driver */
	pin::initialize_peripheral_pins();
	adc::init_adc();
	can::init_can();
	uart::init();
	timing::init();
	monitor.init();

	/* Initial state is checked twice at the beginning to get rid
	 * of transient errors such as OVRD_ALERT.
	 * If it's transient, second reading should give "OK" */
	state::status_encoder();
	state::status_encoder();

    while(1)
    {
		/*
		 * CHARGE state is entered when Delta is connected to the PCB and only when no error is present.
		 * It stays in such state until reaching the LVB target voltage setpoint.
		 * It's worth requiring that charging procedure doesn't start when the battery voltage is above a
		 * pre-determined setpoint, in order to avoid problems.
		 *
		 * During the charging procedure, RTTOUTs are required in order to oversee the correct functioning
		 */
		if ((monitor.battery_voltage <= bms_config::voltage_setpoint) && (adc::current_sense <= bms_config::charge_enable_threshold) && (bms_state == READY))
		{
			/* This allows for disconnecting the charger whenever charging has finished
			 * and not re-enter charging right after. This applies whenever the setpoint is lower
			 * than the maximum LVB voltage or when the current doesn't go below the required setpoint */
			if (charging_enabling_count++ == bms_config::charging_debounce)
			{
				in_charge = true;
				RTTOUT("Charging Initiated\n");
			}

			while (in_charge)
			{
				/* Signals charging procedure */
				gpio::set(pin::UT_ERROR);

				if (!monitor.balancing_enabled)
				{
					state::set_state(CHARGE);
				}

				monitor.read_cellvoltages();
				for (int i=0; i<bms_config::n_cells; i++)
				{
					RTTOUT("CELL VOLTAGE\t(%d): %d\n", i+1, monitor.voltage_readings[i]);
				}
				state::status_encoder();
				RTTOUT("BMS_STATE\t0x%02X\n", bms_state);

				adc::measure_current();
				RTTOUT("CURRENT\t%d\n", adc::current_sense);

				adc::measure_temperature();
				for (int i=0; i<bms_config::n_temperature_sensors; i++)
				{
					RTTOUT("TEMPERATURES\t(%d) %d\n", i+1, adc::temperature_readings[i]);
				}

				monitor.read_battery_voltage();
				RTTOUT("BATTERY VOLTAGE\t%d\n", monitor.battery_voltage);

				/***************************************************/
				if (gpio::get_state(pin::wakeup))
				{
					RTTOUT("Enable balancing when charging\n");
					monitor.balancing_enabled = true;
					state::set_state(CHARGE_AND_BAL);

					/* Signals balancing procedure */
					gpio::set(pin::UT_ERROR);
					gpio::set(pin::UV_ERROR);
				}
				if (monitor.balancing_enabled && balancing_enabler == bms_config::balancing_timeout)
				{
					monitor.check_balancing(true);
					balancing_enabler = 0;
				}
				if (monitor.balancing_enabled)
				{
					balancing_enabler++;
				}
				monitor.check_balancing(false);
				/***************************************************/

				if (adc::current_sense >= bms_config::charge_stop_threshold)
				{
					/* Exit from CHARGE state */
					RTTOUT("Charging Finished\n");
					gpio::clear(pin::UT_ERROR);
					monitor.write_register(sys_ctrl2, monitor.FET_DISABLE);
					state::set_state(READY);
					in_charge = false;
					charging_enabling_count = 0;
					check = true;
					break;
				}
			}
		}

		/* First execution of the code will enable DSG MOSFET */
		if (check)
		{
			monitor.write_register(sys_ctrl2, monitor.FET_ON);
			check = false;
		}

		/* User-enabled balancing initialization (only when no errors occurred) */
		if (gpio::get_state(pin::wakeup) && bms_state == READY && balancing_enabling_count > bms_config::balancing_debounce)
		{
			monitor.balancing_enabled = true;
			state::set_state(BALANCING);
			balancing_enabling_count = 0;

			/* Signals balancing procedure */
			gpio::set(pin::UT_ERROR);
			gpio::set(pin::UV_ERROR);
		}

		/* Balancing procedure */
		if (monitor.balancing_enabled && balancing_enabler == bms_config::balancing_timeout)
		{
			monitor.check_balancing(true);
			balancing_enabler = 0;
		}

		/* Reads LVB cells voltages */
		monitor.read_cellvoltages();
		for (int i=0; i<bms_config::n_cells; i++)
		{
			RTTOUT("CELL VOLTAGE\t(%d): %d\n", i+1, monitor.voltage_readings[i]);
		}

		/* Reads LVB pack voltage */
		monitor.read_battery_voltage();
		RTTOUT("BATTERY VOLTAGE\t%d\n", monitor.battery_voltage);

		/* Reads LVB state of charge */
		monitor.read_stateofcharge();

		/* Reads current flowing to/from the car/charger */
		adc::measure_current();
		RTTOUT("Current\t%d\n", adc::current_sense);

		/* Reads LVB cells temperatures */
		adc::measure_temperature();
		for (int i=0; i<bms_config::n_temperature_sensors; i++)
		{
			RTTOUT("TEMPERATURES\t(%d) %d\n", i+1, adc::temperature_readings[i]);
		}

		/* Status check is performed during each loop, to tackle unexpected errors immediately */
		/* FIX 27/06/2019
		 * If there's an error, we don't want to reset it immediately but let it persist for
		 * 10s, to avoid weird behaviors of the car */
		if (bms_state == READY || status_reset % bms_config::reset_count == 0)
		{
			state::status_encoder();
			status_reset = 0;
		}

		if (can_update_counter++ == bms_config::can_update_limit)
		{
			/* Prepares and send data over the CAN bus for the relevant variables */
			protocol::bms::BMSVoltages voltage_data;
			uint16_t subset[2] = { 0 };
			for (uint8_t i = 0; i < bms_config::n_cells; i++)
			{
				if (i % 2 == 0)
				{
					voltage_data.cell = i;
					if (i != 6)
					{
						subset[0] = monitor.voltage_readings[i];
						subset[1] = monitor.voltage_readings[i+1];
					}
					else
					{
						subset[0] = monitor.voltage_readings[i];
						subset[1] = 0;
					}

					for (int j=0; j<2; j++) voltage_data.voltages[j] = subset[j];
					/* Do not use. DUT19's proprietary communication protocol */
					// communication::send_data(&voltage_data);
				}
			}
			/* Do not use. DUT19's proprietary communication protocol */
			// protocol::bms::BMSBatVoltage batvoltage_data(monitor.battery_voltage);
			// communication::send_data(&batvoltage_data);
			// protocol::bms::BMSCharge charge_data(monitor.state_of_charge);
			// communication::send_data(&charge_data);
			// protocol::bms::BMSCurrent current_data(adc::current_sense);
			// communication::send_data(&current_data);
			// protocol::bms::BMSTemperatures temperature_data(adc::temperature_readings);
			// communication::send_data(&temperature_data);
			// protocol::bms::BMSState state_data(bms_state);
			// communication::send_data(&state_data);

			can_update_counter = 0;
		}

		/* Checks whether the LVB is being disconnected and starts the timer to enable
		 * the transition to deep sleep mode. It also enables lvb_sense in order to check
		 * for further connections of the LVB */
		if (!gpio::get_state(pin::sense_pos) || !gpio::get_state(pin::sense_neg))
		{
			/* If balancing is enabled, BMS should not go to Deep Sleep until it
			 * has finished doing so. */
			if (!monitor.balancing_enabled)
			{
				/* Avoids weird behaviours of the "timer" */
				if (!lvb_sense)
				{
					lvb_sense = true;
				}
				deep_sleep_timer++;

				if (deep_sleep_timer == bms_config::deep_sleep_timeout)
				{
					state::enter_sleep_state();
				}
			}
		}
		if ((gpio::get_state(pin::sense_pos) || gpio::get_state(pin::sense_neg)) && lvb_sense)
		{
			/* Disables timer as LVB is connected again */
			deep_sleep_timer = 0;
			lvb_sense = false;
		}

		/* Automatic balancing stop condition */
		monitor.check_balancing(false);

		/* Automatic increase of debouncer. The control over balancing_debounce * 2 is used
		 * to prevent variable overflow or undesired behaviors */
		if (balancing_enabling_count++ == bms_config::balancing_debounce * 2)
		{
			balancing_enabling_count = 0;
		}

		if (monitor.balancing_enabled)
		{
			balancing_enabler++;
		}

		/* When the BMS is in error state, the counter increases to avoid the error to be
		 * wiped out with a subsequent usage of the status encoder function.
		 * If car is in undervoltage error it's not safe to reset the error, so it will never be done. */
		if (bms_state != READY || bms_state != UNDERVOLTAGE)
		{
			status_reset++;
		}
    }

    return 0;
}
