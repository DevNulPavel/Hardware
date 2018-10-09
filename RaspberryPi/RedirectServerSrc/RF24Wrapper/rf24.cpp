#include "rf24.h"
#include <stdio.h>
#include <RF24/RF24.h>

#ifdef __cplusplus
extern "C" {
#endif


const uint8_t pipes[2][6] = {"1Node","2Node"};

RF24 radio(RPI_BPLUS_GPIO_J8_15, RPI_BPLUS_GPIO_J8_24, BCM2835_SPI_SPEED_8MHZ);



void RF24Begin(){
    radio.begin();
    radio.setRetries(15, 15);
    radio.setDataRate(RF24_250KBPS);
    radio.setPALevel(RF24_PA_HIGH);
    //radio.enableAckPayload();
    radio.setAutoAck(true);
    radio.openWritingPipe(pipes[1]);
    radio.openReadingPipe(1, pipes[0]);
    radio.printDetails();
}

void RF24Ping(){
    printf("Now sending...\n");
    unsigned long time = millis();

    bool ok = radio.write(&time, sizeof(unsigned long));
    if (!ok){
        printf("failed.\n");
    }

    radio.startListening();

    unsigned long startedWaitingAt = millis();
    bool timeout = false;
    while(!radio.available() && !timeout) {
        if ((millis() - startedWaitingAt) > 200){
            timeout = true;
        }
    }

    // Describe the results
    if (timeout) {
        printf("Failed, response timed out.\n");
    }else{
        // Grab the response, compare, and send to debugging spew
        unsigned long gotTime = 0;
        radio.read(&gotTime, sizeof(unsigned long));

        // Spew it
        printf("Got response %lu, round-trip delay: %lu\n", gotTime, millis()-gotTime);
    }
    radio.stopListening();
}

RF24Error RF24SendCommand(RF24Command command, RF24Result* result){
    if (result == NULL){
        return ERR_NO_RESULT_PTR;
    }

    const unsigned long timeoutMilliSec = 200;

    unsigned long startedSendAt = millis();
    bool writeTimeout = false;
    while(!radio.write(&command, sizeof(char)) && !writeTimeout) {
        if ((millis() - startedSendAt) > timeoutMilliSec){
            writeTimeout = true;
        }
    }
    if (writeTimeout){
        printf("RF24 write to target failed\n");
        return ERR_WRITE_FAILED;
    }

    radio.startListening();

    unsigned long startedWaitingAt = millis();
    bool timeout = false;
    while(!radio.available() && !timeout) {
        if ((millis() - startedWaitingAt) > timeoutMilliSec){
            timeout = true;
        }
    }

    radio.stopListening();

    if (timeout) {
        printf("RF24 response timed out\n");
        return ERR_TIMEOUT;
    }

    radio.read(result, sizeof(RF24Result)); // TODO: Надо ли учитывать порядок байтов??

    return ERR_OK;
}

void TestCGo() {
    printf("TEST! I am in C code now!\n");
}

#ifdef __cplusplus
}
#endif