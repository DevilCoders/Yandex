#!/usr/bin/env python
import logging

logger = logging.getLogger(__name__)

LOG_LEVEL = logging.DEBUG
LOG_FORMAT = "%(asctime)s - %(levelname)s - %(message)s"


def main():
    console_handler = logging.StreamHandler()
    console_handler.setFormatter(logging.Formatter(LOG_FORMAT))
    console_handler.setLevel(LOG_LEVEL)
    logger.addHandler(console_handler)
    logger.setLevel(LOG_LEVEL)
    lib_logger = logging.getLogger("ycinfra")
    lib_logger.addHandler(console_handler)
    lib_logger.setLevel(LOG_LEVEL)


if __name__ == "__main__":
    main()
