// http://www.count-zero.ru/2016/attiny13/
// http://osboy.ru/blog/microcontrollers/attiny13-switch.html
// http://1io.ru/articles/microcontroller/attiny13/
// http://we.easyelectronics.ru/AVR/avr-power-management-ili-kak-pravilno-spat.html
// http://avr.ru/beginer/understand/sleep_mode
// http://osboy.ru/blog/microcontrollers/attiny13-pwm.html
// http://www.customelectronics.ru/avr-apparatnyiy-shim-mikrokontrollera/


#define F_CPU 9600000UL  // ������� 9.6Mhz
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

#define MICROSECONDS_PER_INTERRUPT (200)
#define MS_TO_INTERRUPTS(MS_VAL) (MS_VAL*5)
#define INTERRUPTS_TO_MS(INTERRUPTS) (INTERRUPTS/5)

#define bool uint8_t
#define false 0
#define true 1
#define max(V1, V2) (V1 > V2 ? V1 : V2)

const uint8_t blinkONDuration = 150;
const uint8_t blinkOFFDuration = 150;
const uint8_t MODES_POWERS_COUNT = 5;
const uint8_t STEP_VALUE = 20;

volatile bool hasACInterrupt = false;
volatile bool hasKeyInterrupt = false;
volatile uint16_t timerInterrupts = 0;

uint16_t disabledEndTime = 0;
uint16_t enabledEndTime = 0;
uint16_t buttonCheckTime = 0;
uint16_t blinkStartTime = 0;
uint16_t blinkEndTime = 0;
uint8_t powerMode = 0;
uint8_t blinksLeft = 0;
bool buttonInProcess = false;


// ���������� ���������� INT0, �������� �� ���� PB1
// ����� �� ���������� ���������� ����������� ����������������
ISR(INT0_vect) {
    //PORTB &= ~(1<<PB0); // ����� �� ����� PB0 ����� �� ������ ���� �������� ����� �� ������������ ����� ��������� �����, ������������� � ������ �������
    hasACInterrupt = true;
}

// ���������� ���������� PCINT0, �������� �� ����� ������, ����������� �� ��������� ������
// ����� �� ���������� ���������� ����������� ����������������
ISR(PCINT0_vect) {
    hasKeyInterrupt = true;
}

// ���������� ���������� ������������ �������
/*ISR(TIM0_OVF_vect) {
    timerInterrupts++;
}*/

// ���������� ���������� �� ���������� �������� ����������� ��������
ISR(TIM0_COMPA_vect) {
    // �� ���� ������������ � ��� ����� 5 ����������
    //TCNT0 = 0; // �������� �������, ����� ������ ��� ������ - ������ ������ �������� ���, ��� ����� �������� ���������� �������������, ������� �� �����
    timerInterrupts++;
}

// ��������� ����� � ���������� ��������� InputPullup, ����� �� ��� �� ����������� ����������
void initialSetupOutPorts(){
    // ��� ������ PB0
    DDRB |= (1<<PB0); // ����������� ����� PB0 ��� �����
    PORTB &= ~(1<<PB0); // ����� �� ����� PB0 ��������

    // ��� ��������� ���� PB1
    DDRB &= ~(1<<PB1); // ����������� �������� ���� �� ����� PB1 ��� ����
    PORTB |= (1<<PB1); // ������ ������ �� ����� PB1 ���������� � �������� ������, ���������� � �����

    // ���� ������ �� ����� PB2
    DDRB &= ~(1<<PB2); // ����������� ������ �� ����� PB2 ��� ����
    PORTB |= (1<<PB2); // ������ ������ �� ����� PB2 ���������� � �������� ������, ���������� � �����

    // ��� ������ PB3
    DDRB |= (1<<PB3); // ����������� ����� PB3 ��� �����
    PORTB &= ~(1<<PB3); // ����� �� ����� PB3 ��������
    
    // ����������� ��������� ����� ��� ����������������
    DDRB &= ~(1<<PB4); // ����������� ������ �� ����� ��� ����
    PORTB |= (1<<PB4); // ������ ������ �� ����� ���������� � �������� ������, ���������� � �����
}

