import logging
import os
import threading
import functools
import signal


class WdWrapper(object):
    def __init__(self, tout):
        self.tout = tout

    def __call__(self, func):
        @functools.wraps(func)
        def g(*args, **kwargs):
            with Watchdog(self.tout, func.__name__):
                return func(*args, **kwargs)

        return g


def with_watchdog(tout):
    return WdWrapper(tout)


class Watchdog(object):
    def __init__(self, tout, name):
        self._tout = tout
        self._ev = threading.Event()
        self._name = name

    def drop(self):
        try:
            logging.error('%s watchdog timed out', self._name)
            os.kill(0, signal.SIGTERM)
        finally:
            os._exit(-1)

    def __enter__(self):
        def handler():
            if not self._ev.wait(self._tout):
                self.drop()

        self._thr = threading.Thread(target=handler)
        self._thr.start()

        return self

    def release(self):
        self._ev.set()

    def __exit__(self, *args):
        self.release()
        self._thr.join()
