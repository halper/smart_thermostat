import logging


PI_COMMANDS = {
    'GET_DATA': str.encode("0"),
    'HEATER_ON': str.encode("1"),
    'HEATER_OFF': str.encode("2"),
    'TEST_HEATER': str.encode("3")
}

LOGGER = None


def get_logger():
    global LOGGER
    if LOGGER:
        return LOGGER
    else:
        logger = logging.getLogger()
        logging.basicConfig(filename='arduino.log', filemode='w+', format='%(asctime)s - %(message)s',
                            datefmt='%d-%b-%y %H:%M:%S')
        logger.setLevel(logging.INFO)
        LOGGER = logger
        return LOGGER


def log_message(message):
    print(message)
    if not LOGGER:
        get_logger()
    LOGGER.info(message)
