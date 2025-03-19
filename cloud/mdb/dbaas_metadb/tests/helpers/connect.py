"""
Connect helpers
"""
import logging
from contextlib import contextmanager

import psycopg2
import psycopg2.extras


def connect(dsn, autocommit=False, application_name=None):
    """
    Create new connection
    """
    connect_extra = {}
    if application_name is not None:
        connect_extra['application_name'] = application_name
    conn = psycopg2.connect(dsn, connection_factory=psycopg2.extras.LoggingConnection, **connect_extra)
    connection_logger = logging.getLogger('queries')
    # psycopg log to debug, but we need INFO,
    # cause by default behave doesn't catch DEBUG log
    connection_logger.debug = connection_logger.info
    conn.initialize(connection_logger)
    if autocommit:
        conn.autocommit = True
    return conn


@contextmanager
def transaction(dsn, autocommit=False, **connect_extra):
    """
    Connect -> Commit|Rollback -> Close
    """
    conn = connect(dsn, autocommit=autocommit, **connect_extra)
    try:
        with conn:
            yield conn
    finally:
        if not conn.closed:
            conn.close()