void setupPowerSaveRegisters(){
    // � ������� ���������������� ���������� PRTIM0 (���������� ��������)
    //power_timer0_disable() //PRR |= (1<<PRTIM0);

    // � ������� ���������������� PRADC (���������� ������ ADC ���������������)
    power_adc_disable(); //PRR |= (1<<PRADC);
    //PRR &= ~(1<<PRTIM0);

    // ���������� A��
    ADCSRA &= ~(1 << ADEN); //power_adca_disable();

    // ��������� ����������
    ACSR |= (1 << ACD); //power_aca_disable(); 
}

void setupMillisTimer(){
    // ��������� �������
    // �������� ����� ���������� � timer0 ��� ������������
    //TIMSK0 &= ~((1<<TOIE0); // ���������� �� ������������ ���������
    TIMSK0 |= (1<<OCIE0A);  // ��������� ���������� �� ����������� ��� ���������� 240 ���������� A
    //TIMSK0 &= ~(1<<OCIE0B);  // ���������� �� ����������� B ���������
    
    // �����������, ����� ��� ���������� ����������� �������� � OCR0A ������ ��������� ���, � �� ���� ���� �� � ���������� ������� �������� �������
    //TCCR0B &= ~(1<<WGM02);
    TCCR0A |= (1<<WGM01);
    //TCCR0A &= ~(1<<WGM00);

    // ������������ 8
    // Each timer tick is 1/(9.6MHz/8) = 0.8333333333333333us
    //TCCR0B &= ~(1<<CS02);
    TCCR0B |= (1<<CS01);
    //TCCR0B &= ~(1<<CS00); 
    
    // ����������� ���������� ����� ��� ������� ����� ���� �� ��������� ���������� ����������� � ���������� �������� ������
    // ��� �������� 1: 1/(9.6MHz/8) = 0.8333333333333333us ������ ���, 1200 ����� - 1ms
    //TCCR0B |= (1<<FOC0A);  // Force Output Compare A, �������� ��������� � ������?? (����� �� �������� � ��� �����)
    OCR0A = 240; // ����� ���������� ��������, 5 ���������� � 1ms (���� �� 240-1?)

    // ��������� ��������
    TCNT0 = 0; // ��������� �������� ��������
}

/*void enableMillisTimer(){
    // ������������ 1
    // Each timer tick is 1/(1.2MHz/1) = 0.8333333333333333us
    TCCR0B &= ~(1<<CS02);
    TCCR0B &= (1<<CS01);
    TCCR0B |= ~(1<<CS00); 

    // ��������� ��������
    TCNT0 = 0;
    
    // ��������� �������
    timerInterrupts = 0;
}

void disableMillisTimer(){
    // ��������� ������
    TCCR0B &= ~(1<<CS02);
    TCCR0B &= ~(1<<CS01);
    TCCR0B &= ~(1<<CS00); 
    // ��������� ��������
    TCNT0 = 0;
    // ��������� �������
    timerInterrupts = 0;
}*/

void setupACInterrupts(){
    // ��������� ���������� INT0 ��� �������������� ����
    GIMSK |= (1<<INT0); // ��������� ������� ���������� �� INT0
    MCUCR |= (1<<ISC01);  // ���������� ����� ��� ������� ������, �������������� ������ � ���������� INT0, �� � PCINT (ISC01, ISC00 ���� ��� ��������)
    //MCUCR &= ~(1<<ISC00); // ���������� ����� ��� ������� ������, �������������� ������ � ���������� INT0, �� � PCINT (ISC01, ISC00 ���� ��� ��������)
}

void setupButtonInterrupts(){
    // ��������� ���������� ��� ������
    GIMSK |= (1<<PCIE); // ��������� ������� ���������� PCINTx, �������� ������ �� ��������� ���������
    PCMSK |= (1<<PB2);  // ��������� �� ����� ���������� �� ���� PB2
}

void enableInterrupts(){
    // ��������� ����������
    SREG |= (1<<SREG_I);
    sei(); 
}

void setupSleepMode(){
    // ��������� ������ ���, ����� SLEEP_MODE_IDLE, ����� �������� �������?
    set_sleep_mode(SLEEP_MODE_IDLE);
    //MCUCR &= ~(1<<SM1); // �������� idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
    //MCUCR &= ~(1<<SM0); // �������� idle mode, set_sleep_mode (SLEEP_MODE_IDLE);
}

