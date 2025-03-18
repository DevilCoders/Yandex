DEBUG = True
TEMPLATE_DEBUG = DEBUG

DATABASES = {
    'default': {
        'ENGINE': 'django.db.backends.mysql',
        'NAME': 'steam',
        'USER': 'steam',
        'PASSWORD': 'steam',
        'OPTIONS': {
            'init_command':
            'SET names utf8, storage_engine=INNODB, SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE',
        },

        'HOST': '',
        'PORT': '',
    }
}

ANG_FRONT = 'http://ang2.dumdum.yandex-team.ru'
ASK_ANG_TASKPOOL_AVAILABILITY = False

try:
    from local.ui import *
except ImportError:
    pass
