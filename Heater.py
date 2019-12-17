from Utilities import PI_COMMANDS, log_message, get_logger
from TempSensor import TempSensor
import time

class Heater:
    LAST_STAT = 'NONE'
    COMMANDS = {'ON', 'OFF'}

    def __init__(self, ser):
        self.ser = ser
        self.tempSensor = TempSensor(ser)

    def turn_off(self):
        return self.switch_burner('OFF')

    def turn_on(self):
        return self.switch_burner('ON')

    def switch_burner(self, on_off_command):
        on_off_command = str.upper(on_off_command)
        if on_off_command not in self.COMMANDS:
            get_logger().error('Heater\'s burner stat is not right! {}'.format(on_off_command))
            return False
        log_message('Turning {} the heater!'.format(str.lower(on_off_command)))
        self.ser.write(PI_COMMANDS['HEATER_{}'.format(on_off_command)])
        time.sleep(0.5)
        response = self.ser.readline().decode().strip()
        if 'ERROR' in response:
            print('ERROR', response)
            get_logger().error('Heater\'s stat could not be set to {}! {}'.format(on_off_command, response))
            return False
        # Check if heater status is successfully set
        self.LAST_STAT = on_off_command
        return True

    def is_on(self):
        return self.check_status('ON')

    def is_off(self):
        return self.check_status('OFF')

    def check_status(self, status):
        return str.upper(self.LAST_STAT) == str.upper(status)

    def get_status(self):
        # returns ON or OFF
        return self.LAST_STAT

    def set_status(self, pref_temp):
        avg_temp = self.tempSensor.get_avg_temp()
        log_message("Average temperature for the last 5 minutes is: {:.2f}".format(avg_temp))
        if avg_temp >= pref_temp + 0.2:
            if self.turn_off():
                log_message('It gets hot with {:.1f}oC - {:.1f}oC is preferred'.format(avg_temp, pref_temp))
        elif avg_temp <= pref_temp - 0.3:
            if self.turn_on():
                log_message('It is kinda chilly with {:.1f}oC - {:.1f}oC is preferred'.format(avg_temp, pref_temp))
        else:
            self.ser.write(PI_COMMANDS['SEND_ALIVE'])
            time.sleep(0.2)
            response = self.ser.readline().decode().strip()
            if 'ERROR' in response:
                print('ERROR', response)
                get_logger().error('Could not send ALIVE_SIG')
                return False