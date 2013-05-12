# MicroMoody

RGB LED driven by an ATTiny85. Supports various modes of operation and can be
remote controlled with an I2C-like protocol. Real I2C support will follow soon.

## remote control (almost-I2C)

This is enabled by the `-DI2CEN` compile flag, which is set by default in the
Makefile.

Each command consists of 6 bytes, transfered with the most significant bit
first. I2C start/stop signals are not yet supported.

The bytes are:
* opmode
* red value
* green value
* blue value
* don't care
* don't care

the don't care bytes may be used for a 16bit device address in the future.

red, green and blue are the color value, 0 is off, 0xff is full brightness.

### modes of operation

The highest three `opmode` bits determine the mode of operation, the other five
the fade/blink speed (if applicable). Note that a speed of 0 causes the fastest
transitions, while 31 is slowest. Modes marked with `RGB` interpret the
red/green/blue bytes, for the others they're not important. Modes are:

*   0 ..  31: steady `RGB` light
*  32 ..  63: rainbow colors, hard transitions
*  64 ..  95: random colors, hard transitions
*  96 .. 127: `RGB` color, blink on/off
* 128 .. 159: steady light, fade to specified `RGB` color
* 160 .. 191: rainbow colors, fading transitions
* 192 .. 223: random colors, fading transitions
* 224 .. 255: `RGB` color, fade on/off

Modes are saved and recalled after a power cycle.

## temperature sensor

Enabled by `-DTEMPERATURE`. Disables I2C and fading.

Uses the LED to display the current ambient temperature, blue means cold,
red means warm.
