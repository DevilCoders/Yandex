# coding: utf-8
from __future__ import unicode_literals, absolute_import


ALIVE_MIDDLEWARE_REDUCE = 'django_alive.state.default_reduce'

# Таймаут проверки в секундах
ALIVE_CHECK_TIMEOUT = 60

# Конфигурация чекеров
ALIVE_CONF = {
    'stampers': {},
    'checks': {},
    'groups': {},
}

# Паттерн урла для ручки ping
ALIVE_MIDDLEWARE_PING_PATTERN = r'^/ping/?$'

# Паттерн урла для страницы статусов
ALIVE_MIDDLEWARE_STATE_PATTERN = r'^/_alivestate/?$'
