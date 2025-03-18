# coding: utf-8
from __future__ import unicode_literals

from django.core.management import call_command
from .app.models import RandomModel

import pytest


@pytest.mark.django_db(transaction=False)
def test_db_command():

    call_command(
        'db', "INSERT INTO app_randommodel (name) VALUES ('foobar');"
    )
    assert RandomModel.objects.filter(name='foobar').exists()
