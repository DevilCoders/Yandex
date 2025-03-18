# coding: utf-8


def create_manager(backend, **kwargs):
    try:
        module = _load_backend(backend)
    except ImportError:
        raise ValueError('No backend available: %s' % backend)

    try:
        manager_class = getattr(module, 'Manager')
    except AttributeError:
        raise ValueError('No manager availabe from backend: %s' % backend)

    return manager_class(**kwargs)


def _load_backend(backend):
    return __import__('ylock.backends.%s' % backend, {}, {}, [''])
