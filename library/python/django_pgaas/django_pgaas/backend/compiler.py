# coding: utf-8
from __future__ import unicode_literals

import six
import sys

# noinspection PyUnresolvedReferences
from django.db.models.sql.compiler import (
    SQLCompiler as BaseSQLCompiler,
    SQLAggregateCompiler as BaseSQLAggregateCompiler,
    SQLDeleteCompiler as BaseSQLDeleteCompiler,
    SQLInsertCompiler as BaseSQLInsertCompiler,
    SQLUpdateCompiler as BaseSQLUpdateCompiler,
)  # noqa
from django.db.utils import DatabaseError, InterfaceError

from django_pgaas.utils import can_retry_error, get_retry_range_for


def retry(execute_sql):
    def wrapper(self, *args, **kwargs):

        retry_range = get_retry_range_for('Query')

        for _ in retry_range:
            try:
                return execute_sql(self, *args, **kwargs)

            except (DatabaseError, InterfaceError) as exc:
                if self.connection.in_atomic_block or not can_retry_error(exc):
                    raise

                last_exc_info = sys.exc_info()

                self.connection.close()  # to avoid 'InterfaceError: connection already closed'
                self.connection.ensure_connection()

        setattr(last_exc_info[1], '_django_pgaas_retried', True)
        six.reraise(*last_exc_info)

    return wrapper


class SQLCompiler(BaseSQLCompiler):
    @retry
    def execute_sql(self, *args, **kwargs):
        return super(SQLCompiler, self).execute_sql(*args, **kwargs)


class SQLAggregateCompiler(BaseSQLAggregateCompiler):
    @retry
    def execute_sql(self, *args, **kwargs):
        return super(SQLAggregateCompiler, self).execute_sql(*args, **kwargs)


class SQLDeleteCompiler(BaseSQLDeleteCompiler):
    @retry
    def execute_sql(self, *args, **kwargs):
        return super(SQLDeleteCompiler, self).execute_sql(*args, **kwargs)


class SQLInsertCompiler(BaseSQLInsertCompiler):
    @retry
    def execute_sql(self, *args, **kwargs):
        return super(SQLInsertCompiler, self).execute_sql(*args, **kwargs)


class SQLUpdateCompiler(BaseSQLUpdateCompiler):
    @retry
    def execute_sql(self, *args, **kwargs):
        return super(SQLUpdateCompiler, self).execute_sql(*args, **kwargs)

