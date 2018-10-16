При прошивке с помощью Arduino в SinaProg надо выбирать AVRISP, BR-19200
Для прошивки с помощью Arduino через ArduinoIDE - надо обязательно повесить конденсатор между GND и Reset у Arduino, иначе будет ошибка (либо можно провод между 5V и RST)
Поддержка Attiny13 для ArduinoIDE, наиболее продвинутая - https://github.com/MCUdude/MicroCore