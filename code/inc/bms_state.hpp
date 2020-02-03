/*
 * bms_state.hpp
 *
 *  Created on: Dec 4, 2018
 *      Author: @fedefiorini
 */
#ifndef BMS_STATE_HPP_
#define BMS_STATE_HPP_

#include "chip.h"

#include "sysctl_11xx.h"
#include "clock_11xx.h"
#include "pmu_11xx.h"
#include "cmsis.h"
#include "BQ76930.hpp"
#include "pins.hpp"
#include "bms_adc.hpp"
#include "bms_can.hpp"
/*
 * This header contains the definition of the BMS states
 * It's used runtime to discriminate between different operation
 * modes for the BMS.
 * Explanation for each state is below in the enum
 *
 * This header redefines /libs/statemanager/statemanager.hpp
 * to be compliant to the LPC11Cxx definitions.
 * In particular, it doesn't use RTOS++ (that's why it's needed)
 */

/*
 * This enum contains the possible states and errors in which the BMS is during the execution.
 *
 * SETUP 		Boot and startup phase
 * SLEEP		LVB is detached from the car, BMS is not working (but still powered on)
 * READY		The car is running and BMS is monitoring the LVB
 *
 * CHARGING		The LVB is charging, but it's still not connected to the car
 * It's not a proper state handled by the BMS, but it's used to discriminate the main loop behavior
 *
 * BALANCING	The BMS is handling balancing of the LVB
 * It's not a proper state handled by the BMS, but it's used to determine when the LVB is actually balacing.
 * The bms_state needs to be in READY to enable balancing when not charging!
 *
 * Errors that might appear in the Analog Front End measurements.
 * Voltage and Current conditions are directly treated from the AFE (blocking DSG FET)
 * but OT and UT need specific handling from the MCU.
 *
 * The error syndromes are encoded as the ones read out directly from the AFE (see BQ76930.hpp/.cpp
 * for further details), that's the reason for this placement.
 *
 * ALL ERROR STATES ARE UNRECOVERABLE FOR THE BMS (LVB DISCONNECT)
 *
 */
enum state_t : uint8_t
{
	/* Current Errors */
	OVERCURRENT 		= 0x01,
	SHORTCIRCUIT 		= 0x02,

	/* Voltage Errors */
	OVERVOLTAGE 		= 0x04,
	UNDERVOLTAGE 		= 0x08,

	/* AFE fault indicator */
	AFE_FAULT			= 0x20,

	/* I2C fault indicator */
	I2C_FAIL			= 0x30,

	/* Transient errors */
	ERR_TRANSIENT		= 0x40,

	/* LEGAL STATES */
	SETUP				= 0xD0,
	SLEEP				= 0xD1,
	READY				= 0xD2,
	CHARGE				= 0xD3,
	BALANCING			= 0xD4,
	CHARGE_AND_BAL		= 0xD5,

	/* Wrong state transition */
	ILLEGAL_STATE		= 0xE0,

	/* Temperature Errors */
	OVERTEMPERATURE	 	= 0xFE,
	UNDERTEMPERATURE	= 0xFF
};

/*
 * BMS state. This variable is maintained through
 * the program execution and it contains the current state
 */
extern state_t bms_state;

/*
 * AFE Class object.
 * Declared here and used anywhere in the code (defined in bms.cpp)
 */
extern BQ76930 monitor;
/*
 * Added small check for AFE FET closing
 */
extern bool check;
/*
 * "Software Timer" to enter deep sleep when the LVB is disconnected
 */
extern uint32_t deep_sleep_timer;
/*
 * Sanity check used to stop above timer whenever LVB is connected again
 */
extern bool lvb_sense;
/*
 * Variable used to continue CHARGE procedure until setpoint reached
 */
extern bool in_charge;
/*
 * Enables CAN messages to be sent at a pre-determined rate (avoids bus overflow)
 */
extern int can_update_counter;
/*
 * Debounces balancing procedure and wakeup interrupt handler (both use Button1 to be enabled)
 */
extern int balancing_enabling_count;
/*
 * Counter that calls balancing update according to the timeout
 */
extern int balancing_enabler;
/*
 * Counter that enables de-bouncing of the charging procedure
 */
extern int charging_enabling_count;

namespace state
{
	/*
	 * State transition structure (comprises an old state and a new one)
	 */
	struct state_transition
	{
		state_t old_;
		state_t new_;
	};
	/*
	 * Valid state transitions.
	 * It's used to avoid messing up with the state machine
	 */
	extern state_transition valid_state_transitions[10	];

	inline bool is_valid_transition(state_t old_state, state_t new_state)
	{
		for (uint8_t i=0; i<sizeof(valid_state_transitions); i++)
		{
			if ((valid_state_transitions[i].old_ == old_state) && (valid_state_transitions[i].new_ = new_state))
			{
				return true;
			}
		}
		return false;
	}

	inline void set_state(state_t new_state)
	{
		if (is_valid_transition(bms_state, new_state))
		{
			bms_state = new_state;
		}
		else bms_state = ILLEGAL_STATE;
	}
	/*
	 * This function encodes the SYS_STAT register output into readable and
	 * understandable error codes.
	 * The status is cleared as soon as it's read, to maintain transient errors out
	 * of the handling.
	 *
	 * \param monitor	Instance of the BQ76930 class (declared in bms.cpp)
	 */
	void status_encoder();
	/*
	 * PMU initialization and required steps to set up the DEEP SLEEP mode
	 * defined in the LPC11Cxx user manual.
	 *
	 * For further references on how to setup low power consumption
	 * modes, check UM10398.
	 *
	 * After setup has finished, it enters DEEP SLEEP mode.
	 *
	 * (it just waits for interrupts to happen)
	 * __WFI();
	 *
	 * The chip will wake up as soon as it receives an interrupt,
	 * and such signals are sent using the FALLING edge of pre-determined pins
	 *
	 * PIO0_2	ALERT		input from TI bq76930
	 * PIO0_6	Push		external (wakeup) button
	 * PIO0_8	sense_pos	current sense positive terminal
	 * PIO0_9	sense_neg	current sense negative terminal
	 *
	 */
	void enter_sleep_state();
}

#endif /* BMS_STATE_HPP_ */
