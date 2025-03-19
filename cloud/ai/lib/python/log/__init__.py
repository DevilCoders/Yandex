import logging
import os
import sys

logging.basicConfig(
    format='%(asctime)s %(name)s:%(funcName)s:%(lineno)d %(levelname)s: %(message)s',
    level=os.environ.get('LOGLEVEL', 'INFO').upper(),
    stream=sys.stdout,
)


def get_logger(logger_name):
    return logging.getLogger(logger_name)
