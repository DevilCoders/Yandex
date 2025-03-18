# coding: utf-8
from __future__ import unicode_literals

import random
import re
import six
import time

from functools import WRAPPER_ASSIGNMENTS

from django.db.utils import DatabaseError, InternalError, OperationalError, InterfaceError

from django_pgaas.compat import logger

RO_MESSAGE_REGEX = re.compile('^cannot execute (UPDATE|INSERT|DELETE) '
                              'in a read-only transaction$')


def can_retry_error(exc):
    already_retried = getattr(exc, '_django_pgaas_retried', False)

    return (not already_retried and
            (type(exc) in (DatabaseError, OperationalError, InterfaceError) or
             (type(exc) is InternalError and
              RO_MESSAGE_REGEX.match(str(exc)) is not None)))


class RandomisedBackoffRange(six.Iterator):
    """The object is an iterator that, given an integer N and a time slot T,
    produces sequence 0...N such that its second and the following items
    are produced with a randomised exponential backoff delay.
    The first item is produced immediately.

    Specifically, between the items (i-1) and (i) there will be a randomly
    chosen delay from range [0, T*(2**i-1)] with step T.

    :param max_attempts: number of items produced AFTER the first one
    :param retry_slot: exponential backoff time slot

    >>> list(RandomisedBackoffRange(5, 0.1))
    [0, 1, 2, 3, 4, 5]

    >>> from time import time
    ... start = time()
    ... for i in RandomisedBackoffRange(5, 0.1):
    ...     print('%d: %.3f' % (i, time()-start))

    0: 0.000
    1: 0.103
    2: 0.103
    3: 0.607
    4: 1.812
    5: 3.916
    """

    def __init__(self, max_attempts, retry_slot):
        self.max_attempts = max_attempts
        self.retry_slot = retry_slot
        self.attempt = None
        self.mult = None

    def __iter__(self):
        self.mult = 2
        self.attempt = None
        return self

    @property
    def is_last(self):
        # As expected, with max_attempts=None the iterator will never terminate.
        return self.attempt == self.max_attempts

    def __next__(self):

        if self.attempt is None:
            self.attempt = 0
            return self.attempt

        if self.is_last:
            raise StopIteration

        delay = random.randint(0, self.mult-1) * self.retry_slot
        time.sleep(delay)

        self.mult <<= 1
        self.attempt += 1

        return self.attempt


def logged_retry_range(label, retry_range):

    for i in retry_range:
        yield i

        if retry_range.is_last:
            logger.exception('[%d] %s failed, ran out of retries', i, label)
        else:
            logger.warning('[%d] %s failed, retrying', i, label)


def get_retry_range_for(label):
    from django_pgaas.conf import settings

    backoff = RandomisedBackoffRange(max_attempts=settings.PGAAS_RETRY_ATTEMPTS,
                                     retry_slot=settings.PGAAS_RETRY_SLOT)

    return logged_retry_range(label, backoff)


def available_attrs(func):
    """
    Copy-paste from
    https://github.com/django/django/blob/stable/1.11.x/django/utils/decorators.py#L121.

    This is required as a workaround for http://bugs.python.org/issue3445
    under Python 2.
    """
    if six.PY3:
        return WRAPPER_ASSIGNMENTS
    else:
        return tuple(a for a in WRAPPER_ASSIGNMENTS if hasattr(func, a))
