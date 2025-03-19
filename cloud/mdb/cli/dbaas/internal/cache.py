import os
import time
from functools import wraps


def cached(file, ttl):
    """
    Decorator that caches function result in file.
    """

    def _load():
        try:
            file_path = os.path.expanduser(file)

            age = time.time() - os.path.getmtime(file_path)
            if age > ttl:
                return None

            with open(file_path, 'r') as f:
                return f.read().strip()

        except Exception:
            pass

        return None

    def _save(value):
        try:
            file_path = os.path.expanduser(file)
            dir_path = os.path.dirname(file_path)

            os.makedirs(dir_path, exist_ok=True)

            with open(file_path, 'w') as f:
                f.write(value)

        except Exception as e:
            print(f'Warning: {e}')

    def _decorator(fun):
        @wraps(fun)
        def wrapper(*args, **kwargs):
            value = _load()
            if value:
                return value

            value = fun(*args, **kwargs)

            _save(value)

            return value

        return wrapper

    return _decorator
