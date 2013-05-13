# MicroMoody

RGB LED driven by an ATTiny85. Supports various modes of operation and can be
remote controlled with an I2C-like protocol. Real I2C support will follow soon.

## remote control (I2C)

This is enabled by the `-DI2CEN` compile flag, which is set by default in the
Makefile.

After sending the 7bit address and the r/w bit (which are not interpreted yet),
the bus master must send the following 7 bytes to the MicroMoody:

* opmode
* fade/blink speed
* red value
* green value
* blue value
* address (high byte)
* address (low byte)

the default address is 0x0001.

red, green and blue are the color value, 0 is off, 0xff is full brightness.
the speed is inverted, so 0 is fastest, 255 is slowest.

### opmode

Modes marked with `RGB` interpret the red/green/blue bytes, for the others
they're not important.

*   0: steady `RGB` light
*   1: rainbow colors, hard transitions
*   2: random colors, hard transitions
*   3: `RGB` color, blink on/off
*   4: steady light, fade to specified `RGB` color
*   5: rainbow colors, fading transitions
*   6: random colors, fading transitions
*   7: `RGB` color, fade on/off

Mode and speed are saved and recalled after a power cycle.

## temperature sensor

Enabled by `-DTEMPERATURE`. Disables I2C and fading. Not finished yet.

Uses the LED to display the current ambient temperature, blue means cold,
red means warm.
