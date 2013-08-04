# MicroMoody

RGB LED driven by an ATTiny85. Largely blink(1)-compatible, except that it uses
I2C instead of USB.

## setup

See <https://wiki.chaosdorf.de/MicroMoody> and
<https://wiki.raumzeitlabor.de/wiki/MicroMoody> for soldering instructions.

For the firmware, run `make && make program`. You may need to edit the
Makefile to fit your programmer, it's written for an usbasp.

## remote control (I2C)

This is enabled by the `-DI2CEN` compile flag, which is set by default in the
Makefile. Hardware pull-ups on SCL and SDA are required (1.5 kOhm recommended)

After sending the 7bit address and the r/!w bit,
the bus master must send the following 7 bytes to the MicroMoody:

* command
* delay
* red value
* green value
* blue value
* payload address (high byte)
* payload address (low byte)

The default I2C address is `0x11` (17),
The default payload address is `0x0001`. The idea behind this distinction is
to use the I2C address to talk to any micromoody, and the payload address to
select the specific micromoody to address. This way, you can have various
other I2C devices and up to 65534 MicroMoodies on the same bus
(just in case you need to).

Commands with the broadcast payload address of `0xffff` are always accepted,
even when they contain the "set address" opmode.

red, green and blue are the color value, 0 is off, 0xff is full brightness.

There are several types of commands:

### command 0 .. 63 (set mode of operation)

The red, green and blue bytes are "don't care" unless a command is
marked `RGB`.

*   0: steady `RGB` light
*   1: rainbow colors, hard transitions
*   2: random colors, hard transitions
*   3: `RGB` color, blink on/off
*   4: steady light, fade to specified `RGB` color
*   5: rainbow colors, fading transitions
*   6: random colors, fading transitions
*   7: `RGB` color, fade on/off
*  63: Run animation. It will start in slot 1 and end in the slot last set
       (see also below)

modes 8 to 62 are reserved for further blink / fade modes.

### command 64 .. 127 (animation sequences)

*  64: Save `RGB` + delay in animation slot 1
* ...
* 127: Save `RGB` + delay in animation slot 64

For instance, to set a steady red pulse, you can transmit (in hex)
`40 10 ff 00 00 ff ff`, `41 10 50 00 00 ff ff`. This will make the animation
run in slot 1 -> slot 2 -> slot 1 -> ....

Note that this does not start the animation yet, use `3f XX XX XX XX ff ff`
for that. Also note that the upper bound of the current sequence is
determined by the last animation command. So if you send `40 ...`,
`41 ...`, `42 ...`, `40 ...`, the animation run will be
slot 1 -> slot 1 -> slot 1 -> .... (i.e. a steady color).

Refer to example/\*/rgbpulse for a more sophisticated example.

### command 128 .. 239

reserved

### command 240 .. 255 (EEPROM)

These commands access the EEPROM. Writing one byte takes about 3.4ms and is
implemented synchronously, so expect them to be slow. They may even cause I2C
bus timeouts (which do not indicate write failures).

* 240: Save current mode of operation (last command &lt; 64) to EEPROM to be
       recalled after a power cycle. NOTE: if the current opmode is 63
       (animation), this may take up to 1 second (~12ms for each used animation
       slot)
* 241: Set address to color bytes. payload high = red, payload low = green,
       i2c = blue. This will also set the operation mode to random fading.
       Note that for i2c, the least significant 7 bit are the address, while
       the most significant bit is ignored

commands 242 to 255 are reserved for further EEPROM commands.

## I2C master hardware

If you have an Arduino, see `examples/arduino/SerialToI2C` for a USB-Serial
to I2C interface.

Otherwise, see [vusb-i2c](https://github.com/derf/vusb-i2c) for a
general-purpose USB-I2C tool based on VUSB.

## recognizing the firmware state

At the time of this writing, a newly flashed MicroMoody with default address
will run the RGB blink mode (starting with yellow), while a newly flashed
MicroMoody with only the addresses changed and no mode set will run in
rgb fading mode (starting with green).

## I2C master operation

When compiled with -DI2CMASTER (disabled by default), the MicroMoody will act
as I2C bus master. After each transition (after half the time until the next
transition has passed) it will send its current color and fade delay as a
broadcast (payload address 0xffff) over the bus. The I2C address used will be
the MicroMoody's I2C address (see above for default).

If it is operating with hard transitions, so will the slaves. same for soft
(fading) transitions.
