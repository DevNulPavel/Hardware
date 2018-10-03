#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>

//#define WITH_LOG
#define OUT_PIN 3
#define PHOTO_PIN 0
#define DELAY_SECONDS 2
#define LIGHT_LIMIT_ON 250
#define LIGHT_LIMIT_OFF 300

bool onStatus = false;

void setup()  {
#ifdef WITH_LOG
  Serial.begin(9600);
#endif
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  pinMode(OUT_PIN, OUTPUT);
}

void loop() {
  int hourNow = hour();
  int minNow = minute();

  int testTime = hourNow*60 + minNow;
  bool validStart = (testTime >= (9*60+0));
  bool validEnd = (testTime < (21*60+0));
  bool validTime = validStart && validEnd;

#ifdef WITH_LOG
  Serial.print(hourNow);
  Serial.print(":");
  Serial.print(minNow);
  Serial.print(", ");
  Serial.print(validTime);
  Serial.println();
#endif

  int curLight = analogRead(PHOTO_PIN);
  bool validLight = onStatus ? (curLight < LIGHT_LIMIT_OFF) : (curLight < LIGHT_LIMIT_ON);

#ifdef WITH_LOG
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

  delay(1000 * DELAY_SECONDS);
}
