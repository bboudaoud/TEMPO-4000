The firmware portion of the TEMPO 4 platform is provided in this sub-section of the repository. It is worth noting that though this directory contains nearly all code produced for the TEMPO 4 platform not all of it will be required for every project. In an effort to better delineate these divisions the firmware is grouped into 3 core classes described below. In addition, HTML-based documentation is provided using inline Doxygen-parsed comments.

-----------------------------------------------------------------------------
C Code
-----------------------------------------------------------------------------
The C libraries produced for the TEMPO 4 firmware are split into three primary sections, these are described below. It is worth noting that though these files are seperated into different folders for organization purposes they may have any amount of interdepence across these folders (particularly when dealing with driver and system-level code).

1. Base Functionality
This section includes the set of code which can be ported to serve only the widest possible set of applications on the MSP430. Support functionality includes:
	- System clock management via the on-chip FLL
	- Communications over UART, SPI, and I2C using the USCI module
	- HAL for the TEMPO 4 platform
	- Info flash management
	- Interrupt callback registration for critical events
	- Real-time clock setter and getter routines
	- Timing management for a number of on-chip resources
	- Useful utilities for simplified device operation

2. Drivers
This section includes only code designed to support specific chips or chip-sets affiliated with the TEMPO 4 platform. Generally speaking, nearly all of these libraries rely on some portion of the base-functionality code base. Files include:
	- MMC communication libraries
	- Flash management and file system
	- FTDI UART to USB bridge control code
	- MPU6050 device driver

3. System-level
This section includes only the highest-level code produced for the TEMPO 4 platform and is in no way intended to be the end-all-be-all in firmware control schemes. It is acknowledged that much of this code was not tested as thoroughly as individual drivers and low-level platform support. Files include:
	- Command interface and data parsing
	- Main.c and system level coordination for operation
	- An additional copy of the HAL

Last, but not lease this section also includes a doxygen input file and graphic for creation of HTML-based documentation from the inline Doxygen comments used throughout all of the previously mentioned files. This is supplied to enable future developers to reproduce this documentation easily based upon any additions or modifications they produce.

-----------------------------------------------------------------------------
Doxygen
-----------------------------------------------------------------------------
Contains the HTML output directory from Doxygen documentation generation based on the previously mentioned input file and comments. This is intended to provide a quick reference to developers interested in working atop the existing firmware in an API-style context.

It is worth noting that no direct link is provided at the top-level. Instead the developer can simply navigate to any .html file in the affiliated html directory and open it using a common browser in order to access the parsed documentation. From here, the in-window navigation links can be used to move throughout the work.

-----------------------------------------------------------------------------
Areas for Improvement
-----------------------------------------------------------------------------
Though the TEMPO firmware was tested rigorously in a variety of contexts a number of improvements can still stand to be made to all three areas of operational code. Some of these improvements are summarized below.

1. Addition of more base-level peripheral management and system control. This work only sought to develop atop the necessary peripherals for use in the TEMPO 4 operating model, so a number of potentially useful hardware modules within the MSP are still not supported by this code-based. For the purpose of future developers, who might be interested in precise control of timer routines, or computationally complex arithmetic that could benefit from use of a hardware multiplier it is recommended new libraries be produced, possibly including a ported math.c/h to support more complex on-node signal processing efforts in the limited resource context.

2. Development of additional support for interrupt callback registration and masking via interrupts.c. This is a good consideration for those looking to eliminate the need for developer-defined ISRs. By masking all interrupts for the user and simply providing callbacks to registered pointers this layer would slightly increase time-to-callback as seen by the developer, but may dramatically increase reliability of code operation as a result.

3. Improvement of file system. Though the mmc.c/h and Flash.c/h files produced as part of this work are rigorously tested and not recommended for major revision in the future the file system was produced somewhat hastily and without full consideration of all possible use cases. Future investigation of the overheads affiliated with running TI's FAT16 code base or a system similar to that of TEMPO 3.2 may be worth serious consideration.

4. Extension of the MPU driver. Currently the MPU driver support only MPU6xxx devices and their affiliated register maps. Provided it does not require major restructuring of core code, it is recommended that macro-based conditional compilation directives are used to allow for extension of this library to the MPU9xxx series platform as well. This should primarily consist of creation/extension of the register map (found in MPU.h) and some addition/extension of structures designed to control things such as axis data collection and sensor power gating.

5. Command structure review. Though the command structure used in this work has been tested to perform admirably at 1Mbaud and above, it is recommended that the method by which the incoming byte stream is parsed (found in system.c not command.c) is revisited. Though this code should work fine, it is prone to overrun and framing errors that may be produced by damaged or partially severed USB or UART connections. In the future I would recommend integrating command parsing into command.h using an interrupt callback driven approach that simply calls the runCommand() function once an acceptable number of bytes has been queued into the UART FIFO in the comm library.

6. Top-level operational models. The top-level control scheme used in the TEMPO 4 platform is by no means optimial for even the 6 DoF IMU collection case, let alone the full spectrum of operational models that the full set of end-users may adopt. For this reason experimentation with a variety of operating models is suggested. Currently a wide array of successful operating systems have been implemented on MSP430 platforms including FreeRTOS, Contiki, TinyOS, and TI's SYS/BIOS (http://processors.wiki.ti.com/index.php/MSP430_Real_Time_Operating_Systems_Overview). For the purpose of determining which of these operating systems impose the fewest constraints on the developer, while allowing for maximum efficiency and flexibility it is suggested that a variety of these options be implemented and evaluated on the platform to help better determine the fundamental trade-offs of selection of an embedded RTOS.
