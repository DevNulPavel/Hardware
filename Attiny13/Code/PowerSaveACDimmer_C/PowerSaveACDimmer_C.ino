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
volatile bool hasACInterrupt = false;
volatile unsigned long timerInterrupts = 0;
volatile unsigned long millis = 0;

unsigned long disabledEndTimeMs = 0;
unsigned long enabledEndTimeMs = 0;


// Обработчик прерывания INT0, доступен на ноге PB1
// Так же по прерыванию происходит пробуждение микроконтроллера
ISR(INT0_vect) {
    hasACInterrupt = true;
}

// Обработчик прерывания по таймеру
ISR(TIM0_OVF_vect) {
    // За одну миллисекунду у нас будет 150 прерываний
    if((++timerInterrupts % 150) == 0){
        timerInterrupts = 0;
        millis++;
    }
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

void setupMillisTimer(){
    // Настройка таймера
    // Включаем вызов прерывания у timer0 при переполнении
    TIMSK0 |= (1<<TOIE0);
    // Отключаем таймер
    TCCR0B &= ~(1<<CS02);
    TCCR0B &= ~(1<<CS01);
    TCCR0B &= ~(1<<CS00); 

    // Настраиваем количество тиков при котором произойдет прерывание,
    // при делителе 8: 1/(1.2MHz/8) = 6.666us каждый тик, 150 тиков - 1ms
    TCCR0B |= (1<<FOC0A);  // Включаем сравнение с маской
    OCR0A = 149; // Маска совпадения значения (надо ли указывать 150-1???)

    // Обнуление счетчика
    TCNT0 = 0; // Начальное значение счётчика
    // Обнуление количества прерываний
    timerInterrupts = 0;
    millis = 0;
}

void enableMillisTimer(){
    // Предделитель 1/8
    // Each timer tick is 1/(1.2MHz/8) = 6.666us
    TCCR0B &= ~(1<<CS02);
    TCCR0B |= (1<<CS01);
    TCCR0B &= ~(1<<CS00); 
    // Обнуление счетчика
    TCNT0 = 0;
    // Обнуление количества прерываний
    timerInterrupts = 0;
    millis = 0;
}

void disableMillisTimer(){
    // Отключаем таймер
    TCCR0B &= ~(1<<CS02);
    TCCR0B &= ~(1<<CS01);
    TCCR0B &= ~(1<<CS00); 
    // Обнуление счетчика
    TCNT0 = 0;
    // Обнуление количества прерываний
    timerInterrupts = 0;
    millis = 0;
}

void setupInterrupts(){
    // Настройка прерываний INT0 для детектирования нуля
    GIMSK |= (1<<INT0); // Разрешаем внешние прерывания на INT0
    MCUCR |= (1<<ISC01)  // Прерывания будут при падении уровня, поддерживается только у прерывания INT0, не у PCINT (ISC01, ISC00 биты для настроек)
    MCUCR &= ~(1<<ISC00) // Прерывания будут при падении уровня, поддерживается только у прерывания INT0, не у PCINT (ISC01, ISC00 биты для настроек)
    SREG |= (1<<SREG_I); // Разрешаем прерывания, sei();
}

void setupSleepMode(){
    // Настройки режима сна, режим SLEEP_MODE_IDLE, чтобы работали таймеры?
    MCUCR &= ~(1<<SM1); // Включаем idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
    MCUCR &= ~(1<<SM0); // Включаем idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
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

int main(void) {
    // Отключение WatchDog
    wdt_disable();

    // Выставить порты в конкретное состояние InputPullup, чтобы по ним не происходили прерывания
    initialSetupOutPorts()

    // Настраиваем регистры энергосбереженияэ
    setupPoserSaveRegisters();

    // Пин детектора нуля PB1
    DDRB &= ~(1<<PB1); // Настраиваем детектор нуля на порте PB1 как вход
    PORTB |= (1<<PB1); // Делаем кнопку на порте PB1 подтянутой к ВЫСОКОМУ уровню, подключать к земле

    // Пин выхода PB0
    DDRB |= (1<<PB0); // Настраиваем выход PB0 как выход
    PORTB &= ~(1<<PB0); // Выход на порте PB0 выключен

    // Настройка прерываний
    setupInterrupts();

    // Настройка счетчика времени
    setupMillisTimer();

    // Включаем полный режим сна, прерывание будет по внешним прерываниям
    setupSleepMode();

    // Обнуление переменных
    hasACInterrupt = false;
    timerInterrupts = 0;

    // Главный цикл
    while(1) {
        // Пока есть было прерывания - обрабатываем
        if(hasACInterrupt){
            hasACInterrupt = false;

            const float powerACValue = 0.4f; // 40%
            const float periodTime = 1.0f/50.0f*1000.0f;
            const float disableMsDuration = (1.0f - powerACValue) * periodTime;
            const float enableMsDuration = powerACValue * periodTime;

            PORTB &= ~(1<<PB0); // Выход на порте PB0 выключен

            // Запускаем счетчик миллисекунд
            enableMillisTimer();

            disabledEndTimeMs = millis + disableMsValue;
            enabledEndTimeMs = millis + disableMsDuration + enableMsDuration;
        }

        // Включаем выход если настало время
        if (millis > disabledEndTimeMs){
            PORTB |= (1<<PB0); // Выход на порте PB0 включен
        }
        
        // выключаем выход если настало время
        if (millis > enabledEndTimeMs){
            PORTB &= ~(1<<PB0); // Выход на порте PB0 выключен

            // Вырубаем счетчик миллисекунд
            disableMillisTimer();
        }

        // Пока есть было прерывания - обрабатываем
        if(!hasACInterrupt){
            // Просто перекидываем процессор в сон, пробуждение по любому прерыванию
            MCUCR |= (1<<SE);   // Включаем режим сна, sleep_enable();
            asm("sleep"); // sleep_cpu();
        }
    }

    return 0;
}
