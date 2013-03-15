#include <avr/eeprom.h>
#include <avr/io.h>
#include <math.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

volatile struct {
	unsigned blue:1;
	unsigned green:1;
	unsigned red:1;
} ledstate;

void set_led(void){
	// in difference to the schematic, PB4 is red, PB1 is green and PB3 is blue
	// not because of common cathode
	PORTB = (!ledstate.red << PB4) | (!ledstate.green << PB1)
		| (!ledstate.blue << PB3);
}

int main(void){
	
	DDRB = _BV(DDB1) | _BV(DDB3) | _BV(DDB4);

	uint16_t fadecnt = 0;

	uint8_t cnt = 0;
	uint8_t want_blue = 0;
	uint8_t want_green = 0;
	uint8_t want_red = 10;

	uint8_t is_blue, is_green, is_red;

	while (1) {
		cnt++;
		if (cnt == 0) {
			ledstate.red = ledstate.green = ledstate.blue = 1;
			if (++fadecnt == 2048) {
				fadecnt = 0;
				want_blue = rand();
				want_green = rand();
				want_red = rand();
			}
			if ((fadecnt % 8) == 0) {
				if (is_red > want_red)
					is_red--;
				if (is_red < want_red)
					is_red++;
				if (is_green > want_green)
					is_green--;
				if (is_green < want_green)
					is_green++;
				if (is_blue > want_blue)
					is_blue--;
				if (is_blue < want_blue)
					is_blue++;
			}
		}
		if (cnt == is_blue)
			ledstate.blue = 0;
		if (cnt == is_green)
			ledstate.green = 0;
		if (cnt == is_red)
			ledstate.red = 0;

		set_led();
	}

	return 0;
}
