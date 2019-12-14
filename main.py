import time
import serial
import RPi.GPIO as GPIO
from Utilities import get_logger, log_message
from Schedule import Schedule
from Heater import Heater
from TempSensor import TempSensor

logger = get_logger()

ser = serial.Serial("/dev/ttyUSB0", 9600)  # change ACM number as found from ls /dev/tty/ACM*

GPIO.setmode(GPIO.BOARD)
GPIO.setup(11, GPIO.OUT)

HEATER = Heater(ser)
TEMP_SENSOR = TempSensor(ser)
schedule = Schedule('schedule')

try:
    log_message("The script is started")
    # wait for setup phase
    while True:
        read_ser = ser.readline().decode().strip()
        if 'Test the connection' in read_ser:
            time.sleep(5)
            break
    log_message("Heater status is: {}\n".format(HEATER.get_status()))
    while True:
        log_message("Reading schedule file")
        HEATER.set_status(schedule.get_pref_temp())
        for i in range(20):
            current_temp = TEMP_SENSOR.get_current_temp()
            TEMP_SENSOR.temperature_readings.push(current_temp)
            time.sleep(15)
finally:
    GPIO.cleanup()
