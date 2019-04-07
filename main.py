import datetime
import time
from SensorData import SensorData
import serial
import RPi.GPIO as GPIO
import logging

logger = logging.getLogger()
logging.basicConfig(filename='arduino.log', filemode='w', format='%(asctime)s - %(message)s',
                    datefmt='%d-%b-%y %H:%M:%S')
logger.setLevel(logging.INFO)

ser = serial.Serial("/dev/ttyUSB0", 9600)  # change ACM number as found from ls /dev/tty/ACM*

GPIO.setmode(GPIO.BOARD)
GPIO.setup(11, GPIO.OUT)

START_TIME_POS = 1
END_TIME_POS = 2
PREF_TEMP_POS = 3
PI_COMMANDS = {
    'GET_DATA': str.encode("0"),
    'HEATER_ON': str.encode("1"),
    'HEATER_OFF': str.encode("2"),
    'HEATER_STATUS': str.encode("3"),
    'TEST_HEATER': str.encode("4"),
    'TEST_TEMP': str.encode("5")
}


def get_time(time_string):
    hour, minute = time_string.split(':')
    hour = int(hour)
    minute = int(minute)
    return datetime.time(hour=hour, minute=minute)


def get_current_temp():
    avg_temp = get_sensor_data().get_temp()
    for i in range(0, 9):
        avg_temp += get_sensor_data().get_temp()
        time.sleep(.200)
    return avg_temp / 10.0


def get_sensor_data():
    # Sensor data is being printed as
    # ADDR HUM TEMP HEAT_INDEX - separated with tab
    ser.write(PI_COMMANDS['GET_DATA'])
    time.sleep(2)
    sensor_raw_data = ser.readline().decode().strip()
    log_message(sensor_raw_data)
    sensor_data = SensorData('{}'.format(sensor_raw_data))
    return sensor_data


def turn_off_heater():
    if get_heater_status() == 'OFF':
        return
    log_message('Turning off the heater!')
    ser.write(PI_COMMANDS['HEATER_OFF'])


def turn_on_heater():
    if get_heater_status() == 'ON':
        return
    log_message('Turning on the heater!')
    ser.write(PI_COMMANDS['HEATER_ON'])


def get_heater_status():
    # returns ON or OFF
    ser.write(PI_COMMANDS['HEATER_STATUS'])
    heater_status = ser.readline().decode().strip()
    log_message('Heater Status is ' + heater_status)
    return heater_status


def log_message(message):
    print(message)
    logger.info(message)


try:
    print("The script is started")
    while True:
        read_ser = ser.readline().decode().strip()
        if '4. Test the connection' in read_ser:
            break
    print("Heater status is: {}\n".format(get_heater_status()))
    while True:
        now = datetime.datetime.now()
        weekday = now.strftime('%w')
        hour = now.strftime('%H')
        minute = now.strftime('%M')
        log_message("Reading schedule file")
        with open('schedule') as f:
            for line in f:
                if not line.startswith(weekday):
                    continue
                splitted_line = line.rstrip().split()
                start_time = get_time(splitted_line[START_TIME_POS])
                end_time = get_time(splitted_line[END_TIME_POS])
                preferred_temp = float(splitted_line[PREF_TEMP_POS])
                if end_time > now.time() >= start_time:
                    log_message("Checking record for {} - {} for temp {:.1f}".format(start_time, end_time, preferred_temp))
                    current_temp = get_current_temp()
                    log_message("Current temprature is: {:.2f}".format(current_temp))
                    if current_temp >= preferred_temp + 1.0:
                        log_message('It gets hot with {:.1f}oC - {:.1f} is preferred'.format(current_temp, preferred_temp))
                        turn_off_heater()
                    elif current_temp <= preferred_temp - 0.5:
                        log_message('It is kinda chilly with {:.1f}oC - {:.1f} is preferred'.format(current_temp, preferred_temp))
                        turn_on_heater()
                    else:
                        log_message('The temp is just right {:.1f}oC - {:.1f} is preferred'.format(current_temp, preferred_temp))
                    break
        log_message('Sleeping for 30 seconds')
        time.sleep(30)
finally:
    GPIO.cleanup()