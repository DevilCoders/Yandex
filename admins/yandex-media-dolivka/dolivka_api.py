#!/usr/bin/python3
"""Dolivka dom0 CADMIN-4880"""
import logging
from media_dolivka.buhlo import Buhlo
from media_dolivka.config import load_config


def main():
    """Main"""

    # setup defaults

    """
    log = '/var/log/dolivka/dolivka-api.log'
    handler = logging.FileHandler(log)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    handler.setFormatter(formatter)

    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)
    logger.addHandler(handler)
    logging.basicConfig()
    """
    config = load_config()
    log = logging.getLogger("main")
    log.debug(config.toDict().__str__())

    app = Buhlo(config.dolivka)
    app.run()

if __name__ == '__main__':
    main()
