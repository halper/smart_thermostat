import time
from Utilities import PI_COMMANDS, log_message
from SensorData import SensorData


class TempSensor:

    def __init__(self, ser):
        self.ser = ser

    def get_current_temp(self):
        sensor_data = self.get_sensor_data()
        avg_temp = sensor_data.get_hi()
        for i in range(0, 9):
            avg_temp += self.get_sensor_data().get_hi()
        log_message(sensor_data.print_sensor_data())
        return avg_temp / 10.0

    def get_sensor_data(self):
        # Sensor data is being printed as
        # ADDR HUM TEMP HEAT_INDEX - separated with tab
        self.ser.write(PI_COMMANDS['GET_DATA'])
        time.sleep(3)
        sensor_raw_data = self.ser.readline().decode().strip()
        sensor_data = SensorData('{}'.format(sensor_raw_data))
        return sensor_data
