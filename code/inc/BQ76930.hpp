/*
 * BQ76930.hpp
 *
 *  Created on: Jan 17, 2019
 *      Author: @fedefiorini
 */

/*
 * This header contains the declaration of all the functions needed to
 * use the Texas Instrument bq76930 Analog Front End (BMS MONITOR).
 *
 */

#ifndef BQ76930_HPP_
#define BQ76930_HPP_

#include "chip.h"
#include "bms_i2c.hpp"
#include "timing.hpp"
#include "pins.hpp"
#include "configuration.hpp"

#include <stdlib.h>

#include "SEGGER_RTT.h"

/* I2C Address of the AFE */
#define I2C_ADDRESS		0x08
/* Key of the polynomial CRC error detection */
#define CRC_KEY			7

/*
 * bq76930 Registers Map
 *
 * Each register has its own feature and the complete description can be
 * found at http://www.ti.com/lit/ds/symlink/bq76930.pdf
 *
 * Consult section 7.5 "Register Maps" and 4 "Device Comparison Table" for
 * further information
 *
 * Each register is 8 bits and the value displayed is their address in hex.
 */
enum TI_Register_ID : uint8_t
{
	/*
	 * System Status register.
	 */
	sys_stat 	= 0x00,

	/*
	 * Cell Balancing registers.
	 * Writing a "1" on each bit allows balancing
	 * of the correspondent battery cell.
	 */
	cellbal1 	= 0x01,
	cellbal2 	= 0x02,

	/*
	 * System Configuration registers.
	 */
	sys_ctrl1	= 0x04,
	sys_ctrl2 	= 0x05,

	/*
	 * OCD and SCD Thresholds registers
	 */
	protect1 	= 0x06,
	protect2 	= 0x07,
	protect3 	= 0x08,

	/*
	 * OV and UV Thresholds registers
	 */
	ov_trip 	= 0x09,
	uv_trip 	= 0x0A,

	/*
	 * Coulomb Counter Configure.
	 * This register has to be initialized to 0x19
	 */
	cc_cfg 		= 0x0B,

	/*
	 * Cell ADC readings registers.
	 * To obtain a cell voltage, read out the two values
	 * stored in vcx_hi and vcx_lo and combine them.
	 * Then apply the formula
	 * V(cell) = GAIN x ADC(cell) + OFFSET
	 *
	 * Cells 4,8,9 are not connected by HW design
	 */
	vc1_hi 		= 0x0C,
	vc1_lo 		= 0x0D,
	vc2_hi 		= 0x0E,
	vc2_lo 		= 0x0F,
	vc3_hi 		= 0x10,
	vc3_lo 		= 0x11,
//	vc4_hi 		= 0x12,
//	vc4_lo 		= 0x13,
	vc5_hi 		= 0x14,
	vc5_lo 		= 0x15,
	vc6_hi 		= 0x16,
	vc6_lo	 	= 0x17,
	vc7_hi 		= 0x18,
	vc7_lo 		= 0x19,
//	vc8_hi 		= 0x1A,
//	vc8_lo 		= 0x1B,
//	vc9_hi 		= 0x1C,
//	vc9_lo 		= 0x1D,
	vc10_hi 	= 0x1E,
	vc10_lo 	= 0x1F,

	/*
	 * Battery ADC reading registers.
	 * To obtain the battery voltage, read out the two
	 * values stored in bat_hi and bat_lo and combine them.
	 * Then apply the formula
	 * V(bat) = 4 x GAIN x ADC(cell) + (#Cells x OFFSET)
	 */
	bat_hi 		= 0x2A,
	bat_lo 		= 0x2B,

//	ts1_hi 		= 0x2C,
//	ts1_lo 		= 0x2D,
//	ts2_hi 		= 0x2E,
//	ts2_lo 		= 0x2F,

	/*
	 * Coulomb Counter registers.
	 * To obtain the CC value, read out the two values
	 * and combine them to obtain the 16bits voltage.
	 * (LVB STATE OF CHARGE)
	 */
	cc_hi		= 0x32,
	cc_lo 		= 0x33,

