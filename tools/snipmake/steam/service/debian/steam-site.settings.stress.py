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

try:
    from local.ui import *
except ImportError:
    pass
