import serial
import RPi.GPIO as GPIO
import time

ser = serial.Serial("/dev/ttyUSB0", 9600)  # change ACM number as found from ls /dev/tty/ACM*
ser.baudrate = 9600

GPIO.setmode(GPIO.BOARD)
GPIO.setup(11, GPIO.OUT)
line_break = False
def print_commands():
    my_str = ""
    my_str += "0. Get data\n"
    my_str += "1. Turn on the heater\n"
    my_str += "2. Turn off the heater\n"
    my_str += "3. Get heater status\n"
    my_str += "4. Test the connection to the heater\n"
    print(my_str)
try:
    # wait for setup phase
    while True:
        while True:
            read_ser = ser.readline().decode().strip()
            print('{}'.format(read_ser))
            if '4. Test the connection' in read_ser:
                time.sleep(2)
                break
            elif read_ser == '\n' and line_break:
                break
            elif read_ser == '\n':
                line_break = True
            else:
                line_break = False
        print('Break out of while')
        while True:
            text = -1
            while text not in {'0', '1', '2', '3', '4'}:
                print_commands()
                text = input("Select a command: ")
            ser.write(str.encode(text))
            time.sleep(2)
except Exception as e:
    print(e)
finally:
    GPIO.cleanup()
