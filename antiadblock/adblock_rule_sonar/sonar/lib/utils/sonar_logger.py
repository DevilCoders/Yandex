import logging
from os import getenv


log_format = '%(levelname)s [%(asctime)s]  %(message)s'
log_level = logging.getLevelName(getenv('log_level', 'DEBUG'))

console_handler = logging.StreamHandler()
console_handler.setFormatter(logging.Formatter(log_format))
console_handler.setLevel(log_level)

logger = logging.getLogger('sonar_logger')
logger.addHandler(console_handler)
logger.setLevel(log_level)
