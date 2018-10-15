#define OUT_PIN 0
#define BUTTON_PIN 1

bool enabled = false;

void setup() {
  pinMode(OUT_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  
  digitalWrite(OUT_PIN, LOW);
}

void loop() {
    bool buttonStatus = (digitalRead(BUTTON_PIN) == HIGH);
    if(buttonStatus && !enabled){
        enabled = true;
        const int delayValue = (int)(1.0f/255.0f*1000.0);
        for(int i = 0; i <= 255; i++){
            analogWrite(OUT_PIN, i);
            delay(delayValue);
            buttonStatus = (digitalRead(BUTTON_PIN) == HIGH);
            if(!buttonStatus){
               break;
            }
        }
    }
    if(!buttonStatus && enabled){
        enabled = false;
        digitalWrite(OUT_PIN, LOW);    
    }
}
