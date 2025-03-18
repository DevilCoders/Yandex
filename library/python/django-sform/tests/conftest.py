# coding: utf-8

import pytest

from django.conf import settings

from .fixtures import form_cls  # noqa


pytestmark = pytest.mark.django_db


def pytest_configure():
    settings.configure(
        DATABASES={'default': {'ENGINE': 'django.db.backends.sqlite3'}, },
        ROOT_URLCONF='tests._test_urls',
    )
