/*
 * timing.hpp
 *
 *  Created on: Feb 12, 2019
 *      Author: @fedefiorini
 */

/*
 * This header contains the defines the timing functionalities using the
 * LPC11Cxx library functions.
 * This allows to have delays functionalities, especially in those applications
 * where timing is crucial (i.e. processing time after I2C operations)
 * exactly like vTaskDelay() is used in FreeRTOS libraries.
 *
 * Original author: @Bernard
 */

#ifndef TIMING_HPP_
#define TIMING_HPP_

#include "chip.h"

namespace timing
{
	/*
	 * Initializes the 32bits timer
	 */
	void init();
	/*
	 * Puts the processor in idle state (__WFI), so it actually it's
	 * waiting for timer to finish without performing any other operation.
	 */
	void wait(uint32_t micros);
}



#endif /* TIMING_HPP_ */
