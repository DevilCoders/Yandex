#!/usr/bin/python

import sys
import django

from django.conf import settings
from django.core.management import call_command

settings.configure(
    DEBUG=True,
    INSTALLED_APPS=(
        'django_abc_data',
    ),
)

django.setup()
call_command('makemigrations', 'django_abc_data')