#include <avr/eeprom.h>
#include <avr/io.h>
#include <math.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

void set_led(char red, char green, char blue){
	// in difference to the schematic, PB4 is red, PB1 is green and PB3 is blue
	// not because of common cathode
	PORTB = (!red << PB4) | (!green << PB1) | (!blue << PB3);
}

int main(void){
	
//	uint16_t softpwm = 0;
	uint8_t r=0, g=85, b=170;
	
	DDRB = _BV(DDB1) | _BV(DDB3) | _BV(DDB4);	
	PORTB = 0xFF;
	
	
/*	TCCR1 = _BV(COM1A1) | _BV(COM1A0) | _BV(PWM1A) | _BV(CS10);
	TCCR0A = _BV(WGM01) | _BV(WGM00);
	GTCCR = _BV(COM1B1) | _BV(COM1B0) | _BV(PWM1B);
		

	OCR1A = 255;
	OCR1C = 255; 
	OCR1B = 127;
*/	
	while(1){
		
	//	set_led(1,1,1);	
	//	_delay_ms(1000);	
	}
	return 0;
}