/*
// ������ � ���������� ������ �� ������������
void EEPROM_write(unsigned char ucAddress, unsigned char ucData) {
    // ������� ���������� ���������� ������
    while(EECR & (1<<EEPE));
    // ���������� ����� ������ � ������ ����������
    EECR |= (1<<EEPM1);
    EECR |= (1<<EEPM0);
    // ���������� ������ � ������ � ��������
    EEARL = ucAddress;
    EEDR = ucData;
    // ��������� ������
    EECR |= (1<<EEMPE);
    // ��������� ������
    EECR |= (1<<EEPE);
}

// ������ �� ���������� ������, ��� �� ������������
unsigned char EEPROM_read(unsigned char ucAddress) {
    // ������� ���������� ���������� ������
    while(EECR & (1<<EEPE)); // ������ ���������� ���� ��� - �� ����� ���������
    // ���������� ����� ������ � ������ ����������
    EECR |= (1<<EEPM1);
    EECR |= (1<<EEPM0);
    // ���������� ������� ������
    EEARL = ucAddress;
    // ��������� �������� ������
    EECR |= (1<<EERE);
    // ���������� ����������� ������
    return EEDR;
}*/

void setup(){
    // ��������� ����������
    hasACInterrupt = false;
    hasKeyInterrupt = false;
	timerInterrupts = 0;    // Store timer0 overflow count
	disabledEndTime = 0;
	enabledEndTime = 0;
	buttonCheckTime = 0;
	blinkStartTime = 0;
	blinkEndTime = 0;
	blinksLeft = 0;
	buttonInProcess = false;

    // ��������� ��������� ����������� �����
    //powerMode = (eeprom_read_byte((uint8_t*)0) % MODES_POWERS_COUNT);//(EEPROM_read(0) % MODES_POWERS_COUNT); // ����� ���������� ���������� �������, eeprom_read_byte((uint8_t*)0);
	powerMode = 0;
	
    // ���������� WatchDog
    //wdt_disable();

    // ��������� ����� � ���������� ��������� InputPullup, ����� �� ��� �� ����������� ����������
    initialSetupOutPorts();

    // ����������� �������� �����������������
    setupPowerSaveRegisters();

    // ����������� ����� ���, ����������� ����� �� ������� ����������� � ��������
    setupSleepMode();

    // ��������� �������� �������
    setupMillisTimer();

    // ��������� ���������� �������������� ����
    setupACInterrupts();

    // ��������� ���������� ��� ������
    setupButtonInterrupts();

    // ��������� ��������� ����������
    enableInterrupts();
}

