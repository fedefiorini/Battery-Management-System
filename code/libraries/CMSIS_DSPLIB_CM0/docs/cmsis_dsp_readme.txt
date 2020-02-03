CMSIS DSP Library (Cortex-M0)
=============================
This project provides a pre-built version of ARM's CMSIS
DSP Library, built at optimisation level -O2.

The reason for providing a pre-built version is that the DSP
Library has a large code base which typically can take several
minutes to build (depending upon PC specification and OS). For
most users, the DSP library routines can be considered as a
"black box" which there is no need to rebuild.

However a project is supplied (CMSIS_DSPLIB_SRC) which
provides a number of out of the box build configurations. Thus
you can easily rebuild this project to provide a version of the 
DSP library built using your required options, to replace the
one provided out of the box.

Note that the project is also built with debug information
enabled. This allows you to step through the DSP library code
if required, but only if you import the CMSIS_DSPLIB_SRC
project into your workspace first.