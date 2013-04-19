#include <avr/eeprom.h>
#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

/*
 * PB1: green (OC1A)
 * PB3: blue  (soft pwm)
 * PB4: red   (OC1B)
 */

volatile uint8_t want_blue = 0;
volatile uint8_t want_green = 0;
volatile uint8_t want_red = 10;

volatile uint8_t is_blue, is_green, is_red;

int main(void){
	
	DDRB = _BV(DDB1) | _BV(DDB3) | _BV(DDB4);
	PORTB = _BV(PB1) | _BV(PB3) | _BV(PB4);

	uint8_t cnt = 0;
	uint8_t state_blue = 0;


	TCCR0A = _BV(WGM01) | _BV(WGM00);
	TCCR0B = _BV(CS02);
	TIMSK = _BV(TOIE0);

	GTCCR = _BV(PWM1B) | _BV(COM1B1) | _BV(COM1B0);
	TCCR1 = _BV(PWM1A) | _BV(COM1A1) | _BV(COM1A0) | _BV(CS10);
	OCR1A = 0;
	OCR1B = 0;
	OCR1C = 0xff;

	sei();

	while (1) {
		cnt++;
		if (cnt == 0) {
			PORTB |= _BV(PB3);
		}
		if (cnt == is_blue)
			PORTB &= ~_BV(PB3);
	}

	return 0;
}

ISR(TIMER0_OVF_vect) {
	static uint8_t fadecnt = 0;

	if (++fadecnt == 255) {
		fadecnt = 0;
		want_blue = rand();
		want_green = rand();
		want_red = rand();
	}

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

	OCR1A = is_green;
	OCR1B = is_red;
}
