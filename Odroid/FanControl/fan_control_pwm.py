#! /usr/bin/env python2.7
# -*- coding: utf-8 -*-

import RPi.GPIO as GPIO                     # Импортируем библиотеку по работе с GPIO
import sys, os, os.path, traceback          # Импортируем библиотеки
import json

from time import sleep                      # Импортируем библиотеку для работы со временем
from re import findall                      # Импортируем библиотеку по работе с регулярными выражениями
from subprocess import check_output         # Импортируем библиотеку по работе с внешними процессами

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

lastTempList = []

def getTemp():
    temp = check_output(["/bin/cat","/sys/class/thermal/thermal_zone0/temp"]).decode()
    temp = float(temp)/1000.0
    return temp

def getNormalizedTemp():
    checkCount = 10          # Получаем среднее между 10 измерениями температуры
    curTemp = getTemp()
    lastTempList.append(curTemp)
    if len(lastTempList) > checkCount:
        lastTempList.pop(0)
        summ = 0
        for oldTemp in lastTempList:
            summ += oldTemp
        summ /= float(checkCount)
        return summ
    else:
        return curTemp


def main():
    try:
        tempOn2 = 60.0                             # Температура включения кулера
        tempOn1 = 55.0                             # Температура включения кулера
        tempOff = 50.0                            # Температура включения кулера
        level2 = 100.0
        level1 = 40.0
        controlPin = 18                         # Пин отвечающий за управление

        # Состояние
        pinValue = 0.0

        # === Инициализация пинов ===
        GPIO.setmode(GPIO.BCM)                  # Режим нумерации в BCM
        GPIO.setup(controlPin, GPIO.OUT, initial=GPIO.LOW) # Управляющий пин в режим OUTPUT

        pwm = GPIO.PWM(controlPin, 1000)
        pwm.start(pinValue)

        while True:                             # Бесконечный цикл запроса температуры
            temp = getNormalizedTemp()

            if temp > tempOn2:
                if pinValue < level2:
                    pinValue = level2
                    pwm.ChangeDutyCycle(pinValue)
            elif temp > tempOn1:
                if pinValue < level1:
                    pinValue = level1
                    pwm.ChangeDutyCycle(pinValue)
            elif temp < tempOff:
                if pinValue != 0.0:
                    pinValue = 0.0
                    pwm.ChangeDutyCycle(pinValue)

            # Выводим температуру в консоль + в файл
            resultJson = {
            	"temperature": temp,
            	"value": pinValue
            }
            jsonString = json.dumps(resultJson)
            print(jsonString)
            with open(os.path.join(SCRIPT_DIR, "fan_control_pwm_out.json"), 'w') as f:
                f.write(jsonString)

            sleep(5)                            # Пауза - 1 секунда

    except KeyboardInterrupt:
        print("Exit pressed Ctrl+C")            # Выход из программы по нажатию Ctrl+C
    except:
        print("Other Exception")                # Прочие исключения
        print("--- Start Exception Data:")
        traceback.print_exc(limit=2, file=sys.stdout) # Подробности исключения через traceback
        print("--- End Exception Data:")
    finally:
        print("CleanUp")                        # Информируем о сбросе пинов
        pwm.stop()
        #GPIO.output(controlPin, GPIO.HIGH) # Оставляем включеным вентилятор
        GPIO.cleanup()                          # Возвращаем пины в исходное состояние
        print("End of program")                 # Информируем о завершении работы программы

if __name__ == '__main__':
    main()
