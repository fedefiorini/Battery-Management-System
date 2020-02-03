/*
 * timing.cpp
 *
 *  Created on: Feb 12, 2019
 *      Author: @fedefiorini
 */
#include "timing.hpp"

#include "SEGGER_RTT.h"

volatile bool waiting = false;

namespace timing
{
	/* Used to uniform the LPC_TIMER32_0 with actual microseconds */
	uint32_t timer_frequency = 0;

	void init()
	{
		Chip_TIMER_Init(LPC_TIMER32_0);
		timer_frequency = Chip_Clock_GetSystemClockRate();

		/* Enable timed interrupt */
		NVIC_ClearPendingIRQ(TIMER_32_0_IRQn);
		NVIC_EnableIRQ(TIMER_32_0_IRQn);
	}

	void wait(uint32_t micros)
	{
		/* Checks if timer is already enabled */
		if (LPC_TIMER32_0->TCR & 1)
		{
			const uint32_t ticks_in_micros = timer_frequency / 1000000;
			uint32_t current_count = Chip_TIMER_ReadCount(LPC_TIMER32_0);

			Chip_TIMER_SetMatch(LPC_TIMER32_0, 1, (micros * ticks_in_micros) + current_count);
			Chip_TIMER_MatchEnableInt(LPC_TIMER32_0, 1);

			waiting = true;

			/* Wait until interrupt is raised (timer is done) */
			while(waiting)
			{
				__WFI();
			}

			Chip_TIMER_MatchDisableInt(LPC_TIMER32_0, 1);
		}
		else
		{
			/* Reset the timer and settings */
			Chip_TIMER_Reset(LPC_TIMER32_0);

			/* Match timer to match register 1 */
			Chip_TIMER_MatchEnableInt(LPC_TIMER32_0, 1);

			const uint32_t ticks_in_micros = timer_frequency / 1000000;
			Chip_TIMER_SetMatch(LPC_TIMER32_0, 1, (micros * ticks_in_micros));

			/* Start timer */
			Chip_TIMER_Enable(LPC_TIMER32_0);

			waiting = true;

			/* Wait until interrupt is raised (timer is done) */
			while(waiting)
			{
				__WFI();
			}

			Chip_TIMER_Disable(LPC_TIMER32_0);
		}
	}
}

/*
 * Timer interrupt handler (called as soon as the micros wait period expires)
 */
extern "C" __attribute__((interrupt)) void TIMER32_0_IRQHandler ( void )
{
	if (Chip_TIMER_MatchPending(LPC_TIMER32_0, 1))
	{
		Chip_TIMER_ClearMatch(LPC_TIMER32_0, 1);
		waiting = false;
	}
}
