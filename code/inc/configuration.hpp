/*
 * configuration.hpp
 *
 *  Created on: 22 feb 2019
 *      Author: @fedefiorini
 */

/*
 * This header file contains all the definitions of constant parameters needed
 * by the Battery Management System.
 *
 * In this way, updating a single value doesn't require changing all occurrencies
 * and it also prevents from hardcoding "magic" numbers.
 *
 * All constants that require FP operations are converted to an equally precise
 * integer version, as the ARM-Cortex M0 processor doesn't have a FP unit.
 */
#ifndef CONFIGURATION_HPP_
#define CONFIGURATION_HPP_

#include "chip.h"

namespace bms_config
{
	/* Voltage ratings (mV) */
	constexpr uint16_t voltage_max 			= 4200;
	constexpr uint16_t voltage_high			= 4000;
	constexpr uint16_t voltage_low 			= 3200;
	constexpr uint16_t voltage_min			= 3000;

	/* Charging-enabling  voltage setpoint (mV) */
	constexpr uint16_t voltage_setpoint 	= 29000;

	/* OV counter (used to disconnect LVB during OV */
	constexpr int max_OV_count				= 2;

	/* Temperature ratings (°C) */
	constexpr int16_t temperature_max		= 60;
	constexpr int16_t temperature_high 		= 55;
	constexpr int16_t temperature_low		= 10;
	constexpr int16_t temperature_min		= 0;

	constexpr int16_t charging_temperature_max	= 45;
	constexpr int16_t charging_temperature_high	= 40;

	/* Constant use to maximize precision in MATLAB script */
	constexpr int temperature_multiplier	= 3;

	/* Number of temperature sensors */
	constexpr int n_temperature_sensors 	= 3;

	/* Number of OT/UT readings before triggering error */
	constexpr int max_wrong_temp			= 3;

	/* Number of cells */
	constexpr int n_cells					= 7;

	/* Maximum number of balancing cells (not adjacent!) */
	constexpr int max_balancing_cells 		= 3;

	/* Voltage difference between cells that stops balancing operations (mV) */
	constexpr uint16_t balancing_stop 		= 10;

	/* Sense resistor (mΩ)
	 * It's used in current sense calculations */
	constexpr uint16_t sense_resistor		= 2;

	/* Deep-Sleep timeout */
	constexpr uint32_t deep_sleep_timeout 	= 10000;

	/* CAN update limiter (defines the frequency of the updates */
	constexpr int can_update_limit			= 20;

	/* Button-pressing debouncer between wakeup and balancing */
	constexpr int balancing_debounce		= 50;

	/* Balancing timer */
	constexpr int balancing_timeout			= 200;

	/* Current offset used to make current measurement more precise when charging (mA) */
	constexpr int16_t charge_current_offset	= 1000;

	/* Current value that enable or stops charging the LVB (mA) */
	/* The current in enabling the charging procedure is not affected by the charging threshold,
	 * as the state at that stage is still READY, thus want to switch when the current is next to
	 * changing sign (about 1A from the power supply) */
	constexpr int16_t charge_enable_threshold 	= 100;
	constexpr int16_t charge_stop_threshold		= -500;

	/* Charging debouncer between subsequent charging activations */
	constexpr int charging_debounce			= 500;

	/* Cycle time for the main loop (ms) */
	constexpr int cycle_time				= 13;

	/* Reset count for status encoder functionality */
	constexpr int reset_count				= 385;
}

/* Lookup Table size */
const int lookup_size = 1024;

/* Lookup Table for the thermistor values.
 * It's used in the temperature measurements, since there's no FP unit in the MCU.
 * The array is created using a MATLAB script and this is the result */
