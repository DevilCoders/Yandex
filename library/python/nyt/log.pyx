# -*- coding: utf-8 -*-
# cython: c_string_type=str, c_string_encoding=utf-8

from util.generic.string cimport TStringBuf
from util.system.types cimport i32
from libcpp cimport bool

import logging

logger = logging.getLogger('NYT')

cdef extern from "<library/python/nyt/native/log.h>" namespace "NYTPython" nogil:
    void InstallPythonLogger() except +;
    void SetLoggerCallback(bool (*)(i32), void (*)(i32, TStringBuf)) except +;


cdef bool logging_check_level_callback(i32 level) with gil:
    return logger.isEnabledFor(level)


cdef void logging_log_callback(i32 level, TStringBuf text) with gil:
    logger.log(level, text)


def init_logging():
    InstallPythonLogger()
    SetLoggerCallback(
        logging_check_level_callback,
        logging_log_callback,
    )
