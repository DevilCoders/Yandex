import logging
import functools


logger = logging.getLogger(__name__)


def locked(name, manager, timeout=None, block=None, block_timeout=None):
    def _decorator(func):
        @functools.wraps(func)
        def _wrapper(*args, **kwargs):
            logger.info('Acquiring lock `%s`', name)
            lock = manager.lock(name, timeout=timeout, block=block, block_timeout=block_timeout)
            with lock as is_locked_successfully:
                if is_locked_successfully:
                    logger.info('Lock `%s` acquired', name)
                    return func(*args, **kwargs)
                else:
                    logger.info('Lock `%s` NOT acquired', name)

        return _wrapper
    return _decorator
