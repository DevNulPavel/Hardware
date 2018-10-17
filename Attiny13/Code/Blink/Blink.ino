#define LED_BUILTIN 0

void setup() {
    // Настраиваем низкое энергопотребление (Reset ножку не трогаем)
    for(char i = 0; i <= 4; i++){
        pinMode(i, OUTPUT);
    }

    pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
    const int delayValue = (int)(1.0f/255.0f*1000.0);
    for(int i = 0; i < 255; i++){
        analogWrite(LED_BUILTIN, i);
        delay(delayValue);
    }
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
}
