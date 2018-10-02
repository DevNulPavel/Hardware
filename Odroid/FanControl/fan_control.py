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

try:
    onTime = 2          # Задержка в секундах перед включением
    offTime = 60        # Задержка в секундах перед отключением
    tempOn = 60.0                             # Температура включения кулера
    tempOff = 45.0                            # Температура включения кулера
    controlPin = 18                         # Пин отвечающий за управление

    # Состояние
    pinState = GPIO.LOW                        # Актуальное состояние кулера
    offDelay = 0
    onDelay = 0
    # === Инициализация пинов ===
    GPIO.setmode(GPIO.BCM)                  # Режим нумерации в BCM
    GPIO.setup(controlPin, GPIO.OUT, initial=pinState) # Управляющий пин в режим OUTPUT

    while True:                             # Бесконечный цикл запроса температуры
        temp = getNormalizedTemp()                   # Получаем значение температуры

        if temp > tempOn:
            offDelay = offTime
            onDelay = onDelay-1
            if (pinState == GPIO.LOW) and (onDelay <= 0):
                onDelay = 0
                pinState = GPIO.HIGH         # Меняем статус состояния
                GPIO.output(controlPin, pinState) # Задаем новый статус пину управления

        if temp < tempOff:
            onDelay = onTime
            offDelay = offDelay-1
            if (pinState == GPIO.HIGH) and (offDelay <= 0):
                offDelay = 0
                pinState = GPIO.LOW         # Меняем статус состояния
                GPIO.output(controlPin, pinState) # Задаем новый статус пину управления

        # Выводим температуру в консоль + в файл
        resultJson = {
        	"temperature": temp,
        	"value": pinState
        }
        jsonString = json.dumps(resultJson)
        print(jsonString)
        with open(os.path.join(SCRIPT_DIR, "fan_control_out.json"), 'w') as f:
        	f.write(jsonString)

        sleep(1)                            # Пауза - 1 секунда

except KeyboardInterrupt:
    # ...
    print("Exit pressed Ctrl+C")            # Выход из программы по нажатию Ctrl+C
except:
    # ...
    print("Other Exception")                # Прочие исключения
    print("--- Start Exception Data:")
    traceback.print_exc(limit=2, file=sys.stdout) # Подробности исключения через traceback
    print("--- End Exception Data:")
finally:
    print("CleanUp")                        # Информируем о сбросе пинов
    #GPIO.output(controlPin, GPIO.HIGH) # Оставляем включеным вентилятор
    GPIO.cleanup()                          # Возвращаем пины в исходное состояние
    print("End of program")                 # Информируем о завершении работы программы
