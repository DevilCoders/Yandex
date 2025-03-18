# encoding: utf-8

__all__ = ['Config', 'get', 'set', 'from_envvar']

import os
import six
from .utils import _get_config_files, is_arcadia

if is_arcadia:
    from .utils import resource


class Config(dict):
    """
    Чтобы обращаться к переменным как к аттрибутам класса или элементам словаря:
    config.VARIABLE
    config['VARIABLE']
    config.get('VARIABLE')
    """

    def __init__(self, defaults=None):
        dict.__init__(self, defaults or {})

    def __getattr__(self, k):
        if k in self:
            return self.get(k)
        else:
            raise AttributeError('%s instance has no attribute %s' % (self.__class__.__name__, k))

    def __setattr__(self, k, v):
        self[k] = v

    def get_namespace(self, namespace, lowercase=True):
        """Returns a dictionary containing a subset of configuration options
        that match the specified namespace/prefix. Example usage::

            config['IMAGE_STORE_TYPE'] = 'fs'
            config['IMAGE_STORE_PATH'] = '/var/app/images'
            config['IMAGE_STORE_BASE_URL'] = 'http://img.website.com'
            image_store_config = config.get_namespace('IMAGE_STORE_')

        The resulting dictionary `image_store` would look like::

            {
                'type': 'fs',
                'path': '/var/app/images',
                'base_url': 'http://img.website.com'
            }

        This is often useful when configuration options map directly to
        keyword arguments in functions or class constructors.

        :param namespace: a configuration namespace
        :param lowercase: a flag indicating if the keys of the resulting
                          dictionary should be lowercase

        """
        rv = {}
        for k, v in six.iteritems(self):
            if not k.startswith(namespace):
                continue
            key = k[len(namespace):]
            if lowercase:
                key = key.lower()
            rv[key] = v
        return rv

    def __repr__(self):
        return '<%s %s>' % (self.__class__.__name__, dict.__repr__(self))


def get(**kw):
    """
    Возвращает "словарь настроек".

    Параметры (см. также описание _get_configs):
    path - опциональный
    envtype - опциональный

    Пример использования:
    import granular_settings
    globals().update(granular_settings.get())
    globals().update(granular_settings.get(path='/etc/myproject'))
    """
    initial_globals = kw.pop('initial_globals', {})
    files, G = _get_config_files(**kw)
    G.update(initial_globals)
    for f in files:
        if is_arcadia:
            exec(compile(resource.resfs_read(f), f, 'exec'), G)
        else:
            if six.PY2:
                execfile(f, G)
            else:
                exec(compile(open(f).read(), f, 'exec'), G)
    return Config(defaults=G)


def from_envvar(variable_name, silent=False, **kw):
    """Loads a configuration from an environment variable pointing to
    a configuration directory.  This is basically just a shortcut with nicer
    error messages for this line of code::

        granular_settings.get(path=os.environ['YOURAPPLICATION_SETTINGS'])

    :param variable_name: name of the environment variable
    :param silent: set to `True` if you want silent failure for missing
                   files.
    :return: bool. `True` if able to load config, `False` otherwise.
    """
    rv = os.environ.get(variable_name)
    if not rv:
        if silent:
            return False
        raise RuntimeError('The environment variable %r is not set '
                           'and as such configuration could not be '
                           'loaded.  Set this variable and make it '
                           'point to a configuration file' %
                           variable_name)
    return get(path=rv, **kw)

def set(globals, **kw):
    """
    Присваивает настройки словарю globals.

    Параметры:
    globals - обязательный
    path - опциональный
    envtype - опциональный

    Пример использования:
    import granular_settings
    granular_settings.set(globals(), path='/etc/myproject')
    """
    globals.update(get(initial_globals=globals, **kw))


