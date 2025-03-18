# coding: utf-8

import functools
import logging

import ylock
from django.conf import settings

logger = logging.getLogger(__name__)

lock_manager = ylock.create_manager(**settings.YLOCK)


def _execute_with_lock(lock_name, callable):
    with lock_manager.lock(lock_name, block=False) as acquired:
        if acquired:
            return callable()
        else:
            logger.info('Did not get lock "%s"', lock_name)


def get_lock_or_do_nothing(lock_name):
    def wrap(function):
        @functools.wraps(function)
        def do(*args, **kwargs):
            _execute_with_lock(
                lock_name,
                functools.partial(function, *args, **kwargs),
            )

        return do

    return wrap
