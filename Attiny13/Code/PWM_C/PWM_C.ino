// http://osboy.ru/blog/microcontrollers/attiny13-pwm.html


#define F_CPU 1200000UL  // Частота 1.2Mhz
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#ifndef CLEAR_BIT
    #define CLEAR_BIT(SRC, BIT) (_SFR_BYTE(SRC) &= ~_BV(BIT))
#endif
#ifndef SET_BIT
    #define SET_BIT(SRC, BIT) (_SFR_BYTE(SRC) |= _BV(BIT))
#endif
#ifndef INVERT_BIT
    #define INVERT_BIT(SRC, BIT) (_SFR_BYTE(SRC) ^= _BV(BIT))
#endif


int main(void) {
    // Настройка выхода
    DDRB |= (1 << PB0); // Делаем порт PB0 выходом
    PORTB &= ~(1 << PB0); // Низкий уровень

    // Таймер для ШИМ на ножке PB0
    TCCR0B = (1<<CS01); // 0x02, предделитель тактовой частоты CLK/8 (биты CS02, CS01, CS00)
    TCCR0A = (1<<WGM01) | (1<<WGM00); // Настраиваем режим шим (биты WGM01, WGM00)
    TCCR0A = (1<<COM0A1); // Настраиваем неинверсный режим (биты COM0A1, COM0A0) (может лучше инверсный?)
    TCNT0 = 0; // Начальное значение счётчика
    OCR0A = 0; // Регистр совпадения A, ножка PB0

    // Главный цикл
    while(1) {

        // Нарастание яркости
        do {
            OCR0A++; // Увеличиваем значение, с которым происходит сравнение счетчика
            _delay_ms(5);
        } while(OCR0A < 255);

        // Пауза 1 сек.
        _delay_ms(1000); 
        
        // Затухание
        do{
            OCR0A--; // Уменьшаем значение, с которым происходит сравнение счетчика
            _delay_ms(5);
        } while(OCR0A > 0);

         // Пауза 1 сек.
        _delay_ms(1000);
    }

    return 0;
}