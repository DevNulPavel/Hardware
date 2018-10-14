#define F_CPU 9600000UL

#include <avr/io.h>
#include <util/delay.h>

#define LED_PIN 0

int main() {
	DDRB |= 1 << LED_PIN;
	while(1) {
		PORTB |= 1 << LED_PIN;
		_delay_ms(100);
		PORTB &= ~(1 << LED_PIN);
		_delay_ms(100);
	}
	return 0;
}