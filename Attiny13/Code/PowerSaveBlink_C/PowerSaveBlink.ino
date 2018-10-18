// http://www.count-zero.ru/2016/attiny13/


#define F_CPU 1200000UL  // Частота 1.2Mhz
#include <avr/io.h>
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

volatile uint8_t i = 0;

// Обработчик прерывания по таймеру
ISR(TIM0_OVF_vect) {
    if (!(++i % 16)){ // Период 4s
        i = 0;
        PORTB ^= (1<<PB0); // Переключаем значение на пине PB0
    }
}

int main(void) {
    // Обнуление переменных
    i = 0;

    // В регистр энергосбережения записываем PRTIM0 (отключение счетчика) и PRADC (отключение ADC преобразователя)
    // PRR = (1<<PRTIM0) | (1<<PRADC);

    // В регистр энергосбережения PRADC (отключение ADC преобразователя)
    PRR = (1<<PRADC);

    // Настройка таймера
    TIMSK0 = (1<<TOIE0);  // Включаем вызов прерывания у timer0 при переполнении
    TCCR0B = (1<<CS02) | (1<<CS00); // Предделитель 1/1024, вызываться переполнение будет раз в 250ms?

    // Настройка PB0 порта на вывод
    DDRB |= (1<<PB0);

    // Настройки режима сна, режим SLEEP_MODE_IDLE, чтобы работали таймеры?
    MCUCR &= ~(1<<SM1); // Включаем idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
    MCUCR &= ~(1<<SM0); // Включаем idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
    MCUCR |= (1<<SE);   // Включаем режим сна, sleep_enable();

    // Включаем прерывания
    SREG |= (1<<SREG_I); // Разрешаем прерывания, sei();

    // Главный цикл
    while(1) {
        // Включаем сон
        MCUCR |= (1<<SE);   // Включаем режим сна, sleep_enable();

        // Просто перекидываем процессор в сон, пробуждение по прерыванию
        asm("sleep"); // sleep_cpu();

        //PORTB ^= (1<<PB0);  // Переключаем значение на пине PB0
        //_delay_ms(3000);
    }

    return 0;
}