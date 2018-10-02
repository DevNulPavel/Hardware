#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import sys
import httplib
import urllib
import time
import os
import os.path
import traceback
import json
from dht11 import dht11
import RPi.GPIO as GPIO


PIN = 4
HUMIDITY_ALERT = 65.0
TEMP_ALERT = 35.0
APP_ID = "E157D178-503A-437C-DA54-A93332A0B657"
NUMBER = "79113228054"
URL = "sms.ru"
PORT = 80
PATH = "/sms/send?api_id="+APP_ID+"&to="+NUMBER+"&msg={0}&json=1"
SMS_PERIOD = 60 * 60 * 5 # 5 min period
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))


lastSMSSendTime = 0.0

def sendAlertSMS(humidity, temp):
    global lastSMSSendTime
    # ограничение частоты SMS
    timeNow = time.time()
    if timeNow < (lastSMSSendTime + SMS_PERIOD):
        return

    message = "Odroid->humidity={0:0.1f},temp={1:0.1f}!".format(humidity, temp)
    #message = urllib.urlencode(message)
    pathFormatted = PATH.format(message)

    conn = httplib.HTTPConnection(URL, PORT, timeout=30)
    conn.request("GET", pathFormatted)
    response = conn.getresponse()
    status = response.status
    if status == 200:
        lastSMSSendTime = timeNow
    else:
        pass


def main():
    GPIO.setmode(GPIO.BCM)
    try:
        while True:
            #print "Try to get temp"
            
            dhtInst = dht11.DHT11(pin=PIN)
            if (dhtInst is None):
                time.sleep(5)
                print "Try again 1"
                continue

            result = dhtInst.read()

            if (result is None):
                time.sleep(5)
                print "Try again 2"
                continue

            humidity = result.humidity
            temperature = result.temperature

            if (humidity == 0) or (temperature == 0):
                time.sleep(5)
                print "Try again 3"
                continue

            print 'Temp={0:0.1f}*  Humidity={1:0.1f}%'.format(temperature, humidity)

            # Analyze humidity
            alertMode = False
            if (humidity >= HUMIDITY_ALERT) or (temperature >= TEMP_ALERT):
                sendAlertSMS(humidity, temperature)
                alertMode = True

            # Выводим температуру в консоль + в файл
            resultJson = {
                "humidity": humidity,
                "temperature": temperature,
                "alert": alertMode
            }
            jsonString = json.dumps(resultJson)
            print(jsonString)
            with open(os.path.join(SCRIPT_DIR, "humidity_control_out.json"), 'w') as f:
                f.write(jsonString)

            # Sleep until next step
            time.sleep(10)

    except KeyboardInterrupt:
        print("Exit pressed Ctrl+C")            # Выход из программы по нажатию Ctrl+C
    except:
        print("Other Exception")                # Прочие исключения
        print("--- Start Exception Data:")
        traceback.print_exc(limit=2, file=sys.stdout) # Подробности исключения через traceback
        print("--- End Exception Data:")
    finally:
        print("CleanUp")                        # Информируем о сбросе пинов
        #GPIO.output(controlPin, GPIO.HIGH) # Оставляем включеным вентилятор
        GPIO.cleanup()                          # Возвращаем пины в исходное состояние
        print("End of program")                 # Информируем о завершении работы программы


if __name__ == '__main__':
    main()
