import time
from Utilities import PI_COMMANDS
from SensorData import SensorData


class TempSensor:

    def __init__(self, ser):
        self.ser = ser
        self.temperature_readings = []

    def get_current_temp(self):
        sensor_data = self.get_sensor_data()
        return sensor_data.get_hi()

    def get_avg_temp(self):
        try:
            avg_reading = sum(self.temperature_readings) / len(self.temperature_readings)
            self.temperature_readings = []
            return avg_reading
        except Exception:
            return self.get_current_temp()

    def get_sensor_data(self):
        # Sensor data is being printed as
        # ADDR HUM TEMP HEAT_INDEX - separated with tab
        self.ser.write(PI_COMMANDS['GET_DATA'])
        time.sleep(3)
        sensor_raw_data = self.ser.readline().decode().strip()
        sensor_data = SensorData('{}'.format(sensor_raw_data))
        return sensor_data
