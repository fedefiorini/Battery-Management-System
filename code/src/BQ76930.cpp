/*
 * BQ76930.cpp
 *
 *  Created on: Jan 17, 2019
 *      Author: @fedefiorini
 */

#include "BQ76930.hpp"

void BQ76930::init()
{
	i2c::init(I2C_INTERFACE, I2C_SPEED);

	//CC_CFG default value
	write_register(cc_cfg, CC_CFG);

	//Enable ADC (and OV_/UV_protection)
	write_register(sys_ctrl1, ADC_EN);

	//OV threshold
	write_register(ov_trip, OV_THRESH);

	//UV threshold
	write_register(uv_trip, UV_THRESH);

	//OCD threshold and delay
	write_register(protect1, SCD_VAL);

	//SCD threshold and delay
	write_register(protect2, OCD_VAL);

	//Delay settings
	write_register(protect3, VOLT_DELAY);

	//Disables the DSG or CHG MOSFETs, prevents errors
	write_register(sys_ctrl2, FET_DISABLE);

	//Disables balancing operations at startup, prevents errors
	write_register(cellbal1, BAL_OFF);
	write_register(cellbal2, BAL_OFF);
}

void BQ76930::write_register(const TI_Register_ID reg, uint8_t data)
{
	uint8_t crc_val = 0;

	/* Calculate CRC on the specific register and data to be sent */
	crc_wr[0] = I2C_ADDRESS << 1;
	crc_wr[1] = reg;
	crc_wr[2] = data;
	crc_val = CRC8(crc_wr, 3, CRC_KEY);

	write_data[0] = reg;
	write_data[1] = data;
	write_data[2] = crc_val;

	if (i2c::send(I2C_INTERFACE, I2C_ADDRESS, 3, write_data) == 0) error_bit = true;
}

uint8_t BQ76930::read_register(TI_Register_ID reg)
{
	if (i2c::command_read(I2C_INTERFACE, I2C_ADDRESS, reg, 2, read_data) == 0) error_bit = true;

	return read_data[0];
}

uint16_t BQ76930::read_voltage(const TI_Register_ID reg_lo, const TI_Register_ID reg_hi)
{
	uint16_t adc_data = 0;
	uint16_t voltage = 0;

	/* Retrieve cell voltage ADC readout */
	if ((i2c::command_read(I2C_INTERFACE, I2C_ADDRESS, reg_hi, 2, voltage_buffer_high) == 0) ||
			(i2c::command_read(I2C_INTERFACE, I2C_ADDRESS, reg_lo, 2, voltage_buffer_low) == 0)) error_bit = true;

	//Preparing voltage data from register (ADC readings)
	adc_data = ((voltage_buffer_high[0] & 0x3F) << 8) | (voltage_buffer_low[0] & 0xFF);

	//Adapt voltage data to ADC GAIN and ADC OFFSET
	voltage = (uint16_t)(((adc_data * ADC_GAIN) / 1000) + ADC_OFFSET);

	return voltage;
}

/*
 * This function basically uses the BQ76930::read_voltage function, where reg_hi = bat_hi and
 * reg_lo = bat_lo registers on the monitor.
 * The only difference is how the value is displayed: in the function above, result is obtained after updating
 * the 14bits ADC reading with the respective ADC_GAIN and ADC_OFFSET.
 * In the function below, the data is already displayed as a 16bits value and it needs to be converted using
 * the equation displayed in the header file.
 */
void BQ76930::read_battery_voltage(void)
{
	uint16_t adc_data = 0;

	/* Retrieve battery voltage ADC readout */
	if ((i2c::command_read(I2C_INTERFACE, I2C_ADDRESS, bat_hi, 2, voltage_buffer_high) == 0) ||
			(i2c::command_read(I2C_INTERFACE, I2C_ADDRESS, bat_lo, 2, voltage_buffer_low) == 0)) error_bit = true;

	//Preparing voltage data from register (16bits)
	adc_data = ((voltage_buffer_high[0] & 0xFF) << 8) | (voltage_buffer_low[0] & 0xFF);

	//Adapt battery voltage
	battery_voltage = (uint16_t)(((adc_data * ADC_GAIN * 4) / 1000) + (ADC_OFFSET * bms_config::n_cells));
}

