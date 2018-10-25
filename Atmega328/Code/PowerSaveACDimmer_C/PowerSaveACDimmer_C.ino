// http://www.count-zero.ru/2016/attiny13/
// http://osboy.ru/blog/microcontrollers/attiny13-switch.html
// http://1io.ru/articles/microcontroller/attiny13/
// http://we.easyelectronics.ru/AVR/avr-power-management-ili-kak-pravilno-spat.html
// http://avr.ru/beginer/understand/sleep_mode
// http://osboy.ru/blog/microcontrollers/attiny13-pwm.html
// http://www.customelectronics.ru/avr-apparatnyiy-shim-mikrokontrollera/


#define F_CPU 1600000UL  // Частота 16.0Mhz
/*#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>*/


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

#define MICROSECONDS_PER_INTERRUPT (500)
#define MS_TO_INTERRUPTS(MS) (MS*2)
#define INTERRUPTS_TO_MS(INTERRUPTS) (INTERRUPTS/2)

const unsigned short blinkONDuration = 150;
const unsigned short blinkOFFDuration = 150;
const unsigned char MODES_POWERS_COUNT = 5;
const unsigned char STEP_VALUE = 20;

volatile bool hasACInterrupt = false;
volatile bool hasKeyInterrupt = false;
volatile unsigned int timerInterrupts = 0;

unsigned int disabledEndTime = 0;
unsigned int enabledEndTime = 0;
unsigned int buttonCheckTime = 0;
unsigned int blinkStartTime = 0;
unsigned int blinkEndTime = 0;
unsigned char blinksLeft = 0;
unsigned char powerMode = 0;
bool buttonInProcess = false;


// Обработчик прерывания INT0, доступен на ноге PD2
// Также по прерыванию происходит пробуждение микроконтроллера
ISR(INT0_vect) {
    //PORTB &= ~(1<<PD2); // Выход на порте PB0 сразу же должен быть выключен чтобы не активировать новую полуволну сразу, раскомментить в случае проблем
    hasACInterrupt = true;
}

// Обработчик прерывания PCINT0, доступно на любых ножках, срабатывает на изменение уровня
// Также по прерыванию происходит пробуждение микроконтроллера
ISR(PCINT0_vect) {
    hasKeyInterrupt = true;
}

// Обработчик прерывания переполнения таймера
/*ISR(TIM0_OVF_vect) {
    timerInterrupts++;
}*/

// Обработчик прерывания по достижению таймером конкретного значения
ISR(TIM0_COMPA_vect) {
    // За одну миллисекунду у нас будет 5 прерываний
    //TCNT0 = 0; // Обнуляем счетчик, чтобы отсчет шел заново - сейчас таймер настроен так, что сброс счетчика происходит автоматически, вручную не нужно
    timerInterrupts++;
}

void watchdogOff() {
    asm("wdr"); //__watchdog_reset();
    /* Clear WDRF in MCUSR */
    MCUSR &= ~(1<<WDRF);
    /* Write logical one to WDCE and WDE */
    /* Keep old prescaler setting to prevent unintentional time-out */
    WDTCSR |= (1<<WDCE) | (1<<WDE);
    /* Turn off WDT */
    WDTCSR = 0x00;
}

// Выставить порты в конкретное состояние InputPullup, чтобы по ним не происходили прерывания
void initialSetupOutPorts(){
    // Пин выхода PB0
    DDRB |= (1<<PB0); // Настраиваем выход PB0 как выход
    PORTB &= ~(1<<PB0); // Выход на порте PB0 выключен

    // Пин детектора нуля PD2
    DDRD &= ~(1<<PD2); // Настраиваем детектор нуля на порте PD2 как вход
    PORTD |= (1<<PD2); // Делаем кнопку на порте PD2 подтянутой к ВЫСОКОМУ уровню, подключать к земле

    // Пины кнопки на порте PB1
    DDRB &= ~(1<<PB1); // Настраиваем кнопку на порте PB2 как вход
    PORTB |= (1<<PB1); // Делаем кнопку на порте PB2 подтянутой к ВЫСОКОМУ уровню, подключать к земле

    // Пин выхода PB3
    DDRB |= (1<<PB0); // Настраиваем выход PB0 как выход
    PORTB &= ~(1<<PB0); // Выход на порте PB0 выключен
    
    // Настраиваем остальные порты для энергосбережения
    // TODO: !!!
}

