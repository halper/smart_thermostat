import time
import serial
import RPi.GPIO as GPIO
from Utilities import get_logger, log_message
from Schedule import Schedule
from Heater import Heater

logger = get_logger()

ser = serial.Serial("/dev/ttyUSB0", 9600)  # change ACM number as found from ls /dev/tty/ACM*

GPIO.setmode(GPIO.BOARD)
GPIO.setup(11, GPIO.OUT)

HEATER = Heater(ser)
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
        log_message('Sleeping for 5 minutes!')
        time.sleep(10)
finally:
    GPIO.cleanup()
