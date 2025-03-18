# coding: utf-8

from __future__ import unicode_literals

import logging
from threading import local
from functools import wraps


logger = logging.getLogger(__name__)
thread_locals = local()  # этот объект будет зависимым от треда.
                         # положенное в него можно прочесть лишь в том же треде, где положили.


def absorb(name=None, value=None):
    """
    Положить объект на холод
    @param name: ключ для хранения
    @param value: объект
    """
    logger.debug('{name} = {value} absorbed in injector'.format(name=name, value=value))
    store = getattr(thread_locals, 'dir_sync_injected_datastore', None)
    if store is None:
        thread_locals.dir_sync_injected_datastore = store = {}

    if name in list(store.keys()):
        logger.warning('Object {0} is already absorbed in the injector'.format(name))

    store[name] = value


def get_from_thread_store(name):
    """
    Получить из тред-безопасного хранилища значение или бросить RuntimeError.
    """
    if not hasattr(thread_locals, 'dir_sync_injected_datastore'):
        raise RuntimeError('You must call injector.absorb at least once first')

    store = thread_locals.dir_sync_injected_datastore
    if not name in store:
        raise RuntimeError(
            'You must absorb {0} object in injector first'.format(name)
        )

    return store[name]


def is_in_thread_store(name):
    return hasattr(thread_locals, 'dir_sync_injected_datastore') and \
           name in thread_locals.dir_sync_injected_datastore


def inject(*objects_names):
    """
    Декоратор, позволяет загонять в kwargs обернутых функций данные.

    @param objects_names: один или несколько ключей объектов, ранее положенных
                          в завязанное на тред хранилище,
                          которые надо передать в обернутую функцию.
                          Также будут использованы в качестве имен параметров.

    @raise: RuntimeError
    """
    def decorator(func):
        @wraps(func)
        def wrapper(*args, **kwargs):
            for name in objects_names:
                # уже передаваемые параметры с такими же именами не перезаписываются
                kwargs.setdefault(name, get_from_thread_store(name))

            return func(*args, **kwargs)
        return wrapper
    return decorator


def clear(*args, **kwargs):
    """
    Очистить хранилище привязанных к треду данных
    *args, **kwargs - для возможности использования в сигналах
    """
    logger.debug('injector cleared')
    if hasattr(thread_locals, 'dir_sync_injected_datastore'):
        del thread_locals.dir_sync_injected_datastore
