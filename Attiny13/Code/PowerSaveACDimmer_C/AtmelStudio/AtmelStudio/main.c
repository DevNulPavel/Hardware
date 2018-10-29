// http://www.count-zero.ru/2016/attiny13/
// http://osboy.ru/blog/microcontrollers/attiny13-switch.html
// http://1io.ru/articles/microcontroller/attiny13/
// http://we.easyelectronics.ru/AVR/avr-power-management-ili-kak-pravilno-spat.html
// http://avr.ru/beginer/understand/sleep_mode
// http://osboy.ru/blog/microcontrollers/attiny13-pwm.html
// http://www.customelectronics.ru/avr-apparatnyiy-shim-mikrokontrollera/


#define F_CPU 9600000UL  // Частота 9.6Mhz
/*#include <util/delay.h>*/
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>


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

// Macroses
#define bool uint8_t
#define false 0
#define true 1
#define max(V1, V2) (V1 > V2 ? V1 : V2)

// Helper macroses
#define MICROSECONDS_PER_INTERRUPT (200)
#define MS_TO_INTERRUPTS(MS_VAL) (MS_VAL*5)
#define INTERRUPTS_TO_MS(INTERRUPTS) (INTERRUPTS/5)

// Flags
#define AC_INTERRUPT_BIT 1
#define KEY_INTERRUPT_BIT 2
#define BUTTON_IN_PROCESS_BIT 4

// Сonstants
const uint8_t blinkONDuration = 150;
const uint8_t blinkOFFDuration = 150;
const uint8_t MODES_POWERS_COUNT = 5;
const uint8_t STEP_VALUE = 20;

volatile uint8_t flags = 0;
volatile uint16_t timerInterrupts = 0;

uint16_t disabledEndTime = 0;
uint16_t enabledEndTime = 0;
uint16_t buttonCheckTime = 0;
uint16_t blinkStartTime = 0;
uint16_t blinkEndTime = 0;
uint8_t powerMode = 0;
uint8_t blinksLeft = 0;


// Обработчик прерывания INT0, доступен на ноге PB1
// Также по прерыванию происходит пробуждение микроконтроллера
ISR(INT0_vect) {
    //PORTB &= ~(1<<PB0); // Выход на порте PB0 сразу же должен быть выключен чтобы не активировать новую полуволну сразу, раскомментить в случае проблем
    SET_BIT(flags, AC_INTERRUPT_BIT);
}

