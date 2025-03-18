# coding: utf-8

try:
    from django.db.backends.utils import logger
except ImportError:
    from django.db.backends.util import logger

try:
    from django.db.backends.postgresql.base import (
        DatabaseWrapper,
        BaseDatabaseWrapper,
    )
except ImportError:
    from django.db.backends.postgresql_psycopg2.base import (
        DatabaseWrapper,
        BaseDatabaseWrapper,
    )

try:
    from django.db.backends.postgresql.operations import DatabaseOperations
except ImportError:
    from django.db.backends.postgresql_psycopg2.operations import (
        DatabaseOperations
    )
