from datetime import datetime


class CurrentTime:

    def __init__(self):
        now = datetime.now()
        self.weekday = now.strftime('%w')
        self.hour = now.strftime('%H')
        self.minute = now.strftime('%M')

    @staticmethod
    def get_time(time_string):
        hour, minute = time_string.split(':')
        hour = int(hour)
        minute = int(minute)
        return datetime.time(hour=hour, minute=minute)
