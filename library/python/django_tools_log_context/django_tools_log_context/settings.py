# coding: utf-8

from __future__ import unicode_literals
import os


TOOLS_LOG_CONTEXT_PROVIDERS = [
    'django_tools_log_context.provider.request',
    'django_tools_log_context.provider.auth',
    # 'django_tools_log_context.provider.b2b',
    'django_tools_log_context.provider.endpoint',
]
TOOLS_LOG_CONTEXT_REQUESTS_MODULES = [
    'requests.sessions',
    'yt.packages.requests.sessions',
]
# Список заголовков для логирования
# string: имя заголовка
# bool: True - выводить в полученном виде, False - менять на звездочки (заголовки авторизации)
#    [
#      ('REMOTE_ADDR', True),
#      ('HTTP_AUTHORIZATION', False),
#    ]
TOOLS_LOG_CONTEXT_ALLOWED_HEADERS = [
]
TOOLS_LOG_CONTEXT_ENABLE_TRACKING = os.environ.get('YLOG_TRACKING', '1').lower() in ('1', 'true', 'yes')
TOOLS_LOG_CONTEXT_ENABLE_DB_TRACKING = True
TOOLS_LOG_CONTEXT_ENABLE_HTTP_TRACKING = True
TOOLS_LOG_CONTEXT_ENABLE_HTTP_400_WARNING = False
TOOLS_LOG_CONTEXT_ENABLE_STACKTRACES = False
TOOLS_LOG_CONTEXT_RESPONSE_MAX_SIZE = 0  # characters
TOOLS_LOG_CONTEXT_SQL_WARNING_THRESHOLD = 500  # msec
TOOLS_LOG_CONTEXT_ALWAYS_SUBSTITUTE_SQL_PARAMS = False
