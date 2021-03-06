/*
 * Copyright (c) 2013, Daniel Friesel <derf@chaosdorf.de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

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

#define OM_MODE_STEADY        (  0)
#define OM_MODE_BLINKRGB      (  1)
#define OM_MODE_BLINKRAND     (  2)
#define OM_MODE_BLINKONOFF    (  3)
#define OM_MODE_FADETOSTEADY  (  4)
#define OM_MODE_FADERGB       (  5)
#define OM_MODE_FADERAND      (  6)
#define OM_MODE_FADEONOFF     (  7)
#define OM_MODE_TEMPERATURE   (  8)
#define OM_MODE_STARTANIM     ( 63)
#define OM_MODE_ANIM_LOW      ( 64)
#define OM_MODE_ANIM_HIGH     (127)
#define OM_MODE_SAVESTATE     (240)
#define OM_MODE_SETADDR       (241)

#define TEMPERATURE_ZERO (355)

#define P_SDA PB0
#define P_SCL PB2

#ifdef I2CMASTER
#undef I2CEN
#endif

volatile uint8_t want_blue,  cache_blue;
volatile uint8_t want_green, cache_green;
volatile uint8_t want_red,   cache_red;

volatile uint8_t is_blue = 0;
volatile uint8_t opmode, speed;

volatile uint8_t seq[257];
volatile uint8_t step = 0;
volatile uint16_t animstep = 0;

volatile uint8_t addr_hi  = 0x00;
volatile uint8_t addr_lo  = 0x01;
volatile uint8_t addr_i2c = 0x11;

volatile enum {
	S_NONE, S_ACK, S_BYTE } comm_status = S_NONE;

uint8_t ee_valid, ee_mode, ee_speed, ee_red, ee_green, ee_blue EEMEM;
uint8_t ee_seq[257] EEMEM;
uint8_t ee_addrhi, ee_addrlo, ee_addri2c EEMEM;

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
		VRED   = want_red   = cache_red   = eeprom_read_byte(&ee_red);
		VGREEN = want_green = cache_green = eeprom_read_byte(&ee_green);
		VBLUE  = want_blue  = cache_blue  = eeprom_read_byte(&ee_blue);

		opmode   = eeprom_read_byte(&ee_mode);
		speed    = eeprom_read_byte(&ee_speed);
		addr_hi  = eeprom_read_byte(&ee_addrhi);
		addr_lo  = eeprom_read_byte(&ee_addrlo);
		addr_i2c = eeprom_read_byte(&ee_addri2c);

		if (opmode == OM_MODE_STARTANIM) {
			VRED = VGREEN = VBLUE = speed = 0;
			seq[256] = eeprom_read_byte(&ee_seq[256]);
			for (cnt = 0; cnt < 255; cnt++)
				seq[cnt] = eeprom_read_byte(&ee_seq[cnt]);
			seq[255] = eeprom_read_byte(&ee_seq[255]);
		}
	} else {
		opmode = OM_MODE_BLINKRGB;
		speed  = 32;
		VRED   = want_red   = cache_red   = 255;
		VGREEN = want_green = cache_green = 255;
		VBLUE  = want_blue  = cache_blue  = 0;
	}


#ifdef I2CEN
	USICR = _BV(USIWM1) | _BV(USICS1);
	USISR = 0xf0;
	DDRB &= ~_BV(P_SDA);
	PORTB &= ~_BV(P_SDA);
	DDRB |= _BV(P_SCL);
	PORTB |= _BV(P_SCL);
	USICR |= _BV(USISIE);

#endif

#ifdef TEMPERATURE

	ADMUX = _BV(REFS1) | 0x0f;
	ADCSRB = _BV(ADTS2);
	ADCSRA = _BV(ADEN) | _BV(ADATE);
	ADCSRA |= _BV(ADSC);

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

#ifdef I2CMASTER

#define TWI_DELAY_SHORT 8
#define TWI_DELAY_LONG  42

/*
 * since we're doing software pwm, we can't wait for slaves. so this will
 * probably only work on a bus exclusively used by MicroMoodies.
 * TODO: move soft pwm to interrupt handler?
 * Also TODO: Use hardware shift register (USIDR). Fixes welcome.
 */

void sda_high() {
	DDRB  &= ~_BV(PB0);
}

