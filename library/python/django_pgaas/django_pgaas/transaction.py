# coding: utf-8
from __future__ import unicode_literals

import six
import sys

from functools import wraps

from django.db.transaction import atomic, get_connection
from django.db.utils import DatabaseError, InterfaceError, DEFAULT_DB_ALIAS

from django_pgaas.utils import can_retry_error, get_retry_range_for, available_attrs


class AtomicRetry(object):
    def __init__(self, using, savepoint):
        self.using = using
        self.savepoint = savepoint

    def __call__(self, func):
        @wraps(func, assigned=available_attrs(func))
        def wrapper(*args, **kwargs):

            retry_range = get_retry_range_for('Atomic block')

            for _ in retry_range:
                try:
                    block = atomic(using=self.using, savepoint=self.savepoint)(func)
                    return block(*args, **kwargs)

                except (DatabaseError, InterfaceError) as exc:
                    if not can_retry_error(exc):
                        raise

                    last_exc_info = sys.exc_info()

                    connection = get_connection(self.using)
                    connection.close()  # to avoid 'InterfaceError: connection already closed'
                    connection.ensure_connection()

            six.reraise(*last_exc_info)

        return wrapper


def atomic_retry(using=None, savepoint=True):
    if callable(using):
        return AtomicRetry(DEFAULT_DB_ALIAS, savepoint)(using)
    else:
        return AtomicRetry(using, savepoint)