void setupPowerSaveRegisters(){
    // Отключение TWI
    //PRR |= (1<<PRTWI);

    // В регистр энергосбережения записываем отключение ADC преобразователя
    PRR |= (1<<PRADC);

    // Отключение счетчика 0
    //PRR &= ~(1<<PRTIM0);

    // Отключение счетчика 1
    PRR |= (1<<PRTIM1);

    // Отключение счетчика 2
    PRR |= (1<<PRTIM2);

    // Отключение SPI интерфейса
    //PRR |= (1<<PRSPI);

    // Отключение UART
    //PRR |= (1<<PRUSART0);

    // Отключение AЦП
    ADCSRA = ~(1 << ADEN);

    // Отключаем компаратор
    ACSR |= (1 << ACD);
}

void setupMillisTimer(){
    // Настройка таймера
    // Включаем вызов прерывания у timer0 при переполнении
    //TIMSK0 &= ~((1<<TOIE0); // Прерывание по переполнению отключено
    TIMSK0 |= (1<<OCIE0A);  // Вызовется прерывание по компаратору при достижении 120 компаратор A
    //TIMSK0 &= ~(1<<OCIE0B);  // Прерывание по компаратору B отключено
    
    // Настраиваем, чтобы при достижении конкретного значения в OCR0A таймер сбросился сам, и не надо было бы в прерывании вручную обнулять счетчик
    //TCCR0B &= ~(1<<WGM02);
    TCCR0A |= (1<<WGM01);
    //TCCR0A &= ~(1<<WGM00);

    // Предделитель 64
    // Each timer tick is 1/(16.0MHz/64) = 4us
    //TCCR0B &= ~(1<<CS02);
    TCCR0B |= (1<<CS01);
    TCCR0B &= ~(1<<CS00); 
    
    // Настраиваем количество тиков при котором можно было бы настроить прерывание компаратора с обнулением счетчика внутри
    // при делителе 1: 1/(16.0MHz/64) = 4us каждый тик, 250 тиков - 1ms
    TCCR0B |= (1<<FOC0A);  // Включаем сравнение с маской (вроде бы работает и без этого)
    OCR0A = 125; // Маска совпадения значения, 2 прерывания в 1ms (надо ли 125-1?)

    // Обнуление счетчика
    TCNT0 = 0; // Начальное значение счётчика
}

/*void enableMillisTimer(){
    // Предделитель 1
    // Each timer tick is 1/(1.2MHz/1) = 0.8333333333333333us
    TCCR0B &= ~(1<<CS02);
    TCCR0B &= (1<<CS01);
    TCCR0B |= ~(1<<CS00); 

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
}*/

// Настройка прерываний INT0 для детектирования нуля
void setupACInterrupts(){
    // Разрешаем внешние прерывания на INT0
    EIMSK |= (1<<INT0); 
    // Прерывания будут при падении уровня, поддерживается только у прерывания INT0, не у PCINT (ISC01, ISC00 биты для настроек)
    EICRA |= (1<<ISC01); 
    EICRA &= ~(1<<ISC00); 
}

// Настройка прерываний для кнопки
void setupButtonInterrupts(){
    PCICR |= (1<<PCIE0); // Разрешаем внешние прерывания PCINT0-7, работают толкьо на изменение состояний
    PCMSK0 |= (1<<PCINT1);  // Разрешаем по маске прерывания на ноге PB1
}

void enableInterrupts(){
    SREG |= (1<<SREG_I); // Разрешаем прерывания, sei();
}

