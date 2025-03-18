# encoding: utf-8
import glob
import re
import os
import traceback

import six

is_arcadia = False
try:
    from library.python import resource
    is_arcadia = True
except ImportError:
    pass


def _find_entry_point():
    # Немного магии: найдём ближайший вызывающий нас модуль,
    # путь к которому отличается от пути к нашему модулю
    thisdir = os.path.split(os.path.abspath(__file__))[0]
    stack = traceback.extract_stack()
    for i in range(-1, -1*len(stack), -1):
        module_path = stack[i][0]
        if os.path.split(module_path)[0]!=thisdir:
            return module_path


def _get_config_paths(path, _custom_environments):
    return [
        os.path.join(path, _environment or '', '') for _environment
        in [None] + list(filter(None, _custom_environments))
    ]


def _collect_config_files_for_arcadia(envtype, _environment_settings_dir_paths):
    _configs = []
    envtypes = '|'.join('({})'.format(env) for env in envtype)
    envtypes_re = '(\.((local)|{}))?'.format(envtypes)
    for _environment_settings_dir_path in _environment_settings_dir_paths:
        environment_settings_re = '^{}[^/]+\.conf{}$'.format(_environment_settings_dir_path, envtypes_re)
        for file_name in resource.resfs_files():
            if re.match(environment_settings_re, file_name):
                _configs.append(file_name)

    _configs.sort(key=lambda f: os.path.split(f)[1])
    return _configs


def _collect_config_files(envtype, _environment_settings_dir_paths):
    _configs = []
    for _environment_settings_dir_path in _environment_settings_dir_paths:
        _configs += glob.glob(os.path.join(_environment_settings_dir_path, '*.conf'))
        for _env in envtype:
            _configs += glob.glob(os.path.join(_environment_settings_dir_path, '*.conf.%s' % _env))
        _configs += glob.glob(os.path.join(_environment_settings_dir_path, '*.conf.local'))
    _configs.sort(key=lambda f: os.path.split(f)[1])
    return _configs


def _get_config_files(path=None, envtype=None):
    """
    Функция возвращает список конфигурационных файлов и пару "глобальных" настроек

    :param path: где искать конфигурационные файлы, string. Если None, то искать в директории settings
    :param envtype: названия окружений (например: 'development'). string или list/tuple
    """

    if not path:
        _settings_module_path = _find_entry_point()
        #_settings_module_path = os.path.abspath(traceback.extract_stack()[-4][0])
        path = os.path.splitext(_settings_module_path)[0]
        root_path = os.path.split(_settings_module_path)[0]
    else:
        root_path = os.path.split(path)[0]

    if envtype is None:
        import yenv
        envtype = list(yenv.chain_type())
        envtype.reverse()
    else:
        if isinstance(envtype, six.string_types):
            envtype = [envtype, ]

    _custom_environments = os.environ.get('granular_environments', '').split(',')
    _environment_settings_dir_paths = _get_config_paths(path, _custom_environments)
    if is_arcadia:
        _configs = _collect_config_files_for_arcadia(
            envtype=envtype,
            _environment_settings_dir_paths=_environment_settings_dir_paths,
        )
    else:
        _configs = _collect_config_files(
            envtype=envtype,
            _environment_settings_dir_paths=_environment_settings_dir_paths,
        )

    # Глобальные настройки
    _project_path = os.path.split(os.path.abspath(traceback.extract_stack()[0][0]))[0]
    G = {'SETTINGS_ROOT_PATH': root_path,
         'PROJECT_PATH': _project_path,
         'PROJECT': os.path.split(_project_path)[-1]}

    if is_arcadia:
        return _configs, G
    return list(map(os.path.abspath, _configs)), G