void BQ76930::enable_balancing(int cell)
{
	uint8_t crc_val = 0;
	uint8_t balancing_register = cell < 4 ? cellbal1 : cellbal2;
	int balancing_cell;

	/* Calculate the correct balancing cell value that's going to be
	 * written in the cellbal register */
	switch(cell)
	{
	case 6:
	case 3:
		balancing_cell = 4;
		break;

	case 5:
		balancing_cell = 1;
		break;

	case 4:
		balancing_cell = 0;
		break;

	case 2:
	case 1:
	case 0:
	default:
		balancing_cell = cell;
		break;
	}

	/* Calculate CRC on the specific register and data to be sent */
	crc_wr[0] = I2C_ADDRESS << 1;
	crc_wr[1] = balancing_register;
	crc_wr[2] |= 1 << balancing_cell;
	crc_val = CRC8(crc_wr, 3, CRC_KEY);

	write_data[0] = balancing_register;
	write_data[1] |= 1 << cell;
	write_data[2] = crc_val;

	if (i2c::send(I2C_INTERFACE, I2C_ADDRESS, 3, write_data) == 0) error_bit = true;
}

void BQ76930::disable_balancing(void)
{
	/* By writing a 0 in every bit in these two registers, it disables balacing
	 * for all cells in the BMS */
	write_register(cellbal1, BAL_OFF);
	write_register(cellbal2, BAL_OFF);

	/* Signals that balancing procedure has finished */
	gpio::clear(pin::UT_ERROR);
	gpio::clear(pin::UV_ERROR);

	balancing_enabled = false;
}

void BQ76930::check_balancing(bool activate)
{
	if (activate)	/*Checks and enables balancing */
	{
		int num_balancing_cells = 0;

		for (int cell=0; cell<bms_config::n_cells; ++cell)
		{
			/* Balancing enabling condition */
			if (voltage_readings[cell] > min_voltage)
			{
				/* Checks for adjacency condition, and that the current cell is not already balancing */
				if ((num_balancing_cells < bms_config::max_balancing_cells) && check_adjacency(cell) && !is_balancing(cell))
				{
					balancing_cells[num_balancing_cells] = cell;
					num_balancing_cells++;
				}
			}
		}

		for (int i=0; i<bms_config::max_balancing_cells; ++i)
		{
			/* Checks that all balancing cells are in correct range
			 * and that no random cells are balanced */
			if (balancing_cells[i] < 7)
			{
				enable_balancing(balancing_cells[i]);
			}
		}
	}
	else /* Checks and disables balancing */
	{
		if (balancing_enabled)
		{
			if (balancing_stop_condition())
			{
				disable_balancing();
			}
		}
	}
}

void BQ76930::read_cellvoltages(void)
{
	/*
	 * As said before, this function only calls the function read_voltage
	 * for all the cells connected to the AFE
	 */
	voltage_readings[0] = read_voltage(vc1_lo, vc1_hi);
	voltage_readings[1] = read_voltage(vc2_lo, vc2_hi);
	voltage_readings[2] = read_voltage(vc3_lo, vc3_hi);
	voltage_readings[3] = read_voltage(vc5_lo, vc5_hi);
	voltage_readings[4] = read_voltage(vc6_lo, vc6_hi);
	voltage_readings[5] = read_voltage(vc7_lo, vc7_hi);
	voltage_readings[6] = read_voltage(vc10_lo, vc10_hi);

	/* Calculate minimum, maximum and average voltage for the cells pack */
	min();
	max();
	avg();
}

void BQ76930::read_stateofcharge(void)
{
	int16_t twos_complement = 0;

	/* Retrieve register value */
	twos_complement = ((read_register(cc_hi) & 0xFF) << 8) | (read_register(cc_lo) & 0xFF);

	/* Calculate state of charge */
	state_of_charge = int16_t((twos_complement * CC_LSB) / 1000);
}
