# coding: utf-8

from django_yauth.settings import *  # noqa

# Специально созданный Яндекс-пользователь
TEST_YANDEX_LOGIN = 'whitebox-tester'
TEST_YANDEX_PASSWORD = 'jndthnrjq d uytplj'
TEST_YANDEX_UID = 30665115
TEST_YANDEX_SESSION_ID = '3:1398674527.5.0.1398411719000:mc4tBQ:60.0|1120000000008011.0.2|55405.716002.GN2ElXcQicSIsrkmz-DZdGJ_Cxo'
TEST_YANDEX_SESSION_ID2 = '3:1398674527.5.0.1398411719000:mc4tBQ:60.0|1120000000008011.0.2|55405.716002.GN2ElXcQicSIsrkmz-DZdGJ_Cxo'
TEST_YANDEX_SESSGUARD = '3:1398674527.5.0.1398411719000:mc4tBQ:60.0|1120000000008011.0.2|55405.716002.GN2ElXcQicSIsrkmz-DZdGJ_Cxq'
TEST_YANDEX_OAUTH_TOKEN = '1234567890foobaroauthtoken123456'
TEST_YANDEX_APPLICATION_NAME = 'TestApplication'

# Если БЯ дать не .yandex.ru, он ничего не вернёт
TEST_HOST = 'localhost.yandex.ru'
TEST_IP = '127.0.0.77'
YAUTH_TYPE = 'desktop'

INSTALLED_APPS = [
    'django.contrib.sites',
    'django.contrib.contenttypes',
    'django.contrib.auth',
    'django_template_common',
    'django_yauth',
]

SECRET_KEY = 'some-secret-key'

SITE_ID = 1

DATABASES = {'default': {'ENGINE': 'django.db.backends.sqlite3', 'NAME': ':memory:'}}

# TEMPLATE_CONTEXT_PROCESSORS=('django_yauth.context.yauth', 'django.template.context_processors.request')

MIDDLEWARE_CLASSES = (
    'django.middleware.common.CommonMiddleware',
    'django.contrib.sessions.middleware.SessionMiddleware',
    'django_yauth.middleware.YandexAuthMiddleware',
)

ROOT_URLCONF = 'django_yauth.urls'
TEMPLATES = [
    {
        'BACKEND': 'library.python.django.template.backends.arcadia.ArcadiaTemplates',
        'OPTIONS': {
            'debug': False,
            'loaders': [
                'library.python.django.template.loaders.resource.Loader',
                'library.python.django.template.loaders.app_resource.Loader',
            ],
            'context_processors': [
                'django.template.context_processors.debug',
                'django.template.context_processors.request',
                'django.contrib.auth.context_processors.auth',
                'django.contrib.messages.context_processors.messages',
            ],
        },
    }
]
FORM_RENDERER = 'library.python.django.template.backends.forms_renderer.ArcadiaRenderer'
ALLOWED_HOSTS = ['localhost.yandex.ru', 'localhost.yandex.com', 'foo2.bar2']  # >= django 1.11
