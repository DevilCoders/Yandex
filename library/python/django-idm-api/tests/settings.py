from __future__ import unicode_literals
SECRET_KEY = 'some-secret-key'

INSTALLED_APPS = (
    'django.contrib.sites',
    'django.contrib.contenttypes',
    'django.contrib.auth',

    'django_idm_api',
)

SITE_ID = 1

DATABASES = {'default': {'ENGINE': 'django.db.backends.sqlite3'}}

ROOT_URLCONF = 'django_idm_api.tests.urls'

IDM_INSTANCE = 'testing'
