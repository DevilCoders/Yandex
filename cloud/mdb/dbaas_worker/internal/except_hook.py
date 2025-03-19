import sys
import threading
import logging
import traceback

__is_set = False
__hook_lock = threading.Lock()


def set_threading_except_hook():
    """
    Set sys.excepthook as threading.excepthook

    Raven (and sentry-python too) capture unhandled exceptions, but it doesn't work for threads.
    Cause sys.excepthook doesn't work for threads (https://bugs.python.org/issue1230540).
    It is fixed in python3.8 by adding threading.execepthook,
    but raven and sentry-python (at that moment) don't set it.
    """

    global __is_set
    with __hook_lock:
        if __is_set:
            return

        # don't store and use original threading.excepthook, cause it prints to stdout and breaks our logs
        def thread_except_hook(args):
            sys.excepthook(args.exc_type, args.exc_value, args.exc_traceback)
            logging.getLogger('unhandled-exception').error(
                'unhandled exception in thread %r: exc %s: trace: %s',
                args.thread,
                args.exc_value,
                '\n'.join(traceback.format_tb(args.exc_traceback)),
            )

        threading.excepthook = thread_except_hook
