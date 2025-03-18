# coding: utf-8
from __future__ import unicode_literals

import django
from django.core.handlers.wsgi import WSGIHandler
from django.test.client import ClientHandler
from django.db import connections
from django_pgaas import transaction


class RetryingMixin(object):
    def make_view_atomic(self, view):
        non_atomic_requests = getattr(view, '_non_atomic_requests', set())
        for db in connections.all():
            if (db.settings_dict['ATOMIC_REQUESTS']
                    and db.alias not in non_atomic_requests):
                view = transaction.atomic_retry(using=db.alias)(view)
        return view


class IdempotentWSGIHandler(RetryingMixin, WSGIHandler):
    """Idempotent WSGI Handler for brave people who have no non-db side effects in their views"""


class IdempotentClientHandler(RetryingMixin, ClientHandler):
    """Idempotent test handler for brave people who have no non-db side effects in their views
    and still want to test their views"""


def get_wsgi_application():
    """
    The public interface to Django's WSGI support. Should return a WSGI
    callable.

    Allows us to avoid making django.core.handlers.WSGIHandler public API, in
    case the internal WSGI implementation changes or moves in the future.

    """
    django.setup()
    return IdempotentWSGIHandler()
