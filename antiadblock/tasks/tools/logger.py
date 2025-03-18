from os import getenv
import logging


def create_logger(logger_name):
    log_level = logging.getLevelName(getenv('log_level', 'DEBUG'))
    logger = logging.getLogger(logger_name)
    logger.setLevel(log_level)
    if not logger.handlers:
        console_handler = logging.StreamHandler()
        console_handler.setFormatter(logging.Formatter('%(levelname)s [%(asctime)s]  %(message)s'))
        console_handler.setLevel(log_level)
        logger.addHandler(console_handler)
    return logger
