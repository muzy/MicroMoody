#include <avr/eeprom.h>
#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

/*
 * PB1: green (OC1A)
 * PB3: blue  (soft pwm)
 * PB4: red   (OC1B)
 */

#define VRED   ( OCR1B   )
#define VGREEN ( OCR1A   )
#define VBLUE  ( is_blue )

#define MYADDRESS (0x0001)
#define BROADCAST (0xffff)

#define OM_M_MODE  ( _BV(7) | _BV(6) | _BV(5) )
#define OM_M_SPEED ( _BV(0) | _BV(1) | _BV(2) | _BV(3) | _BV(4) )

#define OM_MODE_STEADY        (     0  |     0  |     0  )
#define OM_MODE_BLINKRGB      (     0  |     0  | _BV(5) )
#define OM_MODE_BLINKRAND     (     0  | _BV(6) |     0  )
#define OM_MODE_BLINKONOFF    (     0  | _BV(6) | _BV(5) )
#define OM_MODE_FADEANY       ( _BV(7) |     0  |     0  )
#define OM_MODE_FADETOSTEADY  ( _BV(7) |     0  |     0  )
#define OM_MODE_FADERGB       ( _BV(7) |     0  | _BV(5) )
#define OM_MODE_FADERAND      ( _BV(7) | _BV(6) |     0  )
#define OM_MODE_FADEONOFF     ( _BV(7) | _BV(6) | _BV(5) )


volatile uint8_t want_blue = 0;
volatile uint8_t want_green = 0;
volatile uint8_t want_red = 0;

volatile uint8_t is_blue = 0;
volatile uint8_t opmode = OM_MODE_FADERAND | 8;

const uint8_t pwmtable[32] PROGMEM = {
	0, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 8, 10, 11, 13, 16, 19, 23,
	27, 32, 38, 45, 54, 64, 76, 91, 108, 128, 152, 181, 215, 255
};

int main (void)
{
	
	DDRB = _BV(DDB1) | _BV(DDB3) | _BV(DDB4);
	PORTB = _BV(PB1) | _BV(PB3) | _BV(PB4);

	uint8_t cnt = 0;

	TCCR0A = _BV(WGM01) | _BV(WGM00);
	TCCR0B = _BV(CS01);
	TIMSK = _BV(TOIE0);

	GTCCR = _BV(PWM1B) | _BV(COM1B1) | _BV(COM1B0);
	TCCR1 = _BV(PWM1A) | _BV(COM1A1) | _BV(COM1A0) | _BV(CS10);
	OCR1A = 0;
	OCR1B = 0;
	OCR1C = 0xff;

	sei();

	while (1) {
		cnt++;
		if (cnt == 0 && VBLUE) {
			PORTB &= ~_BV(PB3);
		}
		if (cnt == VBLUE)
			PORTB |= _BV(PB3);
	}

	return 0;
}

ISR(TIMER0_OVF_vect)
{
	static uint8_t step = 0;
	static uint8_t animstep = 0;

	step++;
	if ((step % 64) == 0)
		animstep++;

	if (( ((step % ( (opmode & OM_M_SPEED) + 1 )) == 0) )
			&& (opmode & OM_MODE_FADEANY ) ) {

		if (VRED > want_red)
			VRED--;
		if (VRED < want_red)
			VRED++;
		if (VGREEN > want_green)
			VGREEN--;
		if (VGREEN < want_green)
			VGREEN++;
		if (VBLUE > want_blue)
			VBLUE--;
		if (VBLUE < want_blue)
			VBLUE++;
	}

	if (animstep == ( ( (opmode & OM_M_SPEED) + 1 ) * 4 ) ) {
		animstep = 0;
		switch (opmode & OM_M_MODE) {
			case OM_MODE_BLINKRGB:
				if (!VBLUE && VRED && !VGREEN)
					VGREEN = 255;
				else if (VRED && VGREEN)
					VRED = 0;
				else if (VGREEN && !VBLUE)
					VBLUE = 255;
				else if (VGREEN && VBLUE)
					VGREEN = 0;
				else if (VBLUE && !VRED)
					VRED = 255;
				else if (VBLUE && VRED)
					VBLUE = 0;
				else
					VRED = 255;
				break;
			case OM_MODE_BLINKRAND:
				VRED   = pwmtable[ rand() & 32 ];
				VGREEN = pwmtable[ rand() & 32 ];
				VBLUE  = pwmtable[ rand() & 32 ];
				break;
			case OM_MODE_BLINKONOFF:
				if (VRED == 127) {
					VRED = VGREEN = VBLUE = 0;
				}
				else {
					VRED   = 127;
					VGREEN = 255;
					VBLUE  = 255;
				}
				break;
			case OM_MODE_FADEONOFF:
				if (want_red == 127) {
					want_red = want_green = want_blue = 0;
				}
				else {
					want_red   = 127;
					want_green = 255;
					want_blue  = 255;
				}
				break;
			case OM_MODE_FADERGB:
				if (!want_blue && want_red && !want_green)
					want_green = 255;
				else if (want_red && want_green)
					want_red = 0;
				else if (want_green && !want_blue)
					want_blue = 255;
				else if (want_green && want_blue)
					want_green = 0;
				else if (want_blue && !want_red)
					want_red = 255;
				else if (want_blue && want_red)
					want_blue = 0;
				else
					want_red = 255;
				break;
			case OM_MODE_FADERAND:
				want_red   = pwmtable[ rand() & 32 ];
				want_green = pwmtable[ rand() & 32 ];
				want_blue  = pwmtable [rand() & 32 ];
				break;
			case OM_MODE_FADETOSTEADY:
				want_red = 127;
				want_green = 255;
				want_blue = 255;
				break;
		}
	}
}
