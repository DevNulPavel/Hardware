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


bool pwmEnabled = false;



// Обработчик окончания подсчета аналогового значения
ISR(ADC_vect) {
    //ADCSRA |= (1<<ADIF); // Данный бит выставляется, если было вызвано прерывание по окончанию рассчетов
    //while(ADCSRA & _BV(ADSC)); // Wait for conversion
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

void setupPowerSaveRegisters(){
    // В регистр энергосбережения записываем PRTIM0 (отключение счетчика) и PRADC (отключение ADC преобразователя)
    //PRR = (1<<PRTIM0) | (1<<PRADC);

    // В регистр энергосбережения PRADC (отключение только ADC преобразователя)
    //PRR = (1<<PRADC);
    //PRR &= ~(1<<PRTIM0);

    // В регистр энергосбережения PRADC (отключение только счетчика)
    //PRR &= ~(1<<PRADC);
    //PRR |= (1<<PRTIM0);

    // Отключение AЦП
    //ADCSRA &= ~(1 << ADEN);

    // Отключаем компаратор
    ACSR |= (1 << ACD);
}

void setupSleepMode(){
    // Настройки режима сна, режим SLEEP_MODE_IDLE, чтобы работали таймеры?
    MCUCR &= ~(1<<SM1); // Включаем idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
    MCUCR &= ~(1<<SM0); // Включаем idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
    MCUCR |= (1<<SE);   // Включаем режим сна, sleep_enable();
}

void setupADCInterrupts(){
    // Настройка прерываний ADC о завершении рассчета аналогового сигнала
    ADCSRA |= (1<<ADIE); // Включаем прерывание по завершению рассчета аналогового сигнала
}

void enableInterrupts(){
    SREG |= (1<<SREG_I); // Разрешаем прерывания, sei();
}

void setupAnalogInput(){
    // Настраиваем аналоговый вход 
    ADCSRA |= (1<<ADEN); // Включаем аналоговый вход
    // Prescaler to 4 --> F_CPU / 4, с помощью битов ADPS2, ADPS1, ADPS0
    ADCSRA &= ~(1<<ADPS2); 
    ADCSRA |= (1<<ADPS1);
    ADCSRA &= ~(1<<ADPS0);
     // Выбор входного канала, выбираем ADC1 (PB2) c помощью битов MUX1, MUX0
    ADMUX &= ~(1<<MUX1);
    ADMUX |= (1<<MUX0);
}

void enablePWMOut(){
    if (pwmEnabled == false){
        // ЗАПУСКАЕМ таймер для ШИМ на ножке PB0
        // 0x02, предделитель тактовой частоты CLK/8 (биты CS02, CS01, CS00)
        TCCR0B &= ~(1<<CS02);
        TCCR0B |= (1<<CS01);
        TCCR0B &= ~(1<<CS00);
        // Настраиваем режим быстрого шим (биты WGM01, WGM00)
        TCCR0A |= (1<<WGM01) | (1<<WGM00);
        // Настраиваем неинверсный режим для выхода PB0 (биты COM0A1, COM0A0) (может лучше инверсный?)
        TCCR0A |= (1<<COM0A1);
        TCCR0A &= ~(1<<COM0A0);
        // Настраиваем неинверсный режим для выхода PB1 (биты COM0B1, COM0B0) (может лучше инверсный?)
        //TCCR0A |= (1<<COM0B1);
        //TCCR0A &= ~(1<<COM0B0);

        // Обнуление счетчиков
        TCNT0 = 0; // Начальное значение счётчика
        OCR0A = 0; // Регистр совпадения A, ножка PB0
        //OCR0B = 0; // Регистр совпадения B, ножка PB1

        pwmEnabled = true;
    }
}

void disablePWMOut(){
    if (pwmEnabled){
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

        pwmEnabled = false;
    }
}

void setup(){
    // Отключение WatchDog
    wdt_disable();

    // Выставить порты в конкретное состояние InputPullup, чтобы по ним не происходили прерывания
    initialSetupOutPorts()

    // Настраиваем регистры энергосбереженияэ
    setupPowerSaveRegisters();

    // Включаем режим сна
    setupSleepMode();

    // Пин выхода PB0
    DDRB |= (1<<PB0); // Настраиваем выход PB0 как выход
    PORTB &= ~(1<<PB0); // Выход на порте PB0 выключен

    // Пин аналогового входа PB2
    DDRB &= ~(1<<PB2); // Настраиваем PB2 как вход
    PORTB &= ~(1<<PB2); // Делаем вход на порте PB2 на низкий уровень

    // Настройка прерываний окончания рассчета аналогового сигнала
    setupADCInterrupts();

    // Настраиваем аналоговый вход 
    setupAnalogInput();

    // Разрешаем обработку прерываний
    enableInterrupts();

    // ЗАПУСКАЕМ таймер для ШИМ на ножке PB0
    enablePWMOut();
}

void loop(){
    // TODO: Прерывания по изменению значения PCIN0

    // Инициализируем рассчет
    ADCSRA |= (1<<ADSC);
    
    // Просто перекидываем процессор в сон, пробуждение по любому прерыванию
    MCUCR |= (1<<SE);   // Включаем режим сна, sleep_enable();
    asm("sleep"); // sleep_cpu();
    
    //while(ADCSRA & _BV(ADSC)); // Wait for conversion

    // Проверяем, что вычисление завершилось
    if(ADCSRA & (1<<ADSC)){
        // Вычитываем значение, которое мы получили при аналоговом чтении
        uint8_t l = ADCL;
        uint8_t h = ADCH;
        unsigned int analogValue = (h << 8) | l;

        // Выставляем выходное значение как шим
        unsigned short digitalValue = static_cast<unsigned short>(analogValue*255/1024);
        if (digitalValue == 0){
            // Вырубаем ШИМ
            disablePWMOut();
            // Низкий уровень на пине PB0
            PORTB &= ~(1<<PB0);
        }else if(digitalValue == 255){
            // Вырубаем ШИМ
            disablePWMOut();
            // Высокий уровень на пине PB0 постоянно
            PORTB |= (1<<PB0);
        }else{
            // Включаем ШИМ
            enablePWMOut();
            // Ставим нужное значение ШИМ
            OCR0A = digitalValue; // Выставляем значение ШИМ в регистр
        }
    } 
}

int main(void) {
    setup();
    while(1) {
        loop();
    }
    return 0;
}