void sda_low() {
	DDRB  |=  _BV(PB0);
}

void scl_high() {
	DDRB  &= ~_BV(PB2);
}

void scl_low() {
	DDRB  |=  _BV(PB2);
}

void twi_delay(uint8_t delay)
{
	uint8_t i;
	for (i = 0; i < delay; i++)
		asm("nop");
}

void twi_tx_byte(uint8_t byte)
{
	int8_t i;

	for (i = 7; i >= -1; i--) {
		twi_delay(TWI_DELAY_SHORT);
		if ((i < 0) || (byte & _BV(i)))
			sda_high();
		else
			sda_low();
		twi_delay(TWI_DELAY_SHORT);
		scl_high();
		twi_delay(TWI_DELAY_SHORT);
		scl_low();
	}
	twi_delay(TWI_DELAY_LONG);
}


#endif /* I2CMASTER */

ISR(TIMER0_OVF_vect)
{
	uint8_t seq_addr;

#ifdef I2CMASTER
	static uint8_t i2cstep = 0;
#endif

#ifdef TEMPERATURE
	int16_t thermal = 0;

	if (opmode == OM_MODE_TEMPERATURE) {
		thermal = ADCL;
		thermal |= (ADCH << 8);
		thermal -= TEMPERATURE_ZERO;
	}
#endif

	step++;
	if ((step % 64) == 0)
		animstep++;

	if (( ((step % ( ( speed ) * 1)) == 0) )
			&& (opmode >= 4) ) {

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

#ifdef I2CMASTER
	if ((animstep == ( ( (uint16_t)speed) << 0) ) && (i2cstep <= 3)) {
		switch (i2cstep) {
			case 0:
				sda_low();
				break;
			case 1:
				scl_low();
				break;
			case 2:
				twi_tx_byte(addr_i2c << 1);
				if (opmode >= 4) {
					twi_tx_byte(OM_MODE_FADETOSTEADY);
					twi_tx_byte(speed);
					twi_tx_byte(want_red);
					twi_tx_byte(want_green);
					twi_tx_byte(want_blue);
				}
				else {
					twi_tx_byte(OM_MODE_STEADY);
					twi_tx_byte(speed);
					twi_tx_byte(VRED);
					twi_tx_byte(VGREEN);
					twi_tx_byte(VBLUE);
				}
				break;
			case 3:
				twi_tx_byte(0xff);
				twi_tx_byte(0xff);
				scl_high();
				sda_high();
				break;
		}

		i2cstep++;
	}
#endif

	if (animstep == ( ( (uint16_t)speed ) << 2 ) ) {
		animstep = 0;
#ifdef I2CMASTER
		i2cstep = 0;
#endif
		switch (opmode) {
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
#ifdef TEMPERATURE
			case OM_MODE_TEMPERATURE:
				VGREEN = 0;

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
				break;
#endif /* TEMPERATURE */
			case OM_MODE_STARTANIM:
				opmode = OM_MODE_ANIM_LOW;
				// fall-through
			default:
				if ((opmode >= OM_MODE_ANIM_LOW) && (opmode < OM_MODE_ANIM_HIGH)) {
					seq_addr = (opmode - OM_MODE_ANIM_LOW) * 4;
					speed      = seq[ seq_addr + 0 ];
					want_red   = seq[ seq_addr + 1 ];
					want_green = seq[ seq_addr + 2 ];
					want_blue  = seq[ seq_addr + 3 ];

					if (opmode >= seq[256])
						opmode = OM_MODE_ANIM_LOW;
					else
						opmode++;
				}
				break;
		}
	}

	asm("wdr");
}

#ifdef I2CEN

ISR(USI_START_vect)
{
	// Wait for SCL to go low to ensure the "Start Condition" has completed.
	// otherwise the counter will count the transition
	while ( PINB & _BV(P_SCL) ) ;
	USISR = 0xf0; // write 1 to clear flags; clear counter
	// enable USI interrupt on overflow; SCL goes low on overflow
	USICR |= _BV(USIOIE) | _BV(USIWM0);
	comm_status = S_NONE;
}

ISR(USI_OVF_vect)
{
	static uint8_t rcvbuf[7];
	static uint8_t rcvcnt = sizeof(rcvbuf);
	uint8_t seq_addr;

	switch (comm_status) {
		case S_NONE:
			if ((USIDR & 0xfe) == (addr_i2c << 1)) {
				rcvcnt = sizeof(rcvbuf);
				DDRB |= _BV(P_SDA);
				USISR = 0x0e;
				comm_status = S_ACK;
			}
			else {
				USICR &= ~(_BV(USIOIE) | _BV(USIWM0));
			}
			break;
		case S_ACK:
			DDRB &= ~_BV(P_SDA);
			comm_status = S_BYTE;
			break;
		case S_BYTE:
			rcvbuf[--rcvcnt] = USIDR;
			DDRB |= _BV(P_SDA);
			USISR = 0x0e;
			comm_status = S_ACK;
			break;
	}

	if (!rcvcnt && (comm_status == S_BYTE)) {
		comm_status = S_NONE;
		USICR &= ~(_BV(USIOIE) | _BV(USIWM0));
		rcvcnt = sizeof(rcvbuf);
		if (
				(((rcvbuf[1] == addr_hi) && (rcvbuf[0] == addr_lo))
				 || ((rcvbuf[1] == 0xff) && (rcvbuf[0] == 0xff)))) {
			step = animstep = 0;

			if (rcvbuf[6] < 4) {
				VRED   = cache_red   = rcvbuf[4];
				VGREEN = cache_green = rcvbuf[3];
				VBLUE  = cache_blue  = rcvbuf[2];
			}
			else if (rcvbuf[6] < 8) {
				want_red   = cache_red   = rcvbuf[4];
				want_green = cache_green = rcvbuf[3];
				want_blue  = cache_blue  = rcvbuf[2];
			}
			else if (rcvbuf[6] == OM_MODE_SETADDR) {
				addr_hi  = rcvbuf[4];
				addr_lo  = rcvbuf[3];
				addr_i2c = rcvbuf[2];

				opmode = OM_MODE_FADERGB;
				speed = 64;
				want_red = cache_red = 0;
				want_green = cache_green = 255;
				want_blue = cache_blue = 0;
			}

			if (rcvbuf[6] <= OM_MODE_STARTANIM) {
				opmode = rcvbuf[6];
				speed = rcvbuf[5];
			}
			else if (rcvbuf[6] < OM_MODE_ANIM_HIGH) {
				seq_addr = (rcvbuf[6] - OM_MODE_ANIM_LOW) * 4;
				seq[ seq_addr + 0 ] = rcvbuf[5];
				seq[ seq_addr + 1 ] = rcvbuf[4];
				seq[ seq_addr + 2 ] = rcvbuf[3];
				seq[ seq_addr + 3 ] = rcvbuf[2];
				seq[256] = rcvbuf[6];
			}
			else if (rcvbuf[6] >= OM_MODE_SAVESTATE) {
				if (opmode > OM_MODE_STARTANIM)
					opmode = OM_MODE_STARTANIM;
				eeprom_write_byte(&ee_valid, 1);
				eeprom_write_byte(&ee_addrhi, addr_hi);
				eeprom_write_byte(&ee_addrlo, addr_lo);
				eeprom_write_byte(&ee_addri2c, addr_i2c);
				eeprom_write_byte(&ee_mode, opmode);
				if (opmode < OM_MODE_STARTANIM) {
					eeprom_write_byte(&ee_speed, speed);
					eeprom_write_byte(&ee_red, cache_red);
					eeprom_write_byte(&ee_green, cache_green);
					eeprom_write_byte(&ee_blue, cache_blue);
				}
				else {
					eeprom_write_byte(&ee_seq[256], seq[256]);
					if (seq[256] == OM_MODE_ANIM_HIGH) {
						for (seq_addr = 0; seq_addr < 255; seq_addr++)
						eeprom_write_byte(&ee_seq[seq_addr], seq[seq_addr]);
						eeprom_write_byte(&ee_seq[255], seq[255]);
					}
					else {
						for (seq_addr = 0; seq_addr <= ((seq[256] - OM_MODE_ANIM_LOW) * 4 + 3); seq_addr++) {
							eeprom_write_byte(&ee_seq[seq_addr], seq[seq_addr]);
						}
					}
				}
			}
		}
	}

	USISR |= _BV(USIOIF);
}

#endif /* I2CEN */
