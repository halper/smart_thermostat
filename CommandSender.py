import serial
import RPi.GPIO as GPIO
import time

ser = serial.Serial("/dev/ttyUSB0", 9600, timeout=10)  # change ACM number as found from ls /dev/tty/ACM*
ser.baudrate = 9600

GPIO.setmode(GPIO.BOARD)
GPIO.setup(11, GPIO.OUT)

def read_serial(passed_ser):
    read_ser = passed_ser.readline().decode().strip()
    print(read_ser)

def print_commands():
    my_str = ""
    my_str += "0. Get data\n"
    my_str += "1. Turn on the heater\n"
    my_str += "2. Turn off the heater\n"
    my_str += "3. Test the connection to the heater\n"
    print(my_str)
try:
    # wait for setup phase
    while True:
        read_ser = ser.readline().decode().strip()
        print('{}'.format(read_ser))
        if '3. Test the connection' in read_ser:
            time.sleep(2)
            break
    while True:
        text = -1
        print_commands()
        while text not in {'0', '1', '2', '3'}:
            text = input("Select a command: ")
        ser.write(str.encode(text))
        time.sleep(2)
        read_serial(ser)
        if text == '3':
            time.sleep(10)
            read_serial(ser)
            read_serial(ser)
        print('\n')
except Exception as e:
    print(e)
finally:
    GPIO.cleanup()
