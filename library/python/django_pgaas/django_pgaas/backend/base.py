# coding: utf-8
from __future__ import unicode_literals

import six
import sys

from django.core.exceptions import ImproperlyConfigured

from django_pgaas.compat import DatabaseWrapper as PostgreSQLDatabaseWrapper
from django_pgaas.conf import settings
from django_pgaas.utils import get_retry_range_for
from .operations import DatabaseOperations

try:
    from zero_downtime_migrations.backend.schema import DatabaseSchemaEditor as ZDMDatabaseSchemaEditor

except ImportError:
    if settings.PGAAS_USE_ZDM:
        raise ImproperlyConfigured('USE_ZDM=True requires zero-downtime-migrations installed')


class DatabaseWrapper(PostgreSQLDatabaseWrapper):
    if settings.PGAAS_USE_ZDM:
        SchemaEditorClass = ZDMDatabaseSchemaEditor

    def __init__(self, *args, **kwargs):
        super(DatabaseWrapper, self).__init__(*args, **kwargs)

        self.ops = DatabaseOperations(self)

        self.operators.update({
            'iexact': '= UPPER(%s {})'.format(settings.PGAAS_COLLATION),
            'icontains': 'LIKE UPPER(%s {})'.format(settings.PGAAS_COLLATION),
            'istartswith': 'LIKE UPPER(%s {})'.format(settings.PGAAS_COLLATION),
            'iendswith': 'LIKE UPPER(%s {})'.format(settings.PGAAS_COLLATION),
        })

    def get_new_connection(self, *args, **kwargs):

        retry_range = get_retry_range_for('PGAAS connection')

        for _ in retry_range:
            try:
                return super(DatabaseWrapper, self).get_new_connection(*args, **kwargs)

            except Exception as exc:
                last_exc_info = sys.exc_info()

        six.reraise(*last_exc_info)
