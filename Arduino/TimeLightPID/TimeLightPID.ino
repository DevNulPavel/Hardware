#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <PID_v1.h>

//#define WITH_LOG
#define OUT_PIN 3
#define PHOTO_PIN 0

bool onStatus = false;
double targetValue = 0.0;
double currentValue = 0.0;
double resultValue = 0.0;

//Specify the links and initial tuning parameters
const double kp = 0.0;
const double ki = 0.01;
const double kd = 0.0;
PID pid(&currentValue, &resultValue, &targetValue, kp, ki, kd, DIRECT);


void setup()  {
#ifdef WITH_LOG
  Serial.begin(115200);
#endif
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
  pinMode(OUT_PIN, OUTPUT);
  pid.SetMode(AUTOMATIC);
  pid.SetSampleTime(50); // Раз в 50 миллисекунды будет рассчет

  // Частота ШИМ для 3 ноги 3906.25гц
  // http://kazus.ru/forums/showthread.php?t=107888
  TCCR2B = TCCR2B & 0b11111000 | 0x02;

  targetValue = 600.0;
}

void loop() {
  int hourNow = hour();
  int minNow = minute();

  int testTime = hourNow*60 + minNow;
  bool validStart = (testTime >= (9*60+0));
  bool validEnd = (testTime < (21*60+30));
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
  
  delay(1);
  pid.Compute();

#ifdef WITH_LOG
  Serial.print("Photo val: ");
  Serial.print(currentValue);
  Serial.print(", pid result val: ");
  Serial.print(resultValue);
  Serial.println();

  Serial.println();
#endif

  //int pinOutValue = map(resultValue, 0, targetValue, 0, 255);
  analogWrite(OUT_PIN, resultValue);

  delay(20);
}
