#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <SPI.h>
#include <RF24.h>

//#define LOG_LOGIC
#define LOG_RF24
#define OUT_PIN 9
#define PHOTO_PIN 0
#define LIGHT_LIMIT_ON 250
#define LIGHT_LIMIT_OFF 300
#define RADIO_PIN_1 7
#define RADIO_PIN_2 8


typedef enum {
    CMD_GET_STATUS = 0,
    CMD_LIGHT_AUTO = 1,
    CMD_LIGHT_ON = 2,
    CMD_LIGHT_OFF = 3
} RF24Command;

typedef enum {
    ST_LIGHT_AUTO = 0,
    ST_LIGHT_ON = 1,
    ST_LIGHT_OFF = 2
} RF24Status;

typedef struct RF24Result{
    unsigned char status; // RF24Status
    unsigned char enabled;
    unsigned short lightVal;
} RF24Result;

const byte addresses[2][6] = {"1Node", "2Node"};
RF24 radio(RADIO_PIN_1, RADIO_PIN_2);
RF24Status currentMode = ST_LIGHT_AUTO;
bool onStatus = false;



void setup()    {
#if defined(LOG_LOGIC) || defined(LOG_RF24)
    Serial.begin(115000);
#endif

    // Time
    setSyncProvider(RTC.get);     // the function to get the time from the RTC

    // Pin mode
    pinMode(OUT_PIN, OUTPUT);

    // Radio
    radio.begin();
    radio.setRetries(15, 15);
    radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_HIGH);
    //radio.enableAckPayload();
    radio.setAutoAck(true);
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1, addresses[1]);
    radio.printDetails();
    radio.startListening();
}

void loop() {    
    switch(currentMode){
        case ST_LIGHT_AUTO:{
            // Автоматическое включение освещения
            int hourNow = hour();
            int minNow = minute();

            int testTime = hourNow * 60 + minNow;
            bool validStart = (testTime >= (9 * 60 + 0));
            bool validEnd = (testTime < (22 * 60 + 00));
            bool validTime = validStart && validEnd;

            #ifdef LOG_LOGIC
                Serial.print(hourNow);
                Serial.print(":");
                Serial.print(minNow);
                Serial.print(", ");
                Serial.print(validTime);
                Serial.println();
            #endif

            int curLight = analogRead(PHOTO_PIN);
            bool validLight = onStatus ? (curLight < LIGHT_LIMIT_OFF) : (curLight < LIGHT_LIMIT_ON);

            #ifdef LOG_LOGIC
                Serial.print("Photo val: ");
                Serial.print(curLight);
                Serial.print(", ");
                Serial.print(validLight);
                Serial.println();

                Serial.println();
            #endif


            if (validTime && validLight) {
                onStatus = true;
                digitalWrite(OUT_PIN, HIGH);
            } else {
                onStatus = false;
                digitalWrite(OUT_PIN, LOW);
            }
        }break;

        case ST_LIGHT_ON:{
            // Принудительный свет
            onStatus = true;
            digitalWrite(OUT_PIN, HIGH);
        }break;

        case ST_LIGHT_OFF:{
            // Свет отключен
            onStatus = false;
            digitalWrite(OUT_PIN, LOW);
        }break;
    }

    if(radio.available()){
        // Читаем команду
        byte command = 0;
        radio.read(&command, sizeof(command));
        
        #ifdef LOG_RF24
            Serial.print("Command received: ");
            Serial.print(command);
            Serial.println();
        #endif

        // Обрабатываем команду
        switch(command){
            case CMD_GET_STATUS:{
            }break;
            case CMD_LIGHT_AUTO:{
                currentMode = ST_LIGHT_AUTO;
            }break;
            case CMD_LIGHT_ON:{
                currentMode = ST_LIGHT_ON;
            }break;
            case CMD_LIGHT_OFF:{
                currentMode = ST_LIGHT_OFF;
            }break;
        }

        // Формируем результат
        RF24Result result;
        result.status = currentMode;
        result.enabled = onStatus;
        result.lightVal = analogRead(PHOTO_PIN);

        // Отправка результата
        // TODO: Подтверждение отправки???
        radio.stopListening();
        radio.write(&result, sizeof(RF24Result));
        radio.startListening();
    }
}
