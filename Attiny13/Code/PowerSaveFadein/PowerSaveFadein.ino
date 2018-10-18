// http://www.count-zero.ru/2016/attiny13/
// http://osboy.ru/blog/microcontrollers/attiny13-switch.html
// http://1io.ru/articles/microcontroller/attiny13/


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


volatile bool keyInterrupt = false;

// Обработчик прерывания PCINT0, доступно на любых ножках
ISR(PCINT0_vect) {
    keyInterrupt = true;
}

int main(void) {
    // В регистр энергосбережения записываем PRTIM0 (отключение счетчика) и PRADC (отключение ADC преобразователя)
    PRR = (1<<PRTIM0) | (1<<PRADC);

    // В регистр энергосбережения PRADC (отключение ADC преобразователя)
    // PRR = (1<<PRADC);

    // Отключение AЦП
    ADCSRA &= ~(1 << ADEN); // Отключение AЦП
    ACSR |= (1 << ACD);     // И компаратор

    // Пины кнопок
    DDRB &= ~(1<<PB1); // Настраиваем кнопку на порте PB1 как вход
    PORTB |= (1<<PB1); // Делаем кнопку на порте PB1 подтянутой к ВЫСОКОМУ уровню, подключать к земле

    // Пин светодиода
    DDRB |= (1<<PB0); // Настраиваем выход PB0 как выход
    PORTB &= ~(1<<PB0); // Выход на порте PB0 выключен

    // Настройка прерываний для кнопки
    GIMSK |= (1<<PCIE); // Разрешаем внешние прерывания PCINT0, работают толкьо на изменение состояний
    PCMSK |= (1<<PB1);  // Разрешаем по маске прерывания на ноге PB1
    //MCUCR |= (1<<ISC01)// Прерывания будут при падении уровня, поддерживается только у прерывания INT0, не у PCINT, ISC01, ISC00 биты для настроек
    SREG |= (1<<SREG_I); // Разрешаем прерывания, sei();

    // Настройки режима сна
    MCUCR &= ~(1<<SM1); // Включаем idle mode, set_sleep_mode(SLEEP_MODE_IDLE);
    MCUCR &= ~(1<<SM0); // Включаем idle mode, set_sleep_mode(SLEEP_MODE_IDLE);
    MCUCR |= (1<<SE);   // Включаем режим сна, sleep_enable();

    // Обнуление переменных
    keyInterrupt = false;

    // Главный цикл
    while(1) {
        // Просто перекидываем процессор в сон, пробуждение по прерыванию
        asm("sleep"); // sleep_cpu();

        // Если было прерывание - обрабатываем его
        if (keyInterrupt){
            bool buttonPressed = ((PINB & (1<<PB1)) == 0); // Если низкий уровень - то кнопка нажата
            if (buttonPressed){
                _delay_ms(10); // Ждем чтобы избавиться от дребезга
                buttonPressed = ((PINB & (1<<PB1)) == 0); // Если низкий уровень - то кнопка нажата
                if (buttonPressed){
                    PORTB ^= (1<<PB0);  // Переключаем значение на пине PB0
                }
            }

            keyInterrupt = false;
        }
    }

    return 0;
}