	/*
	 * ADC Gain and Offset registers.
	 * Used to calculate V(cell) and V(bat) after ADC conversion.
	 */
	adc_gain1 	= 0x50,
	adc_offset 	= 0x51,
	adc_gain2 	= 0x59
};


class BQ76930
{
private:

	uint8_t read_data[2] = {0};								//In READ operations, data[0] = register_value and data[1] = CRC received from the I2C slave
	uint8_t write_data[3] = {0};							//In WRITE operations, data[0] = register_address, data[1] = value to write and data[2] is the calculated CRC
	uint8_t voltage_buffer_high[2] = {0};					//Buffer in which voltage readings (high register) and its respective CRC are stored
	uint8_t voltage_buffer_low[2] = {0};					//Buffer in which voltage readings (low register) and its respective CRC are stored

	uint8_t crc_wr[3] = {0};								//CRC_WR is used to calculate CRC8 at every write operation (and it's sent along the data)
	uint8_t crc_rd[2] = {0};								//CRC_RD is used to calculate CRC8 upon every read operation

	/*
	 * This function retrieves the minimum voltage in the voltage_readings buffer
	 */
	void min()
	{
		min_voltage = voltage_readings[0];
		for (int i=0; i<bms_config::n_cells; ++i)
		{
				if (voltage_readings[i] < min_voltage)
				{
					min_voltage = voltage_readings[i];
				}
		}
	}
	/*
	 * This function retrieves the maximum voltage in the voltage_readings buffer
	 */
	void max()
	{
		max_voltage = voltage_readings[0];
		for (int i=0; i<bms_config::n_cells; ++i)
		{
			if (voltage_readings[i] > max_voltage)
			{
				max_voltage = voltage_readings[i];
			}
		}
	}
	/*
	 * This function retrieves the average voltage in the voltage_readings buffer
	 */
	void avg()
	{
		avg_voltage = 0;
		for (int i=0; i<bms_config::n_cells; ++i)
		{
			avg_voltage += voltage_readings[i];
		}

		avg_voltage = uint16_t(avg_voltage / bms_config::n_cells);
	}

	/*
	 * This function checks whether one cell is present in the array of balancing cells
	 */
	bool is_balancing(int cell)
	{
		for (int i=0; i<bms_config::max_balancing_cells; ++i)
		{
			if (balancing_cells[i] == cell)
			{
				return true;
			}
		}
		return false;
	}

	/*
	 * This function checks for adjacency of cells when balancing, as -by datasheet-
	 * it's not possible to balance adjacent cells simultaneously (within same subset).
	 * In particular, considering that cells 4, 8 and 9 are shorted, it checks that
	 * cells 1-2, 2-3 and 6-7 don't need to be balanced at the same time
	 */
	bool check_adjacency(int cell)
	{
		if ((cell == 1 && is_balancing(0)) || (cell == 2 && is_balancing(1)) || (cell == 5 && is_balancing(4)))
		{
			return false;
		}

		return true;
	}

	/*
	 * This function allows to check whether the voltage readings are equal to the 100th
	 * of a Volt (i.e: 3340mV and 3342mV are equal in this sense), and it's used to
	 * determine whether balancing can be stopped or not.
	 */
	bool balancing_stop_condition()
	{
		int ok_cells = 0;

		for (int cell=0; cell<bms_config::n_cells; ++cell)
		{
			if (abs(int(voltage_readings[cell] - min_voltage)) < bms_config::balancing_stop)
			{
				ok_cells++;
			}
		}
		if (ok_cells == 7)
		{
			return true;
		}
		return false;
	}

public:

	/*
	 * This variables are passed throughout the entire program and they're used to store the
	 * cells voltages and the battery voltage.
	 */
	uint16_t voltage_readings[bms_config::n_cells] 		= {0};
	uint16_t battery_voltage 							= 0;
	int16_t state_of_charge 							= 0;

	/* This array is used when enabling cells balacing, to be sure that
	 * no adjacent cells are balanced at the same time
	 * They're initialized as 7 as it's not in the range of possible cells
	 * (check the if condition in check_balancing */
	int balancing_cells[bms_config::max_balancing_cells]= {7};

