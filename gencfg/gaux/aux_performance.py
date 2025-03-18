"""
    Decorator for calculating execution time of specified functions. Do not sure, why we need it, if we have cProfile.
"""

from time import time
import inspect
from config import PERFORMANCE_DEBUG


# try to replace with native profiler usage
class FuncPerfTimer(object):
    def __init__(self, op=None, begin=True, info=''):
        if op is None:
            self.op = '%s(%s)' % (inspect.stack()[1][3], info)
        else:
            self.op = op
        self.start_time = None
        self.total_time = None
        if begin:
            self.begin()

    def begin(self):
        self.start_time = time()

    def end(self):
        self.total_time = time() - self.start_time
        if PERFORMANCE_DEBUG:
            print '%s: %.2f' % (self.op, self.total_time)


def perf_timer(func):
    def decorated(*args, **kwargs):
        timer = FuncPerfTimer(op=func.__name__)
        result = func(*args, **kwargs)
        timer.end()
        return result

    return decorated