void setupSleepMode(){
    // Настройки режима сна, режим SLEEP_MODE_IDLE, чтобы работали таймеры?
    SMCR &= ~(1<<SM2); // Включаем idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
    SMCR &= ~(1<<SM1); // Включаем idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
    SMCR &= ~(1<<SM0); // Включаем idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
}

// Запись в постоянную память из документации
void EEPROM_write(unsigned char ucAddress, unsigned char ucData) {
    // Ожидаем завершения предыдущей записи
    while(EECR & (1<<EEPE));
    // Выставляем режим записи (Можно и режим записи и чтения атомарного)
    EECR |= (1<<EEPM1);
    EECR &= ~(1<<EEPM0);
    // Записываем адреса и данные в регистры
    EEARL = ucAddress; // EEARH - старший байт адреса
    EEDR = ucData;
    // Разрешаем запись
    EECR |= (1<<EEMPE);
    // Запускаем запись
    EECR |= (1<<EEPE);
}

// Чтение из постоянной памяти, код из документации
unsigned char EEPROM_read(unsigned char ucAddress) {
    // Ожидаем завершения предыдущей записи
    // while(EECR & (1<<EEPE)); // Чтение происходит один раз - не нужно проверять
    // Выставляем режим чтения (Можно и режим записи и чтения атомарного)
    EECR &= ~(1<<EEPM1);
    EECR |= (1<<EEPM0);
    // Выставляем регистр адреса
    EEARL = ucAddress; // EEARH - старший байт адреса
    // Запускаем операцию чтения
    EECR |= (1<<EERE);
    // Возвращаем прочитанные данные
    return EEDR;
}

void clearTimedValues(){
    timerInterrupts = 0;
    disabledEndTime = 0;
    enabledEndTime = 0;
    buttonCheckTime = 0;
    blinkStartTime = 0;
    blinkEndTime = 0;
    blinksLeft = 0;
    buttonInProcess = false;
}

void setup(){
    // Обнуление переменных
    hasACInterrupt = false;
    hasKeyInterrupt = false;
    clearTimedValues();

    // Прочитаем последний сохраненный режим
    powerMode = (EEPROM_read(0) % MODES_POWERS_COUNT); // Чтобы запустился предыдущий уровень, eeprom_read_byte((uint8_t*)0);

    // Отключение WatchDog
    watchdogOff(); //wdt_disable();

    // Выставить порты в конкретное состояние InputPullup, чтобы по ним не происходили прерывания
    initialSetupOutPorts();

    // Настраиваем регистры энергосбереженияэ
    setupPowerSaveRegisters();

    // Настраиваем режим сна, пробуждение будет по внешним прерываниям и таймерам
    setupSleepMode();

    // Настройка счетчика времени
    setupMillisTimer();

    // Настройка прерываний детектирования нуля
    setupACInterrupts();

    // Настройка прерывания для кнопки
    setupButtonInterrupts();

    // Разрешаем обработку прерываний
    enableInterrupts();
}