	/*
	 * ERROR BIT
	 * This bit is set to true every time there's a problem in the communication subsystem of the
	 * BQ76930 (meaning that I2C is not functioning as expected).
	 * During normal operation, this bit is checked and reports the error to the main state machine
	 */
	bool error_bit 										= false;

	/*
	 * BALANCING ENABLED
	 * This variable is set to TRUE whenever the LPC is told to start the balancing procedure.
	 * In order to stop it (therefore disabling balancing operations), just set this to FALSE
	 */
	bool balancing_enabled 								= false;

	/*
	 * Minimum cell voltage of the LVB (used to enable balancing of cells)-
	 *
	 * Minimum, Maximum and Average cell voltage are also sent out to the ECU/DASH for
	 * testing and monitoring purposes.
	 */
	uint16_t min_voltage								= 0;
	uint16_t max_voltage								= 0;
	uint16_t avg_voltage								= 0;
	/*
	 * ADC Gain and ADC Offset values.
	 * Those data are stored in registers ADC_GAIN1/2 (0x50, 0x59) and ADC_OFFSET (0x51)
	 *
	 * Since they don't change over time, we can read out those values at the beginning and then
	 * store them in those two values.
	 */
	const uint16_t ADC_GAIN		= 377;		//	377 µV/LSB
	const uint8_t ADC_OFFSET	= 48;		//	+48 mV
	/*
	 * State of Charge LSB value.
	 * It's used to retrieve the value of the current state of charge of the battery.
	 * Calculation:
	 * CC_READING = [16bit 2's complement] x LSB
	 *
	 * Since the value of the LSB is decimal, calculations are made using nV/LSB and
	 * then dividing the result by a factor of 1000 (ARM-Cortex M0 doesn't have FP unit).
	 */
	const uint16_t CC_LSB		= 8440;	//8.44 µV/LSB
	/*
	 * Overvoltage (OV) and Undervoltage (UV) thresholds
	 * Those values will be stored in OV_/UV_TRIP registers (0x09, 0x0A)
	 *
	 * Calculation:
	 * OV_/UV_TRIP = (OV_/UV_FULL - ADC_OFFSET) / ADC_GAIN
	 * From the 14bits value obtained, remove 2MSB and 4LSB and obtain the 8bit value to write
	 */
	const uint8_t OV_THRESH		= 0xB1;		//4.20V
	const uint8_t UV_THRESH		= 0xF0;		//3.05V
	/*
	 * OV and UV protection delay.
	 * Value 0x00 corresponds to 1sec delay for both Undervoltage and Overvoltage faults
	 */
	const uint8_t VOLT_DELAY	= 0x00;
	/*
	 * The following values have to be written on registers PROTECT1 (0x06) and PROTECT2 (0x07) and
	 * they serve as indication of both delay settings and threshold settings for Overcurrent (OC) in
	 * discharge and Short Circuit (SCD) in discharge.
	 *
	 * SCD_VAL contains also value RSNS = 1 (OCD and SCD are considered at upper input range)
	 *
	 * Values are calculated using RSNS sense resistor value of 2mΩ.
	 * SCD = 25A, delay 200µs
	 * OCD = 18A, delay 320ms
	 */
	const uint8_t OCD_VAL		= 0x5B;
	const uint8_t SCD_VAL		= 0x23;
	/*
	 * ADC Enable command.
	 * Enables voltage ADC readings and also enables OV protection.
	 */
	const uint8_t ADC_EN 		= 0x10;
	/*
	 * CC_CFG register default value.
	 * According to manual, this register (0x0B) has to be configured with 0x19 in bits [5..0]
	 * for optimal performance.
	 */
	const uint8_t CC_CFG		= 0x19;
	/*
	 * Charge and Discharge FET closing values, and FET clearing value.
	 * Write them according to the FET that needs to be closed in AFE's register sys_ctrl2.
	 *
	 * First 4 bits of the register have to be set to 0010 in order to enable state of charge (CC)
	 * "one shot" readings.
	 */
	const uint8_t FET_DISABLE	= 0x20;
	const uint8_t CHG_ON		= 0x21;
	const uint8_t DSG_ON		= 0x22;
	const uint8_t FET_ON		= 0x23;
	/*
	 * Disables balancing operations by simply clearing the cells-correspondent bits in cellbalx registers
	 */
	const uint8_t BAL_OFF		= 0x00;