void loop(){
    uint8_t oldSREG = SREG; // Preserve old SREG value 
    cli();                  // Disable global interrupts
    const uint16_t now = timerInterrupts;    // Store timer0 overflow count
    SREG = oldSREG;         // Restore SREG
    
    // ���� ���� ���� ���������� - ������������
    if(hasACInterrupt){
        const uint8_t powerACValue = STEP_VALUE*powerMode+STEP_VALUE;
        const uint16_t zeroCrossPeriodTime = MS_TO_INTERRUPTS(10); // 1000/50/2, ������ ����� ������ - 10ms
        const uint16_t enabledDuration = powerACValue * zeroCrossPeriodTime / 100;
        const uint16_t disabledDuration = zeroCrossPeriodTime - enabledDuration;
        const uint16_t disableOutrun = 800/MICROSECONDS_PER_INTERRUPT; // �� ������� ����������� �� ����� ��������� ��������� ����� ��� ���������� ������������

        // �����, ����� ������ �������
        disabledEndTime = now + disabledDuration;
        // ������������ �� 1 ms �� ����� ���������
        enabledEndTime = disabledEndTime + enabledDuration - disableOutrun;
        //enabledEndTime = max(enabledEndTime, disabledEndTime);
        
        hasACInterrupt = false;
    }

    // �������� ����� ���� ������� �����
    if ((disabledEndTime > 0) && (now >= disabledEndTime)){
        // ����� �� ����� PB0 �������
        PORTB |= (1<<PB0);
        // �������� ���������� �������
        disabledEndTime = 0;
    }
    
    // ��������� ����� ���� ������� �����
    if ((enabledEndTime > 0) && (now >= enabledEndTime)){
        // ����� �� ����� PB0 ��������
        PORTB &= ~(1<<PB0);
        // �������� ���������� �������
        enabledEndTime = 0;
    }

    // ���� ���� ���������� ������ - ������������ ���
    if (hasKeyInterrupt){
        // ���� ������ ������� - �� ������ ������
        if(buttonInProcess == false){
            const bool buttonPressed = ((PINB & (1 << PB2)) == 0);
            if (buttonPressed){
                buttonCheckTime = now + MS_TO_INTERRUPTS(25); //  ������������� �������� �������� ������� ����� 25 �����������
                buttonInProcess = true;
            }
        }
        hasKeyInterrupt = false;
    }

    // ��������� ��������� ������������
    if ((buttonCheckTime > 0) && (now >= buttonCheckTime)){
        const bool buttonPressed = ((PINB & (1 << PB2)) == 0); // ���� ������ ������� - �� ������ ������
        if (buttonPressed){
            // �������� ����� �����
            powerMode = (powerMode + 1) % MODES_POWERS_COUNT;

            // ��������� �����
            // TODO: ������ ��������? ��������� � ���� ���������� ������� ���������?
            //EEPROM_write(0, powerMode); //eeprom_write_byte((uint8_t*)0, powerMode);
            //eeprom_write_byte((uint8_t*)0, powerMode);
            
            // ��������� ��������� �����
            blinksLeft = powerMode+1;
            blinkStartTime = now + MS_TO_INTERRUPTS(blinkONDuration);
            blinkEndTime = blinkStartTime + MS_TO_INTERRUPTS(blinkOFFDuration);
        }
        
        buttonInProcess = false;
        buttonCheckTime = 0;
    }

    // ������ �������� ����������
    if ((blinkStartTime > 0) && (now >= blinkStartTime)){
        // ����� �� ����� PB3 �������
        PORTB |= (1<<PB3);
        // �������� ��������e� �������
        blinkStartTime = 0;
    }
    
    // ��������� ����� ���� ������� �����
    if ((blinkEndTime > 0) && (now >= blinkEndTime)){
        // ����� �� ����� PB3 ��������
        PORTB &= ~(1<<PB3);
        // �������� ���������� �������
        blinkEndTime = 0;
        // ��������� ��������� �����
        blinksLeft--;
        if(blinksLeft > 0){
            blinkStartTime = now + MS_TO_INTERRUPTS(blinkONDuration);
            blinkEndTime = blinkStartTime + MS_TO_INTERRUPTS(blinkOFFDuration);
        }
    }
    
    // ����� ������ ������������ ������ 5 ������
    const uint16_t maxValOverflow = MS_TO_INTERRUPTS(5000); // 1000*60
    if (now > maxValOverflow){
		uint8_t oldSREG = SREG; // Preserve old SREG value
		cli();                  // Disable global interrupts
		const uint16_t nowLocal = timerInterrupts;    // Store timer0 overflow count
		timerInterrupts = 0;
		SREG = oldSREG;         // Restore SREG		
		
		disabledEndTime %= nowLocal;
		enabledEndTime %= nowLocal;
		buttonCheckTime %= nowLocal;
		blinkStartTime %= nowLocal;
		blinkEndTime %= nowLocal;
    }
    
    // ���� ������ �� ������������� - ���������� ��������, ����� �� ������� ������������
    // ��������� ������������ short ���������� ��� ������� ������� ������ int, short ���� ������� �� 55 �����, ������� ���������� ���������
    /*if (!disabledEndTime && !enabledEndTime && !buttonCheckTime && !blinkStartTime && !blinkEndTime){
        timerInterrupts = 0;
    }*/

    // ���������, �� ���� �� ����� ���������� � �������� ������ �������� �����
    if(!hasACInterrupt && !hasKeyInterrupt){
        // ������ ������������ ��������� � ���, ����������� �� ������ ����������
        sleep_enable(); // MCUCR |= (1<<SE);   // �������� ����� ���
        sleep_cpu(); // asm("sleep");
        sleep_disable(); // MCUCR &= ~(1<<SE);   // ��������� ����� ���
        //sei(); // ��� ��� ��������� ����������
    }
}

int main(void) {
    setup();
    while(1) {
        loop();
    }
    return 0;
}
