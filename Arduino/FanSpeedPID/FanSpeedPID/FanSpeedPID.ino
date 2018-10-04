#include <PID_v1.h>
//#include <CyberLib.h>

#define PIN_OUTPUT 5
#define INTERRUPT_PIN 2
#define INTERRUPT_PWM_PIN 3
#define INTERRUPT_INDEX 0 // Pin D2, INTERRUPT_PIN
#define INTERRUPT_PWM_INDEX 1 // Pin D3, INTERRUPT_PWM_PIN


double setpoint = 0.0;
double input = 0.0;
double output = 0.0;
volatile long int changeCountInterupt = 0;
volatile long int changeCountPWMInterupt = 0;

// Параметры регуляции
const double kp = 0.0;
const double ki = 0.5;
const double kd = 0.0;
PID pid(&input, &output, &setpoint, kp, ki, kd, DIRECT);


void interuptValueChanged() {
    changeCountInterupt++;
}

void interuptPWMChanged() {
    changeCountPWMInterupt++;
}

// Читаем значение на выходном пине для отброса PWM
//bool outStatus = bitRead(PORTD, PIN_OUTPUT);

void setup() {
    Serial.begin(115200);

    setpoint = 500.0;

    // ПИД регулятор будет автоматический
    pid.SetMode(AUTOMATIC);

    // http://kazus.ru/forums/showthread.php?t=107888
    // Частота ШИМ для 3 ноги 3906.25гц
    // TCCR2B = TCCR2B & 0b11111000 | 0x02;
    // Частота ШИМ для 5 ноги 7812.5гц
    // TCCR0B = TCCR0B & 0b11111000 | 0x02;

    // Прерывания
    attachInterrupt(INTERRUPT_INDEX, interuptValueChanged, CHANGED);
    attachInterrupt(INTERRUPT_PWM_INDEX, interuptPWMChanged, CHANGED);
}

void loop() {
    delay(1000);

    long int diff = max(changeCountInterupt - changeCountPWMInterupt, 0);
    changeCountInterupt = 0;
    changeCountPWMInterupt = 0;

    input = diff;
    pid.Compute();

    Serial.print(input);
    Serial.print(" ");
    Serial.print(output);
    Serial.println();
    Serial.flush();

    analogWrite(PIN_OUTPUT, output);
}
