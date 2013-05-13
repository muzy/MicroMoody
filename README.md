# MicroMoody

RGB LED driven by an ATTiny85. Supports various modes of operation and can be
remote-controlled via I2C.

## remote control (I2C)

This is enabled by the `-DI2CEN` compile flag, which is set by default in the
Makefile.

After sending the 7bit address and the r/w bit (set to write),
the bus master must send the following 7 bytes to the MicroMoody:

* opmode
* fade/blink speed
* red value
* green value
* blue value
* payload address (high byte)
* payload address (low byte)

The default I2C address is 0x11 (the first byte with write flag set is 0x23).
The default payload address is 0x0001. The idea behind this distinction is
to use the I2C address to talk to any micromoody, and the payload address to
select the specific micromoody to address. This way, you can have various
other I2C devices and more than 127 MicroMoodies on the same bus
(just in case).

In the future, a broadcast payload address may be implemented.

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
* 201: Set address to color bytes. payload high = red, payload low = green,
       i2c = blue. This will also set the operation mode to random fading

Mode and speed are saved and recalled after a power cycle.

## temperature sensor

Enabled by `-DTEMPERATURE`. Disables I2C and fading. Not finished yet.

Uses the LED to display the current ambient temperature, blue means cold,
red means warm.

## remote control via USB -> I2C bridge

[Example firmware and
software](http://lib.finalrewind.org/a/PowerSwitch.2012-12-08.i2c.tar.bz2)
based on [VUSB](http://vusb.wikidot.com/hardware). Refer to their website
for more information.

## recognizing the firmware state

At the time of this writing, a newly flashed MicroMoody with default address
will run the RGB blink mode (starting with yellow), while a newly flashed
MicroMoody with only the addresses changed and no mode set will run in
random fading mode.
