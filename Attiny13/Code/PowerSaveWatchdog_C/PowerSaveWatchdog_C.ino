// http://www.count-zero.ru/2016/attiny13/


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
#ifndef BIT_IS_SET
    #define BIT_IS_SET(SRC, BIT) (_SFR_BYTE(SRC) & _BV(BIT))
#endif



volatile uint8_t i = 0;

// Прерывание по WatchDog
ISR (WDT_vect) {
    if ((++i%4) == 0){
        PORTB |= (1<<PB0); // Включаем светодиод
    }else{
        PORTB &= ~(1<<PB0); // Выключаем светодиод
    }
    WDTCR |= (1<<WDTIE); // Разрешаем прерывания по ватчдогу. Иначе будет резет.
}

int main(void) {
    // Обнуление переменных
    i = 0;

    // В регистр энергосбережения записываем PRTIM0 (отключение счетчика) и PRADC (отключение ADC преобразователя)
    PRR |= (1<<PRTIM0) | (1<<PRADC);

    // В регистр энергосбережения PRADC (отключение ADC преобразователя)
    // PRR |= (1<<PRADC);
    // PRR &= ~(1<<PRTIM0);

    // Настройка PB0 порта на вывод
    DDRB |= (1<<PB0);

    // Настройка WatchDog
    asm("wdr");    // Сбрасываем, wdt_reset();
    wdt_enable(WDTO_1S); // Разрешаем ватчдог 1 сек, отключение - wdt_disable()
    WDTCR |= (1<<WDTIE); // Разрешаем прерывания по ватчдогу. Иначе будет резет.
    SREG |= (1<<SREG_I); // Разрешаем прерывания, sei();

    // Включаем полный режим сна, прерывание будет по таймеру WatchDog
    MCUCR |= (1<<SM1);  // Включаем power down mode, set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    MCUCR &= ~(1<<SM0); // Включаем power down mode, set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    MCUCR |= (1<<SE);   // Включаем режим сна, sleep_enable();

    // Главный цикл
    while(1) {
        // Включаем сон
        MCUCR |= (1<<SE);   // Включаем режим сна, sleep_enable();

        // Просто перекидываем процессор в сон, пробуждение по прерыванию
        asm("sleep"); // sleep_cpu();
    }
    
    return 0;
}
