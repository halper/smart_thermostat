#!/bin/bash

tar -zcvf "/home/pi/logs/arduino-$(date +"%Y-%m-%d").tar.gz" /home/pi/smart_thermostat/arduino.log && rm /home/pi/smart_thermostat/arduino.log

# crontabs
# root
# 1 0 * * * . /home/pi/smart_thermostat/log_rotate.sh && /sbin/shutdown -r now

# pi
# @reboot cd /home/pi/smart_thermostat && nohup python3 main.py &
