#define OUT_PIN 0
#define BUTTON_PIN 1
#define DECCELERATION_TIME_SEC 3.0f

bool enabled = false;
volatile bool buttonPressed = false;

ISR(PCINT0_vect){
    bool buttonStatus = (digitalRead(BUTTON_PIN) == HIGH);
    if(buttonStatus == false){
        return;
    }
    delay(25);
    buttonStatus = (digitalRead(BUTTON_PIN) == HIGH);
    if(buttonStatus){
        buttonPressed = true;
    }
}

void setup() {
    pinMode(OUT_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
  
    digitalWrite(OUT_PIN, LOW);

    // Включаем прерывание
    GIMSK |= (1<<PCIE); // Разрешаем внешние прерывания PCINT0.
    PCMSK |= (1<<BUTTON_PIN); // Разрешаем по маске прерывания на ногак кнопок (PCINT3, PCINT4)
    sei();
}

void loop() {
    if(buttonPressed && !enabled){
        enabled = true;
        
        // После нажатия - сразу максимум
        digitalWrite(OUT_PIN, HIGH);
        delay(200);

        // Корректировка скорости
        bool buttonStatus = (digitalRead(BUTTON_PIN) == HIGH);
        if(buttonStatus == true){
            // Могаем - начался контроль
            digitalWrite(OUT_PIN, LOW);
            delay(50);
            digitalWrite(OUT_PIN, HIGH);

            // Плавно снижаем
            const int delayValue = (int)(DECCELERATION_TIME_SEC/240.0f*1000.0);
            for(int i = 255; i >= 15; i--){
                buttonStatus = (digitalRead(BUTTON_PIN) == HIGH);
                if(!buttonStatus){
                    break;
                }
                analogWrite(OUT_PIN, i);
                delay(delayValue);
            }
        }

        buttonPressed = false;
    }
    if(buttonPressed && enabled){
        enabled = false;
        digitalWrite(OUT_PIN, LOW);
        buttonPressed = false; 
    }
}
