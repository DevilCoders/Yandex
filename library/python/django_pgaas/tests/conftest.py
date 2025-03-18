# coding: utf-8
from __future__ import unicode_literals
from django.conf import settings


def pytest_configure():
    settings.configure(
        DATABASES={
            'default': {
                'ENGINE': 'django_pgaas.backend',
                'HOST': 'db',
                'PORT': None,
                'USER': 'postgres',
                'PASSWORD': 'postgrya',
                'NAME': 'testdb',
                'DISABLE_SERVER_SIDE_CURSORS': True,
            }
        },

        INSTALLED_APPS=[
            'tests.app',
            'django_pgaas',
        ],
        ROOT_URLCONF='tests.app.urls',
    )
