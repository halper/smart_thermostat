from Utilities import PI_COMMANDS, log_message, get_logger
from datetime import datetime, timedelta
from TempSensor import TempSensor


class Heater:
    LAST_STAT = 'NONE'
    STATUS_LAST_CHECK = -1

    def __init__(self, ser):
        self.ser = ser
        self.tempSensor = TempSensor(ser)

    def turn_off(self):
        return self.switch_burner('OFF')

    def turn_on(self):
        return self.switch_burner('ON')

    def switch_burner(self, on_off_command):
        on_off_command = str.upper(on_off_command)
        if on_off_command != 'ON' or on_off_command != 'OFF':
            get_logger().error('Heater\'s burner stat is not right! {}'.format(on_off_command))
            return False
        if self.check_status(on_off_command):
            return False
        log_message('Turning {} the heater!'.format(str.lower(on_off_command)))
        self.ser.write(PI_COMMANDS['HEATER_{}'.format(on_off_command)])
        self.reset_heater_stat()
        # Check if heater status is successfully set
        if self.LAST_STAT != on_off_command:
            get_logger().error('Heater status is not set right! Expected: {}, received: {}'.format(on_off_command, self.LAST_STAT))
            print('Error while setting burner! Check log file!')
            return False
        return True

    def reset_heater_stat(self):
        self.LAST_STAT = 'NONE'
        self.get_status()

    def is_on(self):
        return self.check_status('ON')

    def is_off(self):
        return self.check_status('OFF')

    def check_status(self, status):
        return str.upper(self.get_status()) == str.upper(status)

    def get_status(self):
        # returns ON or OFF
        # update status every 30 minutes
        if self.LAST_STAT == 'NONE' or self.STATUS_LAST_CHECK + timedelta(minutes=30):
            self.ser.write(PI_COMMANDS['HEATER_STATUS'])
            self.LAST_STAT = self.ser.readline().decode().strip()
            self.STATUS_LAST_CHECK = datetime.now()
        return self.LAST_STAT

    def set_status(self, pref_temp):
        current_temp = self.tempSensor.get_current_temp()
        log_message("Current temperature is: {:.2f}".format(current_temp))
        is_set = False
        if current_temp >= pref_temp + 1.0:
            if self.turn_off():
                log_message('It gets hot with {:.1f}oC - {:.1f}oC is preferred'.format(current_temp, pref_temp))
                is_set = True
        elif current_temp <= pref_temp - 0.5:
            if self.turn_on():
                log_message('It is kinda chilly with {:.1f}oC - {:.1f}oC is preferred'.format(current_temp, pref_temp))
                is_set = True
        if not is_set:
            log_message('Heater is already set to {}!'.format(self.get_status()))