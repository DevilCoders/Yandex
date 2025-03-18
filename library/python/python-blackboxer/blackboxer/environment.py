# encoding: utf-8

"""
Известные адреса ЧЯ

подробнее: https://doc.yandex-team.ru/blackbox/concepts/blackboxLocation.xml
"""

import yenv

ENV = {
    'production': 'http://blackbox.yandex.net/blackbox/',
    'intranet': 'http://blackbox.yandex-team.ru/blackbox/',
    'test': 'http://pass-test.yandex.ru/blackbox/',
    'development': 'http://blackbox-mimino.yandex.net/blackbox/',
    'model143': 'http://blackbox.ya-test.ru/blackbox/',
    'ipv6': 'http://blackbox-ipv6.yandex.net/blackbox'
}

YENV_DICT = {
    'intranet': {
        # в интранете один ЧЯ
        'production': ENV['intranet'],
        'testing': ENV['intranet'],
        'development': ENV['intranet'],
        'rc': ENV['intranet']
    },
    'other': {
        # в тестинге тестовый ЧЯ
        'production': ENV['production'],
        'testing': ENV['development'],
        'development': ENV['development'],
    },
    'localhost': {
        # Считаем что на локалхосте есть дырки до обычных инстансов
        'production': ENV['production'],
        'testing': ENV['development'],
        'development': ENV['development'],
    },
    'beta': {
        # бета - обычно нужен стандарный паспорт
        'production': ENV['production'],
        'testing': ENV['development'],
        'development': ENV['development'],
    },
    'model143': {
        # Для модели яндекса в масштабе 1:43
        'production': ENV['model143'],
        'testing': ENV['model143'],
        'development': ENV['model143'],
    },
    'stress': {
        'stress': ENV['development']
    }
}


def url_from_yenv(yenv_name=None, yenv_type=None):
    """
    Returns blackbox environment based on yenv data.

    :param env_cls: environment class
    :param yenv_name: use this name instead of yenv.name
    :param yenv_type: use this type instead of yenv.type
    :param kw: additional parameters for environment constructor
    :return: initialised env_cls instance

    yenv:https://github.yandex-team.ru/common-python/yenv

    """
    if not yenv_name:
        yenv_name = yenv.choose_name(YENV_DICT.keys(), fallback=True)
    if not yenv_type:
        yenv_type = yenv.choose_type(YENV_DICT[yenv_name].keys(), fallback=True)

    return YENV_DICT[yenv_name][yenv_type]


URL = url_from_yenv()
