import logging
import logging.handlers


def Setup(logFileName, level=logging.DEBUG, maxBytes=10000000, backupCount=1):
    formatter = logging.Formatter(fmt='%(asctime)s: %(levelname)s: %(message)s (%(filename)s:%(lineno)d))', datefmt="%Y-%m-%d %H:%M:%S")

    handler = logging.handlers.RotatingFileHandler(logFileName, encoding='utf8', maxBytes=maxBytes, backupCount=backupCount)

    logger = logging.getLogger()
    logger.setLevel(level)
    handler.setFormatter(formatter)
    logger.addHandler(handler)
