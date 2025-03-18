DEBUG = False
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

ALLOWED_HOSTS = ['steam-prestable.yandex-team.ru']
