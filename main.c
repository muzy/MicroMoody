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
#include <avr/interrupt.h>

/*
 * PB1: green (OC1A)
 * PB3: blue  (soft pwm)
 * PB4: red   (OC1B)
 */

#define VR ( OCR1B   )
#define VG ( OCR1A   )
#define VB ( is_blue )

#define OM_MODE_ANIM_LOW      (  0)
#define OM_MODE_ANIM_HIGH     (111)
#define OM_MODE_STARTANIM     (112)
#define OM_MODE_STEADY        (128)
#define OM_MODE_BLINKRGB      (129)
#define OM_MODE_BLINKRAND     (130)
#define OM_MODE_BLINKONOFF    (131)
#define OM_MODE_FADETOSTEADY  (132)
#define OM_MODE_FADERGB       (133)
#define OM_MODE_FADERAND      (134)
#define OM_MODE_FADEONOFF     (135)
#define OM_MODE_SAVESTATE     (240)
#define OM_MODE_SETADDR       (241)

#define SEQ_MAX (448)

#define TEMPERATURE_ZERO (355)

#define P_SDA PB0
#define P_SCL PB2

#ifdef I2CMASTER
#undef I2CEN
#endif

typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
} rgbint8_t;

typedef struct {
	int16_t r;
	int16_t g;
	int16_t b;
} rgbint16_t;

volatile rgbint16_t c_dest = {0, 0, 0};
volatile rgbint16_t c_step = {0, 0, 0};
volatile rgbint16_t c_cur = {0, 0, 0};

volatile uint8_t is_blue = 0;
volatile uint8_t opmode = OM_MODE_ANIM_LOW, delay = 1;

volatile uint8_t seq[SEQ_MAX + 1];
volatile uint8_t step = 0;
volatile uint16_t animstep = 0;

volatile uint8_t addr_hi  = 0x00;
volatile uint8_t addr_lo  = 0x01;
volatile uint8_t addr_i2c = 0x11;

volatile enum {
	S_NONE, S_ACK, S_BYTE } comm_status = S_NONE;

uint8_t ee_valid EEMEM;
uint8_t ee_seq[SEQ_MAX + 1] EEMEM;
uint8_t ee_addrhi EEMEM, ee_addrlo EEMEM, ee_addri2c EEMEM;

static const __flash uint8_t pwmtable[32] = {
	0, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 8, 10, 11, 13, 16, 19, 23,
	27, 32, 38, 45, 54, 64, 76, 91, 108, 128, 152, 181, 215, 255
};

void calc_fadesteps()
{
	if (delay == 0) {
		c_cur.r = c_dest.r;
		c_cur.g = c_dest.g;
		c_cur.b = c_dest.b;
	}
	c_step.r = (c_dest.r - c_cur.r) / 260;
	c_step.g = (c_dest.g - c_cur.g) / 260;
	c_step.b = (c_dest.b - c_cur.b) / 260;
}

void set_colour()
{
	VR = c_cur.r / 64;
	VG = c_cur.g / 64;
	VB = c_cur.b / 64;
}

int main (void)
{
	
	DDRB = _BV(DDB1) | _BV(DDB3) | _BV(DDB4);
	PORTB = _BV(PB1) | _BV(PB3) | _BV(PB4);

	uint8_t cnt = 0;
	uint16_t seq_cnt = 0;

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
		addr_hi  = eeprom_read_byte(&ee_addrhi);
		addr_lo  = eeprom_read_byte(&ee_addrlo);
		addr_i2c = eeprom_read_byte(&ee_addri2c);

		for (seq_cnt = 0; seq_cnt <= SEQ_MAX; seq_cnt++)
			seq[seq_cnt] = eeprom_read_byte(&ee_seq[seq_cnt]);
	}
	else {
		seq[0] =  16;
		seq[1] =  40;
		seq[2] =  12;
		seq[3] =   0;
		seq[4] =  16;
		seq[5] =   0;
		seq[6] =   0;
		seq[7] =   0;
		seq[SEQ_MAX] = OM_MODE_ANIM_LOW + 1;
	}

	set_colour();

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
		if (cnt == 0 && VB) {
			PORTB &= ~_BV(PB3);
		}
		if (cnt == VB)
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
	uint16_t seq_addr;

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

	if ((step % delay) == 0) {

		c_cur.r += c_step.r;
		c_cur.g += c_step.g;
		c_cur.b += c_step.b;
	}

