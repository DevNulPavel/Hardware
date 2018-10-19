// http://www.count-zero.ru/2016/attiny13/
// http://osboy.ru/blog/microcontrollers/attiny13-switch.html
// http://1io.ru/articles/microcontroller/attiny13/
// http://we.easyelectronics.ru/AVR/avr-power-management-ili-kak-pravilno-spat.html
// http://avr.ru/beginer/understand/sleep_mode
// http://osboy.ru/blog/microcontrollers/attiny13-pwm.html
// http://www.customelectronics.ru/avr-apparatnyiy-shim-mikrokontrollera/


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



bool outEnabled = false;
volatile bool keyInterrupt = false;


// Обработчик прерывания PCINT0, доступно на любых ножках, срабатывает на изменение уровня
// Если нужно настроить на конкретный тип изменения уровня - надо использовать прерывание INT0 на ноге PB1
// Так же по прерыванию происходит пробуждение микроконтроллера
ISR(PCINT0_vect) {
    keyInterrupt = true;
}

// Выставить порты в конкретное состояние InputPullup, чтобы по ним не происходили прерывания
void initialSetupOutPorts(){
    DDRB &= ~(1<<PB0); // Настраиваем кнопку на порте как вход
    PORTB |= (1<<PB0); // Делаем кнопку на порте подтянутой к ВЫСОКОМУ уровню, подключать к земле

    DDRB &= ~(1<<PB1); // Настраиваем кнопку на порте как вход
    PORTB |= (1<<PB1); // Делаем кнопку на порте подтянутой к ВЫСОКОМУ уровню, подключать к земле

    DDRB &= ~(1<<PB2); // Настраиваем кнопку на порте как вход
    PORTB |= (1<<PB2); // Делаем кнопку на порте подтянутой к ВЫСОКОМУ уровню, подключать к земле

    DDRB &= ~(1<<PB3); // Настраиваем кнопку на порте как вход
    PORTB |= (1<<PB3); // Делаем кнопку на порте подтянутой к ВЫСОКОМУ уровню, подключать к земле

    DDRB &= ~(1<<PB4); // Настраиваем кнопку на порте как вход
    PORTB |= (1<<PB4); // Делаем кнопку на порте подтянутой к ВЫСОКОМУ уровню, подключать к земле
}

void setupPoserSaveRegisters(){
    // В регистр энергосбережения записываем PRTIM0 (отключение счетчика) и PRADC (отключение ADC преобразователя)
    //PRR = (1<<PRTIM0) | (1<<PRADC);

    // В регистр энергосбережения PRADC (отключение только ADC преобразователя)
    PRR = (1<<PRADC);
    PRR &= ~(1<<PRTIM0);

    // Отключение AЦП
    ADCSRA &= ~(1 << ADEN);

    // Отключаем компаратор
    ACSR |= (1 << ACD);
}

void setupInterrupts(){
    // Настройка прерываний для кнопки
    GIMSK |= (1<<PCIE); // Разрешаем внешние прерывания PCINT0, работают толкьо на изменение состояний
    PCMSK |= (1<<PB1);  // Разрешаем по маске прерывания на ноге PB1
    //MCUCR |= (1<<ISC01)  // Прерывания будут при падении уровня, поддерживается только у прерывания INT0, не у PCINT (ISC01, ISC00 биты для настроек)
    //MCUCR &= ~(1<<ISC01) // Прерывания будут при падении уровня, поддерживается только у прерывания INT0, не у PCINT (ISC01, ISC00 биты для настроек)
    SREG |= (1<<SREG_I); // Разрешаем прерывания, sei();
}

void setupSleepMode(){
    // Включаем полный режим сна, прерывание будет по внешним прерываниям
    MCUCR |= (1<<SM1);  // Включаем power down mode, set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    MCUCR &= ~(1<<SM0); // Включаем power down mode, set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    MCUCR |= (1<<SE);   // Включаем режим сна, sleep_enable();
}

bool isButtonPressedUpNoChatter(char port){
    bool buttonPressed = ((PINB & (1 << port)) == 0); // Если низкий уровень - то кнопка нажата
    if (buttonPressed){
        _delay_ms(10); // Ждем чтобы избавиться от дребезга
        buttonPressed = ((PINB & (1 << port)) == 0); // Если низкий уровень - то кнопка нажата
    }
    return buttonPressed;
}

