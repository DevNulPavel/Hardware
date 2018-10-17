#define OUT_PIN 0
#define BUTTON_PIN 1
#define FADE_IN_TIME_SEC 1.5f

bool enabled = false;

void setup() {
    // Настраиваем низкое энергопотребление (Reset ножку не трогаем)
    for(char i = 0; i <= 4; i++){
        pinMode(i, OUTPUT);
    }

    pinMode(OUT_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
    
    digitalWrite(OUT_PIN, LOW);
}

void loop() {
    bool buttonStatus = (digitalRead(BUTTON_PIN) == HIGH);
    if(buttonStatus && !enabled){
        delay(10);
        buttonStatus = (digitalRead(BUTTON_PIN) == HIGH);

        if(buttonStatus){           
            enabled = true;

            digitalWrite(OUT_PIN, HIGH);
            delay(100);
            
            const int delayValue = (int)(FADE_IN_TIME_SEC/255.0f*1000.0);
            for(int i = 75; i <= 255; i++){
                analogWrite(OUT_PIN, i);
                delay(delayValue);
                buttonStatus = (digitalRead(BUTTON_PIN) == HIGH);
                if(!buttonStatus){
                   break;
                }
            }

            digitalWrite(OUT_PIN, HIGH);
        }
    }
    if(!buttonStatus && enabled){
        enabled = false;
        digitalWrite(OUT_PIN, LOW);    
    }
}