#ifdef I2CMASTER
	if ((animstep == ( ( (uint16_t)delay) << 0) ) && (i2cstep <= 3)) {

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
					twi_tx_byte(delay);
					twi_tx_byte(c_dest.r);
					twi_tx_byte(c_dest.g);
					twi_tx_byte(c_dest.b);
				}
				else {
					twi_tx_byte(OM_MODE_STEADY);
					twi_tx_byte(delay);
					twi_tx_byte(VR);
					twi_tx_byte(VG);
					twi_tx_byte(VB);
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

	if (animstep == (uint16_t)delay << 2 ) {
		animstep = 0;
		c_cur.r = c_dest.r;
		c_cur.g = c_dest.g;
		c_cur.b = c_dest.b;
#ifdef I2CMASTER
		i2cstep = 0;
#endif
		if (opmode <= OM_MODE_ANIM_HIGH) {
			seq_addr = (uint16_t)opmode * 4;
			delay    = seq[ seq_addr + 0 ];
			c_dest.r = seq[ seq_addr + 1 ] * 64;
			c_dest.g = seq[ seq_addr + 2 ] * 64;
			c_dest.b = seq[ seq_addr + 3 ] * 64;

			calc_fadesteps();

			if (opmode >= seq[SEQ_MAX])
				opmode = OM_MODE_ANIM_LOW;
			else
				opmode++;
		}
	}
	set_colour();

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
	int16_t seq_addr;

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
			if (rcvcnt) {
				DDRB |= _BV(P_SDA);
				USISR = 0x0e;
				comm_status = S_ACK;
			}
			else {
				comm_status = S_NONE;
				USISR &= ~(_BV(USIOIE) | _BV(USIWM0));
			}
			break;
	}

	if (rcvcnt == 0) {
		rcvcnt = sizeof(rcvbuf);
		if (
				(((rcvbuf[1] == addr_hi) && (rcvbuf[0] == addr_lo))
				 || ((rcvbuf[1] == 0xff) && (rcvbuf[0] == 0xff)))) {
			step = 0;

			if (rcvbuf[6] == OM_MODE_SETADDR) {
				addr_hi  = rcvbuf[4];
				addr_lo  = rcvbuf[3];
				addr_i2c = rcvbuf[2];
				for (seq_addr = 0; seq_addr < 64; seq_addr += 8) {
					seq[ seq_addr + 0 ] = 4;
					seq[ seq_addr + 1 ] = (addr_hi & _BV(7 - (seq_addr / 8))) ? 255 : 0;
					seq[ seq_addr + 2 ] = 255;
					seq[ seq_addr + 3 ] = ~seq[ seq_addr + 1 ];
					seq[ seq_addr + 4 ] = 8;
					seq[ seq_addr + 5 ] = 0;
					seq[ seq_addr + 6 ] = 0;
					seq[ seq_addr + 7 ] = 0;
				}
				for (seq_addr = 64; seq_addr < 128; seq_addr += 8) {
					seq[ seq_addr + 0 ] = 4;
					seq[ seq_addr + 1 ] = (addr_lo & _BV(15 - (seq_addr / 8))) ? 255 : 0;
					seq[ seq_addr + 2 ] = 255;
					seq[ seq_addr + 3 ] = ~seq[seq_addr + 1 ];
					seq[ seq_addr + 4 ] = 8;
					seq[ seq_addr + 5 ] = 0;
					seq[ seq_addr + 6 ] = 0;
					seq[ seq_addr + 7 ] = 0;
				}
				seq[128] = 32;
				seq[129] = 0;
				seq[130] = 0;
				seq[131] = 0;
				seq[SEQ_MAX] = OM_MODE_ANIM_LOW + 32;
			}

			if (rcvbuf[6] == OM_MODE_STARTANIM) {
				opmode = OM_MODE_ANIM_LOW;
			}
			else if (rcvbuf[6] <= OM_MODE_ANIM_HIGH) {
				seq_addr = rcvbuf[6] * 4;
				seq[ seq_addr + 0 ] = rcvbuf[5];
				seq[ seq_addr + 1 ] = rcvbuf[4];
				seq[ seq_addr + 2 ] = rcvbuf[3];
				seq[ seq_addr + 3 ] = rcvbuf[2];
				seq[SEQ_MAX] = rcvbuf[6];
				if (rcvbuf[6] == OM_MODE_ANIM_LOW) {
					animstep = (uint16_t)delay << 2;
					calc_fadesteps();
					set_colour();
				}
			}
			else if (rcvbuf[6] >= OM_MODE_SAVESTATE) {
				eeprom_write_byte(&ee_valid, 1);
				eeprom_write_byte(&ee_addrhi, addr_hi);
				eeprom_write_byte(&ee_addrlo, addr_lo);
				eeprom_write_byte(&ee_addri2c, addr_i2c);
				eeprom_write_byte(&ee_seq[SEQ_MAX], seq[SEQ_MAX]);
				if (seq[SEQ_MAX] == OM_MODE_ANIM_HIGH) {
					for (seq_addr = 0; seq_addr < SEQ_MAX; seq_addr++)
						eeprom_write_byte(&ee_seq[seq_addr], seq[seq_addr]);
				}
				else {
					for (seq_addr = 0; seq_addr <= (seq[SEQ_MAX] * 4 + 3); seq_addr++) {
						eeprom_write_byte(&ee_seq[seq_addr], seq[seq_addr]);
					}
				}
			}
		}
	}

	USISR |= _BV(USIOIF);
}

#endif /* I2CEN */
