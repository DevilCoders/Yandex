# coding: utf-8
import warnings
from contextlib import contextmanager

from ylock.backends import create_manager


@contextmanager
def locked(name, timeout=None, block=True, backend='dispofa', **kwargs):
    '''
    Usage:

    >>> from ylock import locked
    >>> params = {
    ....    'name': 'lock_name', 'timeout': 10, 'backend': 'zookeeper',
    ....    'hosts': ['host:port']
    ....    }
    >>> with locked(**params) as acquired:
    ....    do_something()
    '''

    warnings.warn(
        '`locked` context manager is deprecated. '
        'Use Lock object semantics instead.',
        DeprecationWarning
    )

    manager = create_manager(backend, **kwargs)

    try:
        with manager.lock(name, timeout, block) as lock:
            yield lock
    finally:
        manager.close()
