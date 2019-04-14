from CurrentTime import CurrentTime
from datetime import datetime
from Utilities import log_message


class Schedule:
    START_TIME_POS = 1
    END_TIME_POS = 2
    PREF_TEMP_POS = 3

    def __init__(self, schedule_file):
        self.file = schedule_file

    def get_pref_temp(self):
        ct = CurrentTime()
        with open(self.file) as f:
            for line in f:
                if not line.startswith(ct.weekday):
                    continue
                splitted_line = line.rstrip().split()
                start_time = CurrentTime.get_time(splitted_line[self.START_TIME_POS])
                end_time = CurrentTime.get_time(splitted_line[self.END_TIME_POS])
                preferred_temp = float(splitted_line[self.PREF_TEMP_POS])
                if end_time > datetime.now().time() >= start_time:
                    log_message("Processing record of {} - {} with preferred temp {:.1f}oC".format(start_time, end_time,
                                                                                                   preferred_temp))
                    return preferred_temp
        # if nothing is being returned just return a value
        return 20.0

