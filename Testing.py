import serial
import RPi.GPIO as GPIO
import time

ser = serial.Serial("/dev/ttyUSB0", 9600)  # change ACM number as found from ls /dev/tty/ACM*
ser.baudrate = 9600

GPIO.setmode(GPIO.BOARD)
GPIO.setup(11, GPIO.OUT)
line_break = False
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
except Exception as e:
    print(e)
finally:
    GPIO.cleanup()
