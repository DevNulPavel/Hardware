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

// За одну миллисекунду у нас будет 150 прерываний
#define MS_TO_INTERRUPTS(MS) MS*150

const unsigned char MODES_POWERS_COUNT = 5;
const char MODES_POWERS[] = {
    20, // Процентов
    40, // Процентов
    60, // Процентов
    80, // Процентов
    100 // Процентов
};

volatile bool hasACInterrupt = false;
volatile bool hasKeyInterrupt = false;
volatile unsigned int timerInterrupts = 0;

bool outEnabled = false;
unsigned int disabledEndTime = 0;
unsigned int enabledEndTime = 0;
unsigned int buttonCheckTime = 0;
unsigned char powerMode = 0;


// Обработчик прерывания INT0, доступен на ноге PB1
// Также по прерыванию происходит пробуждение микроконтроллера
ISR(INT0_vect) {
    hasACInterrupt = true;
}

// Обработчик прерывания PCINT0, доступно на любых ножках, срабатывает на изменение уровня
// Также по прерыванию происходит пробуждение микроконтроллера
ISR(PCINT0_vect) {
    hasKeyInterrupt = true;
}

// Обработчик прерывания переполнения таймера
ISR(TIM0_OVF_vect) {
    // За одну миллисекунду у нас будет 150 прерываний
    timerInterrupts++;
}

// Выставить порты в конкретное состояние InputPullup, чтобы по ним не происходили прерывания
void initialSetupOutPorts(){
    //DDRB &= ~(1<<PB0); // Настраиваем кнопку на порте как вход
    //PORTB |= (1<<PB0); // Делаем кнопку на порте подтянутой к ВЫСОКОМУ уровню, подключать к земле

    //DDRB &= ~(1<<PB1); // Настраиваем кнопку на порте как вход
    //PORTB |= (1<<PB1); // Делаем кнопку на порте подтянутой к ВЫСОКОМУ уровню, подключать к земле

    //DDRB &= ~(1<<PB2); // Настраиваем кнопку на порте как вход
    //PORTB |= (1<<PB2); // Делаем кнопку на порте подтянутой к ВЫСОКОМУ уровню, подключать к земле

    DDRB &= ~(1<<PB3); // Настраиваем кнопку на порте как вход
    PORTB |= (1<<PB3); // Делаем кнопку на порте подтянутой к ВЫСОКОМУ уровню, подключать к земле

    DDRB &= ~(1<<PB4); // Настраиваем кнопку на порте как вход
    PORTB |= (1<<PB4); // Делаем кнопку на порте подтянутой к ВЫСОКОМУ уровню, подключать к земле
}

void setupPowerSaveRegisters(){
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
    OCR0A = 149; // Маска совпадения значения (надо ли указывать 150-1 ???)

    // Обнуление счетчика
    TCNT0 = 0; // Начальное значение счётчика

    // Обнуление времени
    timerInterrupts = 0;
}

void enableMillisTimer(){
    // Предделитель 1/8
    // Each timer tick is 1/(1.2MHz/8) = 6.666us
    TCCR0B &= ~(1<<CS02);
    TCCR0B |= (1<<CS01);
    TCCR0B &= ~(1<<CS00); 

    // Обнуление счетчика
    TCNT0 = 0;
    
    // Обнуление времени
    timerInterrupts = 0;
}

void disableMillisTimer(){
    // Отключаем таймер
    TCCR0B &= ~(1<<CS02);
    TCCR0B &= ~(1<<CS01);
    TCCR0B &= ~(1<<CS00); 
    // Обнуление счетчика
    TCNT0 = 0;
    // Обнуление времени
    timerInterrupts = 0;
}

void setupACInterrupts(){
    // Настройка прерываний INT0 для детектирования нуля
    GIMSK |= (1<<INT0); // Разрешаем внешние прерывания на INT0
    MCUCR |= (1<<ISC01);  // Прерывания будут при падении уровня, поддерживается только у прерывания INT0, не у PCINT (ISC01, ISC00 биты для настроек)
    MCUCR &= ~(1<<ISC00); // Прерывания будут при падении уровня, поддерживается только у прерывания INT0, не у PCINT (ISC01, ISC00 биты для настроек)
}

void setupButtonInterrupts(){
    // Настройка прерываний для кнопки
    GIMSK |= (1<<PCIE); // Разрешаем внешние прерывания PCINTx, работают толкьо на изменение состояний
    PCMSK |= (1<<PB2);  // Разрешаем по маске прерывания на ноге PB2
}

void enableInterrupts(){
    SREG |= (1<<SREG_I); // Разрешаем прерывания, sei();
}

void setupSleepMode(){
    // Настройки режима сна, режим SLEEP_MODE_IDLE, чтобы работали таймеры?
    MCUCR &= ~(1<<SM1); // Включаем idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
    MCUCR &= ~(1<<SM0); // Включаем idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
    MCUCR |= (1<<SE);   // Включаем режим сна, sleep_enable();
}

/*bool isButtonPressedUpNoChatter(char port){
    bool buttonPressed = ((PINB & (1 << port)) == 0); // Если низкий уровень - то кнопка нажата
    if (buttonPressed){
        _delay_ms(5); // Ждем чтобы избавиться от дребезга
        buttonPressed = ((PINB & (1 << port)) == 0); // Если низкий уровень - то кнопка нажата
    }
    return buttonPressed;
}*/

