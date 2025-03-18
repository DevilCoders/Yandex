# -*- coding: utf-8 -*-
from __future__ import unicode_literals
from functools import wraps
from .state import get_state


def stop_logging(func):
    @wraps(func)
    def wrapped(*args, **kwargs):
        state = get_state()
        state.disable()
        try:
            return func(*args, **kwargs)
        finally:
            state.enable()
    return wrapped
