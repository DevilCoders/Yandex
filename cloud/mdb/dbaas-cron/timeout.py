"""
Simple function timeout with threading
"""

import ctypes
import sys
import threading


class FunctionTimeout(BaseException):
    """
    Function call timeout exception
    """


class Timeouter(threading.Thread):
    """
    This thread stops target with injecting FunctionTimeout into execution context
    """
    def __init__(self, target, interval):
        """
        Timeouter constuctor
        """
        threading.Thread.__init__(self)
        self.target_thread = target
        self.interval = interval
        self.daemon = True

    def run(self):
        """
        Try to stop target
        """
        while self.target_thread.is_alive():
            ctypes.pythonapi.PyThreadState_SetAsyncExc(ctypes.c_long(self.target_thread.ident),
                                                       ctypes.py_object(FunctionTimeout))
            self.target_thread.join(self.interval)


class Thread(threading.Thread):
    """
    Stopable thread
    """
    def stop(self, interval):
        """
        Stops the thread by raising exception in execution context
        """
        if not self.is_alive():
            return

        timeouter = Timeouter(self, interval)
        timeouter.start()
        timeouter.join()


def func_timeout(timeout, func, interval=1.0, args=None, kwargs=None):
    """
    Runs function with timeout
    """
    if args is None:
        args = ()
    if kwargs is None:
        kwargs = {}

    func_exception = []
    func_return = []
    timed_out = False

    def wrapper(func_args, func_kwargs):
        try:
            func_return.append(func(*func_args, **func_kwargs))
        except BaseException as exc:
            info = sys.exc_info()
            if not timed_out:
                exc.__traceback__ = info[2].tb_next
            func_exception.append(exc)

    thread = Thread(target=wrapper, args=(args, kwargs))
    thread.daemon = True

    thread.start()
    thread.join(timeout)

    if thread.is_alive():
        timed_out = True
        thread.stop(interval)
    else:
        thread.join()

    if func_exception:
        raise func_exception[0] from None

    if func_return:
        return func_return[0]

    return None
