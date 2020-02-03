# BMS

Known issue with the LPC11C24:

* Entering DEEP SLEEP mode, the board will be no longer accessible for debug purposes.

If something goes wrong and you can't flash code via MCUXpresso again (Error Em(19) - Communication with DAP fails, for example) just follow the steps below, and then you will be able to regain debug access.

* Connect via jumper cable GND and ISP (pin 0.1)
* Connect via jumper cable GND and RESET (pin 0.0) //procedure "Assert RESET with low-going pulse"
* Remove jumper cable between GND and RESET
* Remove jumper cable between GND and ISP
* Hit the green bug (Debug as LinkServer / LPC-Link Probe) to flash code

Keep this file updated with all the known setup-/electronic-related issues

## Deep-Sleep mode

In Deep-sleep mode, the system clock to the processor is disabled as in Sleep mode. All analog blocks are powered down, except for the BOD (Brown-Out Detection) circuit and the watchdog oscillator, which must be selected or deselected during Deep-sleep mode in the PDSLEEPCFG register.
See the user manual (UM10398) for more details. [section 3.9.3]
