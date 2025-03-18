# coding: utf-8
from __future__ import unicode_literals

import mock
import pytest
from django.db.transaction import get_connection
from django.test.client import Client
from psycopg2._psycopg import InterfaceError

from django_pgaas.compat import BaseDatabaseWrapper, DatabaseWrapper
from django_pgaas.wsgi import IdempotentClientHandler


class FailFirst(BaseDatabaseWrapper):
    def __init__(self, *args, **kwargs):
        self.should_fail = True
        super(FailFirst, self).__init__(*args, **kwargs)

    def create_cursor(self):
        if self.should_fail:
            self.should_fail = False
            raise InterfaceError('Connection already closed')
        else:
            return super(FailFirst, self).create_cursor()


class RetryingClient(Client):
    def __init__(self, enforce_csrf_checks=False, **defaults):
        super(RetryingClient, self).__init__(enforce_csrf_checks=enforce_csrf_checks, **defaults)
        self.handler = IdempotentClientHandler(enforce_csrf_checks)


@pytest.mark.django_db(transaction=True)
def test_retry_view():
    client = RetryingClient()
    conn = get_connection('default')
    assert not conn.in_atomic_block

    # The import path depends on the version of Django
    mock_path = '%s.%s' % (DatabaseWrapper.__module__, DatabaseWrapper.__name__)

    with mock.patch(mock_path, FailFirst):
        response = client.get('/hello/')
        assert response.status_code == 200
