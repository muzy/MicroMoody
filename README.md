# MicroMoody

RGB LED driven by an ATTiny85. Supports various modes of operation and can be
remote controlled with an I2C-like protocol. Real I2C support will follow soon.

## remote control (almost-I2C)

This is enabled by the `-DI2CEN` compile flag, which is set by default in the
Makefile.

Each command consists of 7 bytes, transfered with the most significant bit
first. I2C start/stop signals are not yet supported.

The bytes are:
* opmode
* fade/blink speed
* red value
* green value
* blue value
* address (high byte)
* address (low byte)

the default address is 0x0001. **NOTE**: I2C is somewhat broken right now.

red, green and blue are the color value, 0 is off, 0xff is full brightness.
the speed is inverted, so 0 is fastest, 255 is slowest.

### opmode

Modes marked with `RGB` interpret the red/green/blue bytes, for the others
they're not important.

*   0: steady `RGB` light
*  32: rainbow colors, hard transitions
*  64: random colors, hard transitions
*  96: `RGB` color, blink on/off
* 128: steady light, fade to specified `RGB` color
* 160: rainbow colors, fading transitions
* 192: random colors, fading transitions
* 224: `RGB` color, fade on/off

Mode and speed are saved and recalled after a power cycle.

## temperature sensor

Enabled by `-DTEMPERATURE`. Disables I2C and fading.

Uses the LED to display the current ambient temperature, blue means cold,
red means warm.
