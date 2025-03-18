# -*- coding: utf-8 -*-
from __future__ import unicode_literals

INSTALLED_APPS = (
    'django.contrib.sites',
    'django.contrib.contenttypes',
    'django.contrib.auth',

    'django_abc_data',
)

SITE_ID = 1
SECRET_KEY = 'some-secret-key'
DATABASES = {'default': {'ENGINE': 'django.db.backends.sqlite3'}}
TIME_ZONE = 'Europe/Moscow'
USE_TZ = True

ABC_DATA_IDS_USER_AGENT = 'some_user_agent'
ABC_DATA_IDS_OAUTH_TOKEN = 'some_token'
