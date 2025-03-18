# coding: utf-8

import os
import imp
import warnings
import logging


log = logging.getLogger(__name__)


class DatasourcesConfigWrapper(object):
    """
    Обертка над datasources файлом.

    """
    DEFAULT_DATASOURCES_PATH = '/etc/yandex/tools-datasources/datasources.py'

    def __init__(
        self, prefix,
        local_datasources_path=None,
        use_default=False,
        suppress_warning=False,
        fallback_on_env_vars=None,
    ):
        """
        @type prefix: str
        @type local_datasources_path: basestring
        @param prefix: префикс для имен настроек в datasources,
            например "wiki_evaluation"
        @param local_datasources_path: путь до модуля, в котором лежат
            локальные настройки
        @param suppress_warning: включает подавление предупреждений
            об отсутствии файла или настройки
        @param use_default: позволяет использовать настройки без префикса,
            благодаря этому можно сильно уменьшить размер файла секретов,
            если в нем делаются настройки для разных типов приложения
        @param fallback_on_env_vars: позволяет в случае отсутствия нужного
            ключа в файле попытаться взять его из переменных окружения
            (режим включен по умолчанию если приложение развертывается в Deploy)
        """
        self._use_default = use_default
        self._suppress_warning = suppress_warning
        self._datasources_prefix = prefix
        self._datasources_module = None
        if fallback_on_env_vars is None:
            fallback_on_env_vars = 'DEPLOY_BOX_ID' in os.environ
        self._fallback_on_env_vars = fallback_on_env_vars

        for path in (local_datasources_path, self.DEFAULT_DATASOURCES_PATH):
            if path is None:
                continue
            try:
                self._datasources_module = imp.load_source('datasources', path)
            except IOError:
                pass
            else:
                break

        if self._datasources_module is None:
            if not self.suppress_warning():
                warnings.warn('Datasources file is missing', Warning)
                log.warn('Datasources file is missing')

    def __getattr__(self, item):
        """
        Вернуть настройку из datasources или None.
        """
        keys = []
        if self._datasources_prefix:
            keys.append(self._datasources_prefix + '_' + item)
            if self._use_default:
                keys.append(item)
        else:
            keys.append(item)
        for key in keys:
            try:
                return getattr(self._datasources_module, key)
            except AttributeError:
                if self._fallback_on_env_vars:
                    value = os.environ.get(key) or os.environ.get(key.upper())
                    if value:
                        return value

                if not self.suppress_warning():
                    log.warn('Cannot get setting `%s` from datasources', key)

    def suppress_warning(self):
        return (self._suppress_warning or
                os.environ.get('SUPPRESS_DATASOURCES_WARNING'))
