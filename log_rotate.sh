#!/bin/bash

tar -zcvf "/home/pi/logs/arduino-$(date +"%Y-%m-%d").tar.gz" /home/pi/smart_thermostat/arduino.log
