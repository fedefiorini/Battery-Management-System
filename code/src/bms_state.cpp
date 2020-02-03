/*
 * bms_state.cpp
 *
 *  Created on: Dec 18, 2018
 *      Author: @fedefiorini
 */

#include "bms_state.hpp"
#include "bms_gpio.hpp"
#include "bms_uart.hpp"
#include "pins.hpp"

#include "SEGGER_RTT.h"

namespace state
{
	/* OV counter used to disable (open) DSG FET in case of persitent fault condition */
	int OV_counter = 0;

	/* Pre-defined valid state transitions */
	state_transition valid_state_transitions[10] =
	{
			{ SETUP 		, READY			},
			{ READY 		, CHARGE 		},
			{ CHARGE 		, READY 		},
			{ READY 		, BALANCING 	},
			{ BALANCING 	, READY 		},
			{ READY 		, SLEEP			},
			{ SLEEP 		, SETUP 		},
			{ CHARGE		, CHARGE_AND_BAL},
			{ CHARGE_AND_BAL, CHARGE 		}
	};

	void status_encoder()
	{
		uint8_t status = 0;

		gpio::clear(pin::OK_LED);
		gpio::clear(pin::ERROR_LED);
		gpio::clear(pin::OV_ERROR);
		gpio::clear(pin::OT_ERROR);
		gpio::clear(pin::OC_ERROR);
		if (!monitor.balancing_enabled)
		{
			gpio::clear(pin::UV_ERROR);
			gpio::clear(pin::UT_ERROR);
		}

		/* Verify that the I2C communication subsystem works */
		if (monitor.error_bit)
		{
			set_state(I2C_FAIL);

			gpio::set(pin::OK_LED);
			gpio::set(pin::ERROR_LED);
			gpio::set(pin::OV_ERROR);

			/* Doesn't make sense to execute rest of the loop */
			return;
		}

		status = monitor.read_register(sys_stat);
		switch(status & 0x7F)	/* Removes "CC_READY" option from the status reading */
		{
		case 0:		//OK
			gpio::set(pin::OK_LED);
			/* Enable DSG if disabled before (but only if state is not CHARGING and there's no error) */
			if (bms_state == READY)
			{
				monitor.write_register(sys_ctrl2, monitor.FET_ON);
			}
			/* Sanity assignment: if AFE is OK (and there's no OT/UT fault) set the state back to normal.
			 * This avoids transient errors to be carried over */
			if ((bms_state != OVERTEMPERATURE || bms_state != UNDERTEMPERATURE) && !in_charge && !monitor.balancing_enabled)
			{
				set_state(READY);
			}
			break;

		case 1:		//OCD
			set_state(OVERCURRENT);
			gpio::set(pin::OC_ERROR);
			break;

		case 2:		//SCD
			set_state(SHORTCIRCUIT);
			gpio::set(pin::OC_ERROR);
			break;

		case 4:		//OV
			gpio::set(pin::OV_ERROR);
			if (OV_counter < bms_config::max_OV_count)
			{
				OV_counter++;
			}
			else
			{
				/* Open DSG FET (not automatically done by the AFE if OV fault */
				monitor.write_register(sys_ctrl2, monitor.FET_DISABLE);
				set_state(OVERVOLTAGE);
			}
			break;

		case 6:		//OV + SCD
			gpio::set(pin::OV_ERROR);
			gpio::set(pin::OC_ERROR);
			/* In this case, SCD has already triggered the AFE to open DSG FET */
			set_state(OVERCURRENT);
			break;

		case 8:		//UV
			gpio::set(pin::UV_ERROR);
			set_state(UNDERVOLTAGE);
			break;

		case 12:	//UV + OV
			gpio::set(pin::OV_ERROR);
			gpio::set(pin::UV_ERROR);
			/* Usually transient, but can cause DSG_FET to open */
			set_state(UNDERVOLTAGE);
			/* If DSG FET is opened, then set state to UV (that's the error that
			 * caused it to open */
			break;

		case 16:	//OVRD_ALERT
			gpio::set(pin::UT_ERROR);
			gpio::set(pin::ERROR_LED);
			/* This status means that the ALERT pin has been overridden externally,
			 * but this is usually due to the pin not being initialized correctly */
			break;

		case 20:	//OVRD_ALERT + OV
			gpio::set(pin::UT_ERROR);
			gpio::set(pin::OV_ERROR);
			break;

		case 32:	//AFE_FAULT
			gpio::set(pin::ERROR_LED);
			/* No need to open the DSG FET manually */
			set_state(AFE_FAULT);
			break;

		default:
			/* If transient, it will disappear at next reading (don't take countermeasures) */
			break;
		}

		/* Clear system status */
		monitor.write_register(sys_stat, status);
	}

