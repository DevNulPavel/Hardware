#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <PID_v1.h>

//#define WITH_LOG
#define OUT_PIN 2
#define PHOTO_PIN 0
#define DELAY_SECONDS 2
#define LIGHT_LIMIT_ON 250
#define LIGHT_LIMIT_OFF 300

bool onStatus = false;
double targetValue = 0.0;
double currentValue = 0.0;
double resultValue = 0.0;

//Specify the links and initial tuning parameters
const double kp = 2.0;
const double ki = 5.0;
const double kd = 1.0;
PID pid(&currentValue, &resultValue, &targetValue, kp, ki, kd, DIRECT);


void setup()  {
#ifdef WITH_LOG
  Serial.begin(9600);
#endif
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  pinMode(OUT_PIN, OUTPUT);
  pid.SetMode(AUTOMATIC);

  targetValue = 450.0;
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

  currentValue = analogRead(PHOTO_PIN);
  pid.Compute();

#ifdef WITH_LOG
  Serial.print("Photo val: ");
  Serial.print(currentValue);
  Serial.print(", pid result val");
  Serial.print(resultValue);
  Serial.println();

  Serial.println();
#endif

  int pinOutValue = map(resultValue, 0, targetValue, 0, 255);
  analogWrite(OUT_PIN, pinOutValue);

  delay(1000 * DELAY_SECONDS);
}
