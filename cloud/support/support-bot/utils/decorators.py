#!/usr/bin/env python3
"""This module contains decorate funcs."""

import time
import logging

from decorator import decorate
from functools import wraps

logger = logging.getLogger(__name__)


def log_msg(func):
    """Add system messages about telegram user to logs."""
    @wraps(func)
    def wrapper(upd, ctx, *args, **kwargs):
        name = upd.message.chat.username
        chat_id = upd.message.chat.id
        message = upd.message.text
        logger.info(f'Message: {message} from user {name} in chat {chat_id}')
        return func(upd, ctx, *args, **kwargs)

    return wrapper


def log_debug(func, *args, **kwargs):
    """Add debug messages to logger."""
    logger = logging.getLogger(func.__module__)

    def decorator(self, *args, **kwargs):
        logger.debug(f'Entering {func.__name__}')
        result = func(*args, **kwargs)
        logger.debug(vars())
        logger.debug(result)
        logger.debug(f'Exiting {func.__name__}')
        return result

    return decorate(func, decorator)


def retry(exceptions, tries=4, delay=5, backoff=2, log=True):
    """Decorator to retry the execution of the func, if you have received the errors listed."""
    def retry_decorator(func):
        func_name = f'{func.__module__}: {func.__name__}'
        @wraps(func)
        def func_retry(*args, **kwargs):
            mtries, mdelay = tries, delay
            counter = 0
            while mtries > 1:
                try:
                    counter += 1
                    return func(*args, **kwargs)
                except exceptions as e:
                    msg = f'Connection error in {func_name}. Retrying ({counter}) in {mdelay} seconds...'
                    logger.warning(msg) if log else print(msg)
                    time.sleep(mdelay)
                    mtries -= 1
                    mdelay *= backoff
            return func(*args, **kwargs)
        return func_retry
    return retry_decorator
