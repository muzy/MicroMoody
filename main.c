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

#define ADDRHI (0x00)
#define ADDRLO (0x01)

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

#define TEMPERATURE_ZERO (210)

volatile uint8_t want_blue,  cache_blue;
volatile uint8_t want_green, cache_green;
volatile uint8_t want_red,   cache_red;

volatile uint8_t is_blue = 0;
volatile uint8_t opmode;

volatile uint8_t step = 0;
volatile uint8_t animstep = 0;

uint8_t ee_valid, ee_mode, ee_red, ee_green, ee_blue EEMEM;

const uint8_t pwmtable[32] PROGMEM = {
	0, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 8, 10, 11, 13, 16, 19, 23,
	27, 32, 38, 45, 54, 64, 76, 91, 108, 128, 152, 181, 215, 255
};

int main (void)
{
	
	DDRB = _BV(DDB1) | _BV(DDB3) | _BV(DDB4);
	PORTB = _BV(PB1) | _BV(PB3) | _BV(PB4);

	uint8_t cnt = 0;

	WDTCR = _BV(WDE) | _BV(WDP3);

	TCCR0A = _BV(WGM01) | _BV(WGM00);
	TCCR0B = _BV(CS01);
	TIMSK = _BV(TOIE0);

	GTCCR = _BV(PWM1B) | _BV(COM1B1) | _BV(COM1B0);
	TCCR1 = _BV(PWM1A) | _BV(COM1A1) | _BV(COM1A0) | _BV(CS10);
	OCR1A = 0;
	OCR1B = 0;
	OCR1C = 0xff;

	if (eeprom_read_byte(&ee_valid) == 1) {
		opmode =              eeprom_read_byte(&ee_mode);
		VRED   = want_red   = eeprom_read_byte(&ee_red);
		VGREEN = want_green = eeprom_read_byte(&ee_green);
		VBLUE  = want_blue  = eeprom_read_byte(&ee_blue);
	} else {
		opmode = OM_MODE_FADERAND | 30;
		VRED   = want_red   = cache_red   = 0;
		VGREEN = want_green = cache_green = 255;
		VBLUE  = want_blue  = cache_blue  = 255;
	}


#ifdef I2CEN
	USICR = /* _BV(USISIE) | */ _BV(USIOIE) | _BV(USIWM1) | _BV(USICS1);
#endif

#ifdef TEMPERATURE

	ADMUX = _BV(REFS1) | 0x0f;
	ADCSRA = _BV(ADEN) | _BV(ADSC);

#endif /* TEMPERATURE */

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

#ifdef TEMPERATURE

	int16_t thermal = ADCL;
	thermal |= (ADCH << 8);
	thermal -= TEMPERATURE_ZERO;

	if (thermal < 0)
		VBLUE = 0xff;
	else if (thermal < 32)
		VBLUE = pwmtable[31 - thermal];
	else
		VBLUE = 0;

	if (thermal < 20)
		VRED = 0;
	else if (thermal < 52)
		VRED = pwmtable[thermal - 20];
	else
		VRED = 0xff;

#else

	step++;
	if ((step % 32) == 0)
		animstep++;

	if (( ((step % ( ((opmode & OM_M_SPEED) + 1 ) * 1)) == 0) )
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
				if ((VRED == cache_red) && (VGREEN == cache_green)
						&& (VBLUE == cache_blue)) {
					VRED = VGREEN = VBLUE = 0;
				}
				else {
					VRED   = cache_red;
					VGREEN = cache_green;
					VBLUE  = cache_blue;
				}
				break;
			case OM_MODE_FADEONOFF:
				if ((want_red == cache_red) && (want_green == cache_green)
						&& (want_blue == cache_blue)) {
					want_red = want_green = want_blue = 0;
				}
				else {
					want_red   = cache_red;
					want_green = cache_green;
					want_blue  = cache_blue;
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
				want_blue  = pwmtable[ rand() & 32 ];
				break;
			case OM_MODE_FADETOSTEADY:
				break;
		}
	}

#endif /* !TEMPERATURE */

	asm("wdr");
}

#ifdef I2CEN
#ifndef TEMPERATURE

ISR(USI_OVF_vect)
{
	static uint8_t rcvbuf[6];
	static uint8_t rcvcnt = sizeof(rcvbuf) - 1;


	rcvbuf[rcvcnt] = USIBR;

	if (rcvcnt) {
		rcvcnt--;
	} else {
		rcvcnt = sizeof(rcvbuf) - 1;
		if ((rcvbuf[1] == ADDRHI) && (rcvbuf[0] == ADDRLO)) {
//		if ((rcvbuf[1] == ADDRHI) && (rcvbuf[0] == 1)) {
			step = animstep = 0;
			opmode = rcvbuf[5];
			if (opmode & OM_MODE_FADEANY) {
				want_red   = cache_red   = rcvbuf[4];
				want_green = cache_green = rcvbuf[3];
				want_blue  = cache_blue  = rcvbuf[2];
			}
			else if ((opmode | OM_MODE_STEADY) == 0) {
				VRED   = cache_red   = rcvbuf[4];
				VGREEN = cache_green = rcvbuf[3];
				VBLUE  = cache_blue  = rcvbuf[2];
			}
			eeprom_write_byte(&ee_valid, 1);
			eeprom_write_byte(&ee_mode, opmode);
			eeprom_write_byte(&ee_red, rcvbuf[4]);
			eeprom_write_byte(&ee_green, rcvbuf[3]);
			eeprom_write_byte(&ee_blue, rcvbuf[2]);
		}
	}

	USISR |= _BV(USIOIF);
}

#endif /* !TEMPERATURE */
#endif /* I2CEN */