void loop(){
    // Пока есть было прерывания - обрабатываем
    if(hasACInterrupt){
        const char powerACValue = STEP_VALUE*powerMode+STEP_VALUE;
        const unsigned short zeroCrossPeriodTime = MS_TO_INTERRUPTS(1000/50/2); // Период между нулями - 10ms
        const unsigned short enabledDuration = powerACValue * zeroCrossPeriodTime / 100;
        const unsigned short disabledDuration = zeroCrossPeriodTime - enabledDuration;
        const unsigned short disableOutrun = 800/MICROSECONDS_PER_INTERRUPT; // На сколько микросекунд до конца полуволны закроется выход для следующего срабатывания

        // Время, когда сигнал активен
        disabledEndTime = timerInterrupts + disabledDuration;
        // Деактивируем за 1 ms до конца полуволны
        enabledEndTime = disabledEndTime + enabledDuration - disableOutrun;
        enabledEndTime = max(enabledEndTime, disabledEndTime);
        
        hasACInterrupt = false;
    }

    // Включаем выход если настало время
    if (disabledEndTime && (timerInterrupts >= disabledEndTime)){
        // Выход на порте PB0 включен
        PORTB |= (1<<PB0);
        // Обнуляем переменные диммера
        disabledEndTime = 0;
    }
    
    // Выключаем выход если настало время
    if (enabledEndTime && (timerInterrupts >= enabledEndTime)){
        // Выход на порте PB0 выключен
        PORTB &= ~(1<<PB0);
        // Обнуляем переменные диммера
        enabledEndTime = 0;
    }

    // Если было прерывание кнопки - обрабатываем его
    if (hasKeyInterrupt){
        // Если низкий уровень - то кнопка нажата
        if(buttonInProcess == false){
            const bool buttonPressed = ((PINB & (1 << PB1)) == 0);
            if (buttonPressed){
                buttonCheckTime = timerInterrupts + MS_TO_INTERRUPTS(25); //  Окончательную проверку дребезга сделаем через 25 миллисекунд
                buttonInProcess = true;
            }
        }
        hasKeyInterrupt = false;
    }

    // Выполняем обработку антидребезга
    if (buttonCheckTime && (timerInterrupts >= buttonCheckTime)){
        const bool buttonPressed = ((PINB & (1 << PB1)) == 0); // Если низкий уровень - то кнопка нажата
        if (buttonPressed){
            // Выбираем новый режим
            powerMode = (powerMode + 1) % MODES_POWERS_COUNT;

            // Сохраняем режим
            // TODO: Долгая операция? Перенести в цикл очередного запуска полуволны?
            EEPROM_write(0, powerMode); //eeprom_write_byte((uint8_t*)0, powerMode);

            // Выход на порте PB2 выключен
            PORTB &= ~(1<<PB2);
            
            // Планируем следующий блинк
            blinksLeft = powerMode+1;
            blinkStartTime = timerInterrupts + MS_TO_INTERRUPTS(blinkONDuration);
            blinkEndTime = blinkStartTime + MS_TO_INTERRUPTS(blinkOFFDuration);
        }

        buttonInProcess = false;
        buttonCheckTime = 0;
    }

    // Запуск свечения светодиода
    if (blinkStartTime && (timerInterrupts >= blinkStartTime)){
        // Выход на порте PB3 включен
        PORTB |= (1<<PB2);
        // Обнуляем переменнeю времени
        blinkStartTime = 0;
    }
    
    // Выключаем выход если настало время
    if (blinkEndTime && (timerInterrupts >= blinkEndTime)){
        // Выход на порте PB3 выключен
        PORTB &= ~(1<<PB2);
        // Обнуляем переменные диммера
        blinkEndTime = 0;
        // Планируем следующий блинк
        blinksLeft--;
        if(blinksLeft > 0){
            blinkStartTime = timerInterrupts + MS_TO_INTERRUPTS(blinkONDuration);
            blinkEndTime = blinkStartTime + MS_TO_INTERRUPTS(blinkOFFDuration);
        }
    }
    
    // Сброс против переполнений каждые 5 минут
    const unsigned long maxVal = MS_TO_INTERRUPTS(1000*60*5);
    if (timerInterrupts > maxVal){
        clearTimedValues();
        
        // Выход на порте PB0 выключен
        PORTB &= ~(1<<PB0);
        // Выход на порте PB3 выключен
        PORTB &= ~(1<<PB2);
    }
    
    // Если ничего не запланировано - сбрасываем значения, чтобы не словить переполнение
    // Позволяет использовать short переменные для отсчета времени вместо int, short типа хватает на 55 минут, поэтому однозначно сбросится
    /*if (!disabledEndTime && !enabledEndTime && !buttonCheckTime && !blinkStartTime && !blinkEndTime){
        timerInterrupts = 0;
    }*/

    // Проверяем, не было ли новых прерываний в процессе работы итерации цикла
    if(!hasACInterrupt && !hasKeyInterrupt){
        // Просто перекидываем процессор в сон, пробуждение по любому прерыванию
        SMCR |= (1<<SE);   // Включаем режим сна, sleep_enable();
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