void setup(){
    // Отключение WatchDog
    wdt_disable();

    // Выставить порты в конкретное состояние InputPullup, чтобы по ним не происходили прерывания
    initialSetupOutPorts();

    // Настраиваем регистры энергосбереженияэ
    setupPowerSaveRegisters();

    // Пин выхода PB0
    DDRB |= (1<<PB0); // Настраиваем выход PB0 как выход
    PORTB &= ~(1<<PB0); // Выход на порте PB0 выключен

    // Пин детектора нуля PB1
    DDRB &= ~(1<<PB1); // Настраиваем детектор нуля на порте PB1 как вход
    PORTB |= (1<<PB1); // Делаем кнопку на порте PB1 подтянутой к ВЫСОКОМУ уровню, подключать к земле

    // Пины кнопки на порте PB2
    DDRB &= ~(1<<PB2); // Настраиваем кнопку на порте PB2 как вход
    PORTB |= (1<<PB2); // Делаем кнопку на порте PB2 подтянутой к ВЫСОКОМУ уровню, подключать к земле

    // Настраиваем режим сна, пробуждение будет по внешним прерываниям и таймерам
    setupSleepMode();

    // Настройка счетчика времени
    setupMillisTimer();

    // Запускаем таймер
    enableMillisTimer();

    // Настройка прерываний детектирования нуля
    setupACInterrupts();

    // Настройка прерывания для кнопки
    setupButtonInterrupts();

    // Разрешаем обработку прерываний
    enableInterrupts();

    // Обнуление переменных
    hasACInterrupt = false;
    hasKeyInterrupt = false;
    timerInterrupts = 0;
    outEnabled = false;
    disabledEndTime = 0;
    enabledEndTime = 0;
    buttonCheckTime = 0;
    powerMode = 0;
}

void loop(){
    // Сброс при переполнении каждый час
    const unsigned long maxVal = MS_TO_INTERRUPTS(1000*60*60*1);
    if (timerInterrupts > maxVal){
        timerInterrupts = 0;
        disabledEndTime = 0;
        enabledEndTime = 0;
        buttonCheckTime = 0;

        // Выход на порте PB0 выключен
        PORTB &= ~(1<<PB0);
    }
    
    // Пока есть было прерывания - обрабатываем
    if(hasACInterrupt){
        const char powerACValue = MODES_POWERS[powerMode];
        const unsigned short zeroCrossPeriodTime = MS_TO_INTERRUPTS(1000/50/2);
        const unsigned short enabledDuration = powerACValue * zeroCrossPeriodTime / 100;

        PORTB &= ~(1<<PB0); // Выход на порте PB0 выключен

        // Пропускать будем с середины полуволны и расширять ее
        disabledEndTime = timerInterrupts + zeroCrossPeriodTime/2 - enabledDuration/2;
        enabledEndTime = timerInterrupts + zeroCrossPeriodTime/2 + enabledDuration/2;

        hasACInterrupt = false;
    }

    // Если было прерывание кнопки - обрабатываем его
    if (hasKeyInterrupt){
        // Если низкий уровень - то кнопка нажата
        bool buttonPressed = ((PINB & (1 << PB2)) == 0);
        if (buttonPressed){
            buttonCheckTime = timerInterrupts + MS_TO_INTERRUPTS(5); //  Окончательную проверку дребезга сделаем через 5 миллисекунд
        }
        hasKeyInterrupt = false;
    }

    // Выполняем обработку антидребезга
    if (buttonCheckTime && (timerInterrupts > buttonCheckTime)){
        bool buttonPressed = ((PINB & (1 << PB2)) == 0); // Если низкий уровень - то кнопка нажата
        if (buttonPressed){
            outEnabled = !outEnabled;

            // Принудительно отключаем таймер для диммера
            if (outEnabled) {
                // Выбираем новый режим
                powerMode = (powerMode + 1) % MODES_POWERS_COUNT;
            }else{
                // Выход на порте PB0 выключен
                PORTB &= ~(1<<PB0);
            }
        }

        buttonCheckTime = 0;
    }

    // Включаем выход если настало время
    if (disabledEndTime && (timerInterrupts > disabledEndTime)){
        if (outEnabled) {
            // Выход на порте PB0 включен
            PORTB |= (1<<PB0);
        }else{
            // Выход на порте PB0 выключен
            PORTB &= ~(1<<PB0);
        }
    }
    
    // Выключаем выход если настало время
    if (enabledEndTime && (timerInterrupts > enabledEndTime)){
        // Выход на порте PB0 выключен
        PORTB &= ~(1<<PB0);

        // Обнуляем переменные диммера
        disabledEndTime = 0;
        enabledEndTime = 0;
    }

    // Проверяем, не было ли новых прерываний в процессе работы итерации цикла
    if(!hasACInterrupt && !hasKeyInterrupt){
        // Просто перекидываем процессор в сон, пробуждение по любому прерыванию
        MCUCR |= (1<<SE);   // Включаем режим сна, sleep_enable();
        asm("sleep"); // sleep_cpu();
    }
}

int main(void) {
    setup();
    while(1) {
        loop();
    }
    return 0;
}
