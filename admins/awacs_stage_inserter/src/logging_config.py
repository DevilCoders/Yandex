#!/usr/bin/env python

import logging


class LoggingConfig(object):
    def __init__(self, instance):
        self.instance = instance

    def get_logger(self):
        self.logger = logging.getLogger(self.instance)
        self.logger.setLevel(logging.DEBUG)
        ch = logging.StreamHandler()
        formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
        ch.setFormatter(formatter)
        self.logger.addHandler(ch)
        return self.logger
