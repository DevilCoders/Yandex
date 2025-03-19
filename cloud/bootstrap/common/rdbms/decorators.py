import functools


def force_rollback(func):
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        finally:
            kwargs["db"].conn.rollback()
    return wrapper
