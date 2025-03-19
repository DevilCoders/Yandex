"""Database helper functions."""
import logging
from contextlib import contextmanager

import psycopg2

from .exceptions import DatabaseError

# pylint: disable=invalid-name
logger = logging.getLogger(__name__)


def is_master(conn):
    """Check if the connection is to the master server."""
    with conn.cursor() as cur:
        cur.execute('SELECT pg_is_in_recovery()')
        (is_slave,) = cur.fetchone()
        return not is_slave


def get_connection(connstrings, need_master=False):
    """Try to obtain connection to the database."""
    for dsn in connstrings:
        try:
            conn = psycopg2.connect(dsn)
        except psycopg2.Error:
            continue
        if need_master and not is_master(conn):
            conn.close()
            continue
        logger.debug('Connected to the database: %s', dsn)
        return conn
    raise DatabaseError('Failed to connect to the database')


@contextmanager
def get_cursor(connstrings, need_master=False, err_msg='Database error'):
    """Wraps connection/cursor obtaining logic."""
    conn = get_connection(connstrings, need_master)
    try:
        with conn:
            with conn.cursor() as cur:
                yield cur
    except psycopg2.Error as err:
        logger.exception(err_msg)
        raise DatabaseError(err)
    finally:
        try:
            conn.close()
        except psycopg2.Error:
            pass
