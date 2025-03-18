# -*- coding: utf-8 -*-

# Please DO NOT INSERT Sandbox specific code to this module

import logging
from time import sleep


def retries(max_tries, delay=1, backoff=2, exceptions=(Exception,), hook=None, log=True, raise_class=None):
    """
        Wraps function into subsequent attempts with increasing delay between attempts.
        Adopted from https://wiki.python.org/moin/PythonDecoratorLibrary#Another_Retrying_Decorator
    """
    def dec(func):
        def f2(*args, **kwargs):
            current_delay = delay
            for n_try in xrange(0, max_tries + 1):
                try:
                    return func(*args, **kwargs)
                except exceptions as e:
                    if n_try < max_tries:
                        if log:
                            logging.exception(
                                "Error in function %s on %s try:\n%s\nWill sleep for %s seconds...",
                                func.__name__, n_try, e, current_delay
                            )
                        if hook is not None:
                            hook(n_try, e, current_delay)
                        sleep(current_delay)
                        current_delay *= backoff
                    else:
                        logging.error("Max retry limit %s reached, giving up with error:\n%s", n_try, e)
                        if raise_class is None:
                            raise
                        else:
                            raise raise_class(
                                "Max retry limit {} reached, giving up with error: {}".format(n_try, str(e))
                            )

        return f2
    return dec


# TODO: use functools.lru_cache decorator in python3
def memoize(func):
    cache = {}

    def wrap(*args):
        if args not in cache:
            cache[args] = func(*args)
        return cache[args]

    return wrap
