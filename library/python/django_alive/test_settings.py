# coding: utf-8

from django_alive.settings import *

INSTALLED_APPS = (
    'django.contrib.sites',
    'django.contrib.contenttypes',
    'django.contrib.auth',
    'django_alive',
)

SECRET_KEY = 'some-secret-key'

SITE_ID = 1

DATABASES = {'default': {'ENGINE': 'django.db.backends.sqlite3', 'NAME': ':memory:'}}

MIDDLEWARE_CLASSES = (
    'django.middleware.common.CommonMiddleware',
    'django.contrib.sessions.middleware.SessionMiddleware',
    'django_yauth.middleware.YandexAuthMiddleware',
)

ALIVE_CONF = {
    'stampers': {
        'db': {
            'class': 'django_alive.stampers.DBStamper',
        },
    },
    'checks': {
        'db-slave': {
            'class': 'django_alive.checkers.DBChecker',
            'stamper': 'db',
            'alias': 'slave',
        },
        'db-master': {
            'class': 'django_alive.checkers.DBChecker',
            'stamper': 'db',
            'alias': 'default',
        },
        # 'celery': {
        #     'class': 'django_alive.checkers.CeleryWorkerPinger',
        #     'app': 'dogma.celery_app.app',
        # },
        # 'uwsgi': {
        #     'class': 'django_alive.checkers.HttpPinger',
        #     'host': 'localhost',
        #     'port': 7788,
        # }
    },
    'groups': {
        'front': [
            # 'uwsgi',
            'db-master',
            'db-slave',
            # 'celery',
        ],
        'back': [
            'db-master',
            # 'celery',
        ]
    }
}