// Обработчик прерывания PCINT0, доступно на любых ножках, срабатывает на изменение уровня
// Также по прерыванию происходит пробуждение микроконтроллера
ISR(PCINT0_vect) {
    SET_BIT(flags, KEY_INTERRUPT_BIT);
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

// Выставить порты в конкретное состояние InputPullup, чтобы по ним не происходили прерывания
void initialSetupOutPorts(){
    // Пин выхода PB0
    SET_BIT(DDRB, PB0); // Настраиваем порт PB0 как выход
    CLEAR_BIT(PORTB, PB0); // Выход на порте PB0 выключен

    // Пин детектора нуля PB1
    CLEAR_BIT(DDRB, PB1); // Настраиваем детектор нуля на порте PB1 как вход
    SET_BIT(PORTB, PB1);  // Делаем кнопку на порте PB1 подтянутой к ВЫСОКОМУ уровню, подключать к земле

    // Пины кнопки на порте PB2
    CLEAR_BIT(DDRB, PB2); // Настраиваем кнопку на порте PB2 как вход
    SET_BIT(PORTB, PB2);  // Делаем кнопку на порте PB2 подтянутой к ВЫСОКОМУ уровню, подключать к земле

    // Пин выхода PB3
    SET_BIT(DDRB, PB3);    // Настраиваем выход PB3 как выход
    CLEAR_BIT(PORTB, PB3); // Выход на порте PB3 выключен
    
    // Настраиваем остальные порты для энергосбережения
    CLEAR_BIT(DDRB, PB4); // Настраиваем кнопку на порте как вход
    SET_BIT(PORTB, PB4);  // Делаем кнопку на порте подтянутой к ВЫСОКОМУ уровню, подключать к земле
}

void setupPowerSaveRegisters(){
    // В регистр энергосбережения записываем PRTIM0 (отключение счетчика)
    //power_timer0_disable() //PRR |= (1<<PRTIM0);

    // В регистр энергосбережения PRADC (отключение только ADC преобразователя)
    power_adc_disable(); //PRR |= (1<<PRADC);
    //PRR &= ~(1<<PRTIM0);

    // Отключение AЦП
    CLEAR_BIT(ADCSRA, ADEN); //power_adca_disable();

    // Отключаем компаратор
    SET_BIT(ACSR, ACD); //power_aca_disable(); 
}

void setupMillisTimer(){
    // Настройка таймера
    // Включаем вызов прерывания у timer0 при переполнении
    //TIMSK0 &= ~((1<<TOIE0); // Прерывание по переполнению отключено
    SET_BIT(TIMSK0, OCIE0A);  // Вызовется прерывание по компаратору при достижении 240 компаратор A
    //TIMSK0 &= ~(1<<OCIE0B);  // Прерывание по компаратору B отключено
    
    // Настраиваем, чтобы при достижении конкретного значения в OCR0A таймер сбросился сам, и не надо было бы в прерывании вручную обнулять счетчик
    //TCCR0B &= ~(1<<WGM02);
    SET_BIT(TCCR0A, WGM01);
    //TCCR0A &= ~(1<<WGM00);

    // Предделитель 8
    // Each timer tick is 1/(9.6MHz/8) = 0.8333333333333333us
    //TCCR0B &= ~(1<<CS02);
    SET_BIT(TCCR0B, CS01);
    //TCCR0B &= ~(1<<CS00); 
    
    // Настраиваем количество тиков при котором можно было бы настроить прерывание компаратора с обнулением счетчика внутри
    // при делителе 1: 1/(9.6MHz/8) = 0.8333333333333333us каждый тик, 1200 тиков - 1ms
    //TCCR0B |= (1<<FOC0A);  // Force Output Compare A, Включаем сравнение с маской?? (вроде бы работает и без этого)
    OCR0A = 240; // Маска совпадения значения, 5 прерываний в 1ms (надо ли 240-1?)

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

void setupACInterrupts(){
    // Настройка прерываний INT0 для детектирования нуля
    SET_BIT(GIMSK, INT0);   // Разрешаем внешние прерывания на INT0
    SET_BIT(MCUCR, ISC01);  // Прерывания будут при падении уровня, поддерживается только у прерывания INT0, не у PCINT (ISC01, ISC00 биты для настроек)
    //MCUCR &= ~(1<<ISC00); // Прерывания будут при падении уровня, поддерживается только у прерывания INT0, не у PCINT (ISC01, ISC00 биты для настроек)
}

void setupButtonInterrupts(){
    // Настройка прерываний для кнопки
    SET_BIT(GIMSK, PCIE); // Разрешаем внешние прерывания PCINTx, работают толкьо на изменение состояний
    SET_BIT(PCMSK, PB2);  // Разрешаем по маске прерывания на ноге PB2
}

void enableInterrupts(){
    // Разрешаем прерывания
    //SREG |= (1<<SREG_I);
    sei(); 
}

void setupSleepMode(){
    // Настройки режима сна, режим SLEEP_MODE_IDLE, чтобы работали таймеры?
    set_sleep_mode(SLEEP_MODE_IDLE);
    //MCUCR &= ~(1<<SM1); // Включаем idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
    //MCUCR &= ~(1<<SM0); // Включаем idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
}

/*
// Запись в постоянную память из документации
void EEPROM_write(unsigned char ucAddress, unsigned char ucData) {
    // Ожидаем завершения предыдущей записи
    while(EECR & (1<<EEPE));
    // Выставляем режим записи и чтения атомарного
    EECR |= (1<<EEPM1);
    EECR |= (1<<EEPM0);
    // Записываем адреса и данные в регистры
    EEARL = ucAddress;
    EEDR = ucData;
    // разрешаем запись
    EECR |= (1<<EEMPE);
    // Запускаем запись
    EECR |= (1<<EEPE);
}

// Чтение из постоянной памяти, код из документации
unsigned char EEPROM_read(unsigned char ucAddress) {
    // Ожидаем завершения предыдущей записи
    while(EECR & (1<<EEPE)); // Чтение происходит один раз - не нужно проверять
    // Выставляем режим записи и чтения атомарного
    EECR |= (1<<EEPM1);
    EECR |= (1<<EEPM0);
    // Выставляем регистр адреса
    EEARL = ucAddress;
    // Запускаем операцию чтения
    EECR |= (1<<EERE);
    // Возвращаем прочитанные данные
    return EEDR;
}*/

void setup(){
    // Обнуление переменных
    flags = 0;
    timerInterrupts = 0;
    disabledEndTime = 0;
    enabledEndTime = 0;
    buttonCheckTime = 0;
    blinkStartTime = 0;
    blinkEndTime = 0;
    blinksLeft = 0;

    // Прочитаем последний сохраненный режим
    //powerMode = (eeprom_read_byte((uint8_t*)4) % MODES_POWERS_COUNT);//(EEPROM_read(0) % MODES_POWERS_COUNT); // Чтобы запустился предыдущий уровень, eeprom_read_byte((uint8_t*)0);
    powerMode = 4;
    
    // Отключение WatchDog
    wdt_disable();

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
    uint8_t oldSREG = SREG;
    cli();
    const uint16_t now = timerInterrupts;
    SREG = oldSREG;
    
    // Пока есть было прерывания - обрабатываем
    if(BIT_IS_SET(flags, AC_INTERRUPT_BIT)){
        const uint8_t powerACValue = STEP_VALUE*powerMode+STEP_VALUE;
        const uint16_t zeroCrossPeriodTime = MS_TO_INTERRUPTS(10); // 1000/50/2, Период между нулями - 10ms
        const uint16_t enabledDuration = powerACValue * zeroCrossPeriodTime / 100;
        const uint16_t disabledDuration = zeroCrossPeriodTime - enabledDuration;
        const uint16_t disableOutrun = 800/MICROSECONDS_PER_INTERRUPT; // На сколько микросекунд до конца полуволны закроется выход для следующего срабатывания

        // Время, когда сигнал активен
        disabledEndTime = now + disabledDuration;
        // Деактивируем за 1 ms до конца полуволны
        enabledEndTime = disabledEndTime + enabledDuration - disableOutrun;
        //enabledEndTime = max(enabledEndTime, disabledEndTime);
        
        CLEAR_BIT(flags, AC_INTERRUPT_BIT);
    }

    // Включаем выход если настало время
    if ((disabledEndTime > 0) && (now >= disabledEndTime)){
        // Выход на порте PB0 включен
        SET_BIT(PORTB, PB0);
        // Обнуляем переменные диммера
        disabledEndTime = 0;
    }
    
    // Выключаем выход если настало время
    if ((enabledEndTime > 0) && (now >= enabledEndTime)){
        // Выход на порте PB0 выключен
        CLEAR_BIT(PORTB, PB0);
        // Обнуляем переменные диммера
        enabledEndTime = 0;
    }

    // Если было прерывание кнопки - обрабатываем его
    if (BIT_IS_SET(flags, KEY_INTERRUPT_BIT)){
        // Если низкий уровень - то кнопка нажата
        if(BIT_IS_SET(flags, BUTTON_IN_PROCESS_BIT)){
            const bool buttonPressed = ((PINB & (1 << PB2)) == 0);
            if (buttonPressed){
                buttonCheckTime = now + MS_TO_INTERRUPTS(25); //  Окончательную проверку дребезга сделаем через 25 миллисекунд
                BIT_IS_SET(flags, BUTTON_IN_PROCESS_BIT);
            }
        }

        CLEAR_BIT(flags, KEY_INTERRUPT_BIT);
    }

    // Выполняем обработку антидребезга
    if ((buttonCheckTime > 0) && (now >= buttonCheckTime)){
        const bool buttonPressed = (BIT_IS_SET(PINB, PB2) == 0); // Если низкий уровень - то кнопка нажата
        if (buttonPressed){
            // Выбираем новый режим
            powerMode = (powerMode + 1) % MODES_POWERS_COUNT;

            // Сохраняем режим
            // TODO: Долгая операция? Перенести в цикл очередного запуска полуволны?
            //EEPROM_write(0, powerMode); //eeprom_write_byte((uint8_t*)0, powerMode);
            //eeprom_write_byte((uint8_t*)4, powerMode);
            
            // Планируем следующий блинк
            blinksLeft = powerMode+1;
            blinkStartTime = now + MS_TO_INTERRUPTS(blinkONDuration);
            blinkEndTime = blinkStartTime + MS_TO_INTERRUPTS(blinkOFFDuration);
        }
        
        CLEAR_BIT(flags, BUTTON_IN_PROCESS_BIT);
        buttonCheckTime = 0;
    }

    // Запуск свечения светодиода
    if ((blinkStartTime > 0) && (now >= blinkStartTime)){
        // Выход на порте PB3 включен
        SET_BIT(PORTB, PB3);
        // Обнуляем переменнeю времени
        blinkStartTime = 0;
    }
    
    // Выключаем выход если настало время
    if ((blinkEndTime > 0) && (now >= blinkEndTime)){
        // Выход на порте PB3 выключен
        CLEAR_BIT(PORTB, PB3);
        // Обнуляем переменные диммера
        blinkEndTime = 0;
        // Планируем следующий блинк
        blinksLeft--;
        if(blinksLeft > 0){
            blinkStartTime = now + MS_TO_INTERRUPTS(blinkONDuration);
            blinkEndTime = blinkStartTime + MS_TO_INTERRUPTS(blinkOFFDuration);
        }
    }
    
    // Сброс против переполнений каждые 5 секунд
    const uint16_t maxValOverflow = MS_TO_INTERRUPTS(5000); // 1000*60
    if (now > maxValOverflow){
        uint8_t oldSREG = SREG;
        cli();
        const uint16_t nowLocal = timerInterrupts;
        timerInterrupts = 0;
        SREG = oldSREG;
        
        disabledEndTime %= nowLocal;
        enabledEndTime %= nowLocal;
        buttonCheckTime %= nowLocal;
        blinkStartTime %= nowLocal;
        blinkEndTime %= nowLocal;
    }
    
    // Если ничего не запланировано - сбрасываем значения, чтобы не словить переполнение
    // Позволяет использовать short переменные для отсчета времени вместо int, short типа хватает на 55 минут, поэтому однозначно сбросится
    /*if (!disabledEndTime && !enabledEndTime && !buttonCheckTime && !blinkStartTime && !blinkEndTime){
        timerInterrupts = 0;
    }*/

    // Проверяем, не было ли новых прерываний в процессе работы итерации цикла
    if(BIT_IS_SET(flags, AC_INTERRUPT_BIT | KEY_INTERRUPT_BIT)){
        // Просто перекидываем процессор в сон, пробуждение по любому прерыванию
        sleep_enable(); // MCUCR |= (1<<SE);   // Включаем режим сна
        sleep_cpu(); // asm("sleep");
        sleep_disable(); // MCUCR &= ~(1<<SE);   // Отключаем режим сна
        //sei(); // Еще раз разрешаем прерывания
    }
}

int main(void) {
    setup();
    while(1) {
        loop();
    }
    return 0;
}