void enablePWMOut(){
    // ЗАПУСКАЕМ таймер для ШИМ на ножке PB0
    // 0x02, предделитель тактовой частоты CLK/8 (биты CS02, CS01, CS00)
    TCCR0B &= ~(1<<CS02);
    TCCR0B |= (1<<CS01);
    TCCR0B &= ~(1<<CS00);
    // Настраиваем режим быстрого шим (биты WGM01, WGM00)
    TCCR0A |= (1<<WGM01) | (1<<WGM00);
    // Настраиваем неинверсный режим (биты COM0A1, COM0A0) (может лучше инверсный?)
    TCCR0A |= (1<<COM0A1);
    TCCR0A &= ~(1<<COM0A0);

    // Обнуление счетчиков
    TCNT0 = 0; // Начальное значение счётчика
    OCR0A = 0; // Регистр совпадения A, ножка PB0
}

void disablePWMOut(){
    // ОТКЛЮЧАЕМ таймер для ШИМ на ножке PB0
    // 0x02, предделитель тактовой частоты CLK/8 (биты CS02, CS01, CS00)
    TCCR0B &= ~(1<<CS02);
    TCCR0B &= ~(1<<CS01);
    TCCR0B &= ~(1<<CS00);
    // Вырубаем режим быстрого шим (биты WGM01, WGM00)
    TCCR0A &= ~(1<<WGM01)
    TCCR0A &= ~(1<<WGM00);
    // Вырубаем неинверсный режим (биты COM0A1, COM0A0) (может лучше инверсный?)
    TCCR0A &= ~(1<<COM0A1);
    TCCR0A &= ~(1<<COM0A0);
}

int main(void) {
    // Отключение WatchDog
    wdt_disable();

    // Выставить порты в конкретное состояние InputPullup, чтобы по ним не происходили прерывания
    initialSetupOutPorts()

    // Настраиваем регистры энергосбереженияэ
    setupPoserSaveRegisters();

    // Пины кнопок
    DDRB &= ~(1<<PB1); // Настраиваем кнопку на порте PB1 как вход
    PORTB |= (1<<PB1); // Делаем кнопку на порте PB1 подтянутой к ВЫСОКОМУ уровню, подключать к земле

    // Пин светодиода
    DDRB |= (1<<PB0); // Настраиваем выход PB0 как выход
    PORTB &= ~(1<<PB0); // Выход на порте PB0 выключен

    // Настройка прерываний для кнопки
    setupInterrupts();

    // Включаем полный режим сна, прерывание будет по внешним прерываниям
    setupSleepMode();

    // Обнуление переменных
    keyInterrupt = false;
    outEnabled = false;

    // Главный цикл
    while(1) {
        // Включаем сон
        MCUCR |= (1<<SE);   // Включаем режим сна, sleep_enable();

        // Просто перекидываем процессор в сон, пробуждение по прерыванию
        asm("sleep"); // sleep_cpu();

        // Если было прерывание - обрабатываем его
        if (keyInterrupt){
            bool buttonPressed = isButtonPressedUpNoChatter(PB1);
            if (buttonPressed){

                // Включение или выключение
                if (outEnabled == false){
                    // Низкий уровень на пине PB0
                    PORTB &= ~(1<<PB0);

                    // ЗАПУСКАЕМ таймер для ШИМ на ножке PB0
                    enablePWMOut();

                    // Нарастание яркости
                    const float fadeInTimeSec = 1.0f;
                    const short delayMsValue = static_cast<short>(fadeInTimeSec / 255.0f * 1000.0f);
                    do {
                        OCR0A++; // Увеличиваем значение, с которым происходит сравнение счетчика
                        _delay_ms(delayMsValue);
                    } while(OCR0A < 255);

                    // ОТКЛЮЧАЕМ таймер для ШИМ на ножке PB0
                    disablePWMOut();

                    // Высокий уровень на пине PB0 постоянно
                    PORTB |= (1<<PB0);
                    outEnabled = true;
                }else{
                    // Низкий уровень на пине PB0
                    PORTB &= ~(1<<PB0);
                    outEnabled = false;
                }
            }
            
            keyInterrupt = false;
        }
    }

    return 0;
}
