#include <IRremote.h>

#define IR_DIGITAL_PIN 2
#define FAN_PIN 9

IRrecv irrecv(IR_DIGITAL_PIN); // указываем вывод, к которому подключен приемник
bool fanEnabled = false;
int fanSpeed = 0;
unsigned int lastVal = 0;


void setup() {
    Serial.begin(115200); // выставляем скорость COM порта
    irrecv.enableIRIn(); // запускаем прием
    pinMode(FAN_PIN, OUTPUT);

    // http://kazus.ru/forums/showthread.php?t=107888
    // Частота ШИМ для 3 ноги 3906.25гц
    //TCCR2B = TCCR2B & 0b11111000 | 0x02;
    // Частота ШИМ для 5 ноги 7812.5гц
    // TCCR0B = TCCR0B & 0b11111000 | 0x02;
    // Частота ШИМ для 9 ноги 31250гц
    TCCR1B = TCCR1B & 0b11111000 | 0x01;
}

void loop() {
    decode_results results;
    while(irrecv.decode(&results)) { // если данные пришли
        unsigned int curVal = 0;
        if(results.value == 0xFFFFFFFF){
            curVal = lastVal;
        }else{
            curVal = results.value;
        }
        lastVal = curVal;
        
        switch(curVal){
            case 0x9D6228D7: // Up
                fanSpeed += 5;
                break;
            case 0x9D62A857: // Down
                fanSpeed -= 5;
                break;
            case 0x9D62C837: // ON OFF
                fanEnabled = !fanEnabled;
                fanSpeed = 255;
                break;
        }
        irrecv.resume(); // принимаем следующую команду

        fanSpeed = constrain(fanSpeed, 0, 255);

        Serial.print(results.value, BIN);
        Serial.print(" ");
        Serial.print(results.value, HEX);
        Serial.print(" ");
        Serial.print(fanEnabled);
        Serial.print(" ");
        Serial.print(fanSpeed);
        Serial.println();
    }

    if(fanEnabled){
        analogWrite(FAN_PIN, fanSpeed);
    }else{
        analogWrite(FAN_PIN, 0);
    }
}
