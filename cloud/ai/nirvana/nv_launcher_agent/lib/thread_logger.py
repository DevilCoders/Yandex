import os
import sys
import threading

from datetime import datetime
from inspect import currentframe, getframeinfo
from collections import defaultdict

class UnknownThreadException(Exception):
    def __init__(self, message):
        self.message = message


def get_default_logger():
    return sys.stdout


class ThreadLogger:
    LOG_FILES = defaultdict(lambda: get_default_logger())

    @staticmethod
    def register_thread(log_file):
        pass

    @staticmethod
    def __check_thread_is_known():
        ident = threading.currentThread().ident
        if ident not in ThreadLogger.LOG_FILES:
            raise UnknownThreadException('Attempt to print from unknown thread with ident = ' + str(ident))
        return ident

    @staticmethod
    def print(file, level, message):
        frame_info = getframeinfo(currentframe().f_back.f_back)
        time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        file_name = os.path.basename(frame_info.filename)
        line_no = frame_info.lineno
        file.write('[%s] {%20s:%4d} %7s - %s\n' % (time, file_name, line_no, level, message))

    @staticmethod
    def debug(message):
        ident = threading.currentThread().ident
        return ThreadLogger.print(ThreadLogger.LOG_FILES[ident], 'DEBUG', message)

    @staticmethod
    def info(message):
        ident = threading.currentThread().ident
        return ThreadLogger.print(ThreadLogger.LOG_FILES[ident], 'INFO', message)

    @staticmethod
    def warning(message):
        ident = threading.currentThread().ident
        return ThreadLogger.print(ThreadLogger.LOG_FILES[ident], 'WARNING', message)

    @staticmethod
    def error(message):
        ident = threading.currentThread().ident
        return ThreadLogger.print(ThreadLogger.LOG_FILES[ident], 'ERROR', message)