const int16_t thermistor_lookup_table[lookup_size] = {1800,1425,1131,990,903,843,795,756,723,696,675,654,636,618,603,591,576,567,555,546,537,528,519,510,504,495,
													489,483,477,471,465,459,456,450,444,441,435,432,426,423,420,417,411,408,405,402,399,396,393,390,387,384,381,378,
													375,372,369,366,363,360,360,357,354,351,351,348,345,342,342,339,336,336,333,330,330,327,327,324,321,321,318,318,
													315,315,312,312,309,306,306,303,303,300,300,300,297,297,294,294,291,291,288,288,285,285,285,282,282,279,279,279,
													276,276,273,273,273,270,270,267,267,267,264,264,264,261,261,261,258,258,258,255,255,255,252,252,252,249,249,249,
													246,246,246,243,243,243,243,240,240,240,237,237,237,237,234,234,234,231,231,231,231,228,228,228,228,225,225,225,
													225,222,222,222,222,219,219,219,219,216,216,216,216,213,213,213,213,210,210,210,210,210,207,207,207,207,204,204,
													204,204,204,201,201,201,201,201,198,198,198,198,198,195,195,195,195,195,192,192,192,192,192,189,189,189,189,189,
													186,186,186,186,186,183,183,183,183,183,183,180,180,180,180,180,177,177,177,177,177,177,174,174,174,174,174,174,
													171,171,171,171,171,171,168,168,168,168,168,168,165,165,165,165,165,165,162,162,162,162,162,162,162,159,159,159,
													159,159,159,156,156,156,156,156,156,156,153,153,153,153,153,153,153,150,150,150,150,150,150,150,147,147,147,147,
													147,147,147,144,144,144,144,144,144,144,141,141,141,141,141,141,141,138,138,138,138,138,138,138,138,135,135,135,
													135,135,135,135,135,132,132,132,132,132,132,132,129,129,129,129,129,129,129,129,126,126,126,126,126,126,126,126,
													123,123,123,123,123,123,123,123,123,120,120,120,120,120,120,120,120,117,117,117,117,117,117,117,117,114,114,114,
													114,114,114,114,114,114,111,111,111,111,111,111,111,111,111,108,108,108,108,108,108,108,108,105,105,105,105,105,
													105,105,105,105,102,102,102,102,102,102,102,102,102,99,99,99,99,99,99,99,99,99,99,96,96,96,96,96,96,96,96,96,93,
													93,93,93,93,93,93,93,93,90,90,90,90,90,90,90,90,90,90,87,87,87,87,87,87,87,87,87,84,84,84,84,84,84,84,84,84,84,81,
													81,81,81,81,81,81,81,81,81,78,78,78,78,78,78,78,78,78,78,75,75,75,75,75,75,75,75,75,72,72,72,72,72,72,72,72,72,72,
													69,69,69,69,69,69,69,69,69,69,66,66,66,66,66,66,66,66,66,66,63,63,63,63,63,63,63,63,63,63,63,60,60,60,60,60,60,60,
													60,60,60,57,57,57,57,57,57,57,57,57,57,54,54,54,54,54,54,54,54,54,54,51,51,51,51,51,51,51,51,51,51,48,48,48,48,48,
													48,48,48,48,48,45,45,45,45,45,45,45,45,45,45,45,42,42,42,42,42,42,42,42,42,42,39,39,39,39,39,39,39,39,39,39,36,36,
													36,36,36,36,36,36,36,36,33,33,33,33,33,33,33,33,33,33,30,30,30,30,30,30,30,30,30,30,27,27,27,27,27,27,27,27,27,27,
													24,24,24,24,24,24,24,24,24,24,21,21,21,21,21,21,21,21,21,21,18,18,18,18,18,18,18,18,18,15,15,15,15,15,15,15,15,15,
													15,12,12,12,12,12,12,12,12,12,9,9,9,9,9,9,9,9,9,9,6,6,6,6,6,6,6,6,6,3,3,3,3,3,3,3,3,3,0,0,0,0,0,0,0,0,0,-3,-3,-3,-3,
													-3,-3,-3,-3,-3,-6,-6,-6,-6,-6,-6,-6,-6,-6,-9,-9,-9,-9,-9,-9,-9,-9,-9,-12,-12,-12,-12,-12,-12,-12,-12,-15,-15,-15,-15,
													-15,-15,-15,-15,-18,-18,-18,-18,-18,-18,-18,-18,-21,-21,-21,-21,-21,-21,-21,-21,-24,-24,-24,-24,-24,-24,-24,-24,-27,
													-27,-27,-27,-27,-27,-27,-27,-30,-30,-30,-30,-30,-30,-30,-33,-33,-33,-33,-33,-33,-33,-36,-36,-36,-36,-36,-36,-36,-39,
													-39,-39,-39,-39,-39,-39,-42,-42,-42,-42,-42,-42,-42,-45,-45,-45,-45,-45,-45,-45,-48,-48,-48,-48,-48,-48,-51,-51,-51,
													-51,-51,-51,-54,-54,-54,-54,-54,-54,-57,-57,-57,-57,-57,-57,-60,-60,-60,-60,-60,-63,-63,-63,-63,-63,-63,-66,-66,-66,
													-66,-66,-69,-69,-69,-69,-69,-72,-72,-72,-72,-72,-75,-75,-75,-75,-78,-78,-78,-78,-78,-81,-81,-81,-81,-84,-84,-84,-84,
													-87,-87,-87,-87,-90,-90,-90,-90,-93,-93,-93,-93,-96,-96,-96,-99,-99,-99,-102,-102,-102,-102,-105,-105,-105,-108,-108,
													-108,-111,-111,-114,-114,-114,-117,-117,-117,-120,-120,-123,-123,-126,-126,-129,-129,-132,-132,-135,-135,-138,-138,
													-141,-141,-144,-147,-147,-150,-153,-153,-156,-159,-162,-165,-168,-171,-174,-177,-180,-186,-189,-195,-201,-207,-216,
													-228,-240,-264};


#endif /* CONFIGURATION_HPP_ */
