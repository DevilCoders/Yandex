# coding=utf-8

import logging

import library.python.init_log

_L = logging.getLogger(__name__)

if '__main__' == __name__:
    library.python.init_log.init_log(level='DEBUG', wrap_width=80)

    _L.critical('critical message')
    _L.error('error message')
    _L.warning('warning message')
    _L.info('info message')
    _L.debug('debug message')
    _L.debug('long debug message: ' + str(list(range(1000000, 1000030))))
