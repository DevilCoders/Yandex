from functools import wraps

from nose.plugins.skip import SkipTest

def skip_if_not(value):
    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            if not value:
                raise SkipTest()
            return func(*args, **kwargs)
        return wrapper
    return decorator

