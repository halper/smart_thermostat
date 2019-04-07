class SensorData:

    def __init__(self, raw_data):
        print(raw_data.rstrip().split())
        self.addr, hum, temp, hi = raw_data.rstrip().split()
        self.hum = float(hum)
        self.temp = float(temp)
        self.hi = float(hi)

    def get_addr(self):
        return self.addr

    def get_temp(self):
        return self.temp

    def get_hum(self):
        return self.hum

    def get_hi(self):
        return self.hi