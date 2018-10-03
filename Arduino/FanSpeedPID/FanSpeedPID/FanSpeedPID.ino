#include <PID_v1.h>

#define PIN_OUTPUT 3


double setpoint = 0.0;
double input = 0.0;
double output = 0.0;
volatile long int changeCount = 0;

//Specify the links and initial tuning parameters
const double kp = 0.0;
const double ki = 0.5;
const double kd = 0.0;
PID pid(&input, &output, &setpoint, kp, ki, kd, DIRECT);


void valueChanged() {
  changeCount++;
}

void setup() {
  Serial.begin(115200);

  setpoint = 500.0;

  //turn the PID on
  pid.SetMode(AUTOMATIC);

  // Частота ШИМ для 3 ноги 3906.25гц
  // http://kazus.ru/forums/showthread.php?t=107888
  TCCR2B = TCCR2B & 0b11111000 | 0x06;

  attachInterrupt(0, valueChanged, CHANGE); // Pin D2
}

void loop() {
  delay(1000);

  input = changeCount;
  changeCount = 0;

  //pid.Compute();

  Serial.print(input);
  Serial.print(" ");
  Serial.print(output);
  Serial.println();
  Serial.flush();

  //analogWrite(PIN_OUTPUT, output);
}
