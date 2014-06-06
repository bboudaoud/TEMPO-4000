All hardware design for the TEMPO 4 platform was done in PADS 9.3.1 from Mentor Graphics

If this tool is not available, CAM output files are provided to produce the board without the need for PCB editing. The organization of the hardware section of this repository is as follows

-----------------------------------------------------------------------------
Add ons
-----------------------------------------------------------------------------
Contains a 2-layer template file for creating low-cost future add-ons (this could be duplicated in another tool by using two 100mil headers spaced 750mils apart. In addition two completed top-boards are provided. One for BLE operation using the BR-LE4.0-S2A module:

http://www.blueradios.com/hardware_LE4.0-S2.htm

and another for ECG collection using an ADS series dual-channel, programmable bio-signal AFE from Texas Instruments:

http://www.ti.com/product/ads1192/description

These designs are intended for demonstration of effective design strategies for the TEMPO platform. In the future additional add on designers may choose to work from either the template file or these designs themselves depending on the nature of their expansions.

-----------------------------------------------------------------------------
Core Platform
-----------------------------------------------------------------------------
Contains designs, layout, and part information relevant to the TEMPO 4 core platform. Datasheet for a number of critical components, as well as pre-routed, pre-poured, and final layout files are provided.

In addition to CAM files a number of files valuable for production including a bill of material (BoM) and "stuffing" or population instructions document are also provided in this section.

-----------------------------------------------------------------------------
Libs
-----------------------------------------------------------------------------
For those designers interested in using PADS for future development on top of the TEMPO 4 platform the set of libraries created to produce this hardware are also included for convenience. It is worth noting that these libraries contain a number of useful devices that may not have been used in the final TEMPO layout

-----------------------------------------------------------------------------
Areas for Improvement
-----------------------------------------------------------------------------
Since no one is perfect there are of course areas of the TEMPO 4 platform that could stand some improvement provided a collaborator with access to PADS 9.3.1 is willing to take some time to create a few new footprints and re-route the board these are enumerated below.

1. Replace FT232 with FT230x, consider miniUSB, and create a new USB detection circuit. The smaller footprint FT230x device may help to reduce some of the area consumed by the USB transceiver in the final design. Since the FT230x is a completely compatible part (just removes some unused CBUS pins) this could be of value. In addition a lower height miniUSB 2.0 connector, popularized by its use in Android phones could be swapped for the microUSB current being used in the platform. Last but not least, the method of USB connection detection used in the current TEMPO 4 platform is anything but reliable. While charge detection seems to work fine for this purpose for now, it is recommended that rather than using the 3.3V regulated output of the bus-powered FT232 to detect USB connection, a special purpose voltage divider, tied directly to the USB 5V, is used for this detection in the future. This is useful as it should provide an additional level of isolation between the FT232 and other circuitry, as well as providing an easier to maintain interface for monitoring for USB connections.

2. Attempt use of MPU9150. The pin-compatible MPU9150 was not used in this work as it was not available at design time and the debate of whether Invensense would switch to a 3x3mm (as opposed to the 4x4mm) package used in TEMPO 4 proved too great a risk. For future work, since these devices incorporate on-chip power gating, it is recommended the fuller-featured 9150 is populated (providing a magnetometer option) and these addition 3 axes power gated when only 6 DoF motion capture is desired.

3. Move MAX1555 behind reverse voltage protection. Though the reverse voltage protection circuit used for keeping core circuitry operational through battery polarity swaps is functional, it does not protect the MAX1555 LiPo charger IC that is used in this work. Since the exact delivered current, and effect of this chip on system voltage and current flow was not known at design time it was moved outside of this protection circuit. Unfortunately this chip is not reverse-polarity protected, and for that reason it can still be damaged by misinstallation of the battery. In the future it would be advantageous to consider a fuller-featured reverse polarity solution (possibly a shottky-based bridge rectifier) that would allow for full-system reverse voltage protection.

4. Find a pin-compatible MSP430 with full-speed I2C operation. Due to an un-noticed erratum in TI's documentation the MSP430F5342 was used in this work, despite it's hardened I2C module not having timing closure above 50kHz. It was also noticed that even close to this speed the module tends to hang and stall during operation. For this purpose, I2C rates below 10kHz are recommended for the current platform. Due to the pull-ups on the bus this means extra power dissipated for no reason. Hopefully TI fixes this issue in a future tape-out of the IC, if not a similar 48 pin device without these timing issues is recommended for use.

5. More thorough MMC decoupling strategies for coin-cell operation. Since the TEMPO 4 platform is not currently targeting coin-cell battery topologies, MMC decoupling was assumed to be the standard, recommended values in ceramic packages. However, if in the future coin-cells are to be used, it is recommended that an electrolytic and tantalum cap are added to the schematic and layout. This extra energy storage allows for larger bursty current draws during reads, writes, and erasures and correspoding reduced demand for large current draws as seen from the far side of the regulator.