	void enter_sleep_state()
	{
		/* Clears all LEDs to avoid power consumption, and to signal
		 * that Deep Sleep mode is initialized */
		gpio::clear(pin::OK_LED);
		gpio::clear(pin::ERROR_LED);
		gpio::clear(pin::OV_ERROR);
		gpio::clear(pin::UV_ERROR);
		gpio::clear(pin::OC_ERROR);
		gpio::clear(pin::OT_ERROR);
		gpio::clear(pin::UT_ERROR);

		/* Sets current BMS state to SLEEP (used for monitoring only) */
		set_state(SLEEP);

		/* Set Rising Edge on all pins that have start logic enabled */
		LPC_SYSCTL->STARTAPRP0 = 0x00000344;
		/* Resets logic state of start logic input pins */
		LPC_SYSCTL->STARTRSRP0CLR = 0xFFFFFFFF;
		/* Enables the start logic input pins */
		LPC_SYSCTL->STARTERP0 = 0x00000344;
		/* Status register of the start logic input pins */
		LPC_SYSCTL->STARTSRP0 = 0x00000344;

		/* Setup registers (see user manual for further reference) */

		/* Power Control Register: writing a 0 in DPDEN bit will enable Deep Sleep Mode */
		LPC_PMU->PCON |= (0<<1);
		/* Select the Power-up configuration (after the chip wakes up */
		LPC_SYSCTL->PDWAKECFG = LPC_SYSCTL->PDRUNCFG;
		/* Power-down Configuration Register: enables IRC oscillator to be used later as clock source when in deep sleep mode */
		LPC_SYSCTL->PDRUNCFG |= (0<<0) | (0<<1);
		/* Peripheral Clock Register: disable all analog peripherals for deep sleep (limits power consumption) */
		LPC_SYSCTL->SYSAHBCLKCTRL &= ~((1<<5) | (1<<6) | (1<<7) | (1<<8) | (1<<9) | (1<<10)
				| (1<<11) | (1<<12) | (1<<13) | (1<<15) | (1<<16) | (1<<17) | (1<<18));
		/* Select the Deep Sleep clock source. 0x0 corresponds to the IRC oscillator */
		LPC_SYSCTL->MAINCLKSEL = 0x0;
		/* Enable the new selected clock (and wait until it's enabled) */
		LPC_SYSCTL->MAINCLKUEN = 0x0;
		LPC_SYSCTL->MAINCLKUEN = 0x1;
		while (!(LPC_SYSCTL->MAINCLKUEN & 0x01));

		/* Select the correct Deep Sleep power configuration.
		 * Only certain values can be written into this register.
		 * 0x000018FF	=	WDT off, BOD off */
		LPC_SYSCTL->PDSLEEPCFG = 0x000018FF;

		/* Enable wakeup pin interrupts and before make sure to clear all pending interrupts
		 * (so there's no conflict going on */
		//NVIC_ClearPendingIRQ(PIO0_2_IRQn);
		NVIC_ClearPendingIRQ(PIO0_6_IRQn);
		NVIC_ClearPendingIRQ(PIO0_8_IRQn);
		NVIC_ClearPendingIRQ(PIO0_9_IRQn);

		//NVIC_EnableIRQ(PIO0_2_IRQn);
		NVIC_EnableIRQ(PIO0_6_IRQn);
		NVIC_EnableIRQ(PIO0_8_IRQn);
		NVIC_EnableIRQ(PIO0_9_IRQn);

		/* Writes on PMU register that the chip is
		* going to enter DEEP SLEEP mode */
		SCB->SCR |= (1<<2);

		//Enter deep-sleep mode (WaitForInterrupt)
		__WFI();
	}
}

/*
 * Wakeup interrupt handler
 *
 * It has to be defined by the user and specifies the steps required
 * after the chip wakes up (either from SLEEP or DEEP SLEEP mode)
 *
 * It's required to reset GPIO interrupts in the handler
 */
extern "C" __attribute__ ((interrupt)) void WAKEUP_IRQHandler(void)
{
	/* Reprogram the clock source. 0x3 corresponds to the System PLL */
	LPC_SYSCTL->MAINCLKSEL = 0x3;
	/* Enable the new selected clock (and wait until it's enabled) */
	LPC_SYSCTL->MAINCLKUEN = 0x0;
	LPC_SYSCTL->MAINCLKUEN = 0x1;
	while (!(LPC_SYSCTL->MAINCLKUEN & 0x01));

	/* Resets the interrupts on the wakeup pins */
	//Chip_SYSCTL_ResetStartPin(IOCON_PIO0_2);
	Chip_SYSCTL_ResetStartPin(IOCON_PIO0_6);
	Chip_SYSCTL_ResetStartPin(IOCON_PIO0_8);
	Chip_SYSCTL_ResetStartPin(IOCON_PIO0_9);

	/* __NOP() IS PLACED HERE AS DEFINED IN LPC11C24 PMU EXAMPLE */
	__NOP();

	/* Disables the wakeup interrupts (to avoid INTR loop) */
	//NVIC_DisableIRQ(PIO0_2_IRQn);
	NVIC_DisableIRQ(PIO0_6_IRQn);
	NVIC_DisableIRQ(PIO0_8_IRQn);
	NVIC_DisableIRQ(PIO0_9_IRQn);

	/* Set WAKEUP state as SETUP */
	state::set_state(SETUP);

	/* Set all LEDs to signal the WAKEUP event */
	gpio::set(pin::OK_LED);
	gpio::set(pin::ERROR_LED);
	gpio::set(pin::OV_ERROR);
	gpio::set(pin::UV_ERROR);
	gpio::set(pin::OC_ERROR);
	gpio::set(pin::OT_ERROR);
	gpio::set(pin::UT_ERROR);

	/* Initialize back all the peripherals */
	SystemCoreClockUpdate();
	pin::initialize_peripheral_pins();
	adc::init_adc();
	can::init_can();
	timing::init();
	i2c::init(I2C_INTERFACE, I2C_SPEED);
	uart::init();

	/* Initialize back all the global variables */
	lvb_sense = true;
	check = true;
	deep_sleep_timer = 0;
	can_update_counter = 0;
	balancing_enabling_count = 0;
	balancing_enabler = 0;
	charging_enabling_count = 0;
	monitor.balancing_enabled = false;
}