	/*
	 * Initializes the I2C bus and configures the required thresholds into the bq76930 chip.
	 */
	void init(void);
	/*
	 * This function enables balancing of a single cell (or multiple ones) by writing a 1
	 * on one of the two CELLBAL registers (0x01 for cells 0..5, 0x02 for cells 6..10)
	 *
	 * \cell indicates the particular cell to balance
	 */
	void enable_balancing(int cell);
	/*
	 * This function disables balancing (writes a 0 on all bits of CELLBAL registers).
	 */
	void disable_balancing(void);
	/*
	 * This function checks whether a cell needs to be balanced or not.
	 * We can avoid to care about OV-UV protections, as on the datasheet it's stated that
	 * cells balancing doesn't affect nor trigger such protections.
	 *
	 * \param activate is used to check whether check_balancing is used to enable or
	 * to disable the balancing procedure. By default it activates it
	 */
	void check_balancing(bool activate = true);
	/*
	 * This function reads the voltage of one cell from the two registers responsible for it
	 * Example: if you want to read voltage of cell X, you need to read
	 * X_high and X_low, then combine the values accordingly to obtain the 14 bit value
	 * that corresponds to that particular cell's voltage.
	 *
	 * The value is displayed in mV
	 */
	uint16_t read_voltage(const TI_Register_ID reg_hi, const TI_Register_ID reg_lo);
	/*
	 * This function uses the read_voltage(reg_hi, reg_lo) function above and retrieves all the
	 * cell voltages in a single run.
	 * The function above is therefore used to read the intermediate results (thus the single
	 * cell readout), then the return value will be stored in the voltage_readings buffer.
	 *
	 * Each value is displayed in mV
	 */
	void read_cellvoltages(void);
	/*
	 * This function reads the voltage of the battery pack itself, retrieving
	 * the result from the two registers bat_hi and bat_lo in the bq76930.
	 *
	 * The result is in 16 bits.
	 * V(bat) = 4 x ADC(cell) x ADC_GAIN + (#Cells x ADC_OFFSET)
	 *
	 */
	void read_battery_voltage(void);
	/*
	 * This function reads the LVB state of charge, retrieving the result
	 * from the two registers cc_hi and cc_lo in the bq76930.
	 * The selected way is CC_ONESHOT, to prevent triggering interrupts
	 * on the ALERT pin that may cause unexpected behaviors.
	 */
	void read_stateofcharge(void);
	/*
	 * Read a single register value.
	 */
	uint8_t read_register(const TI_Register_ID reg);
	/*
	 * Write data on one of the 8 bits registers of the bq76930 monitor.
	 *
	 * In order to write a 16 bits value (for subsequent registers) this function
	 * has to be called twice with the according \reg values.
	 */
	void write_register(const TI_Register_ID reg, uint8_t data);
	/*
	 * CRC calculation.
	 * Cyclic Redundancy Check (CRC) is used by the bq76930 monitor during read/write
	 * operations to check for data correctness.
	 * The calculated CRC code has to be sent along with the data during each write operation
	 * and calculated after every read operation.
	 * In any case, if there's a mismatch a NACK will be sent (to Master in WR, to Slave in RD)
	 *
	 * The CRC polynomial is x8 + x2 + x + 1, with initial value 0.
	 *
	 * The code is derived from the example code that can be found at:
	 * www.ti.com/product/BQ76940/toolssoftware (under "Software" tab)
	 *
	 * Parameters:
	 * \input				Monitor's I2C address and data to be sent
	 * \length				Length of the input address_and_data
	 * \key					CRC key (defined above)
	 */
	uint8_t CRC8(uint8_t *input, uint8_t length, uint8_t key)
	{
		uint8_t crc = 0;

		while(length-- != 0)
		{
			for (uint8_t i=0x80; i!=0; i/=2)
			{
				if ((crc & 0x80) != 0)
				{
					crc *= 2;
					crc ^= key;
				}
				else
				{
					crc *= 2;
				}

				if ((*input & i) != 0)
				{
					crc ^= key;
				}
			}

			input++;
		}

		return crc;
	}
};

#endif /* BQ76930_HPP_ */
