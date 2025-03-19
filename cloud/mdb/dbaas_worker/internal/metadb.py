"""
MetaDB connection functions
"""

import logging
from contextlib import contextmanager, closing

import psycopg2
from psycopg2.extras import RealDictCursor


class DatabaseConnectionError(Exception):
    """
    Worker database connection error
    """


def is_master_conn(conn):
    """
    Check if given connection is not read-only
    """
    with conn:
        cur = conn.cursor()
        cur.execute('show transaction_read_only')
        return cur.fetchone()['transaction_read_only'] == 'off'


def get_master_conn(dsn_prefix, host_list, log):
    """
    Get metadb connection with master
    """
    for host in host_list:
        try:
            conn = psycopg2.connect(dsn_prefix, cursor_factory=RealDictCursor, host=host)
        except psycopg2.Error as exc:
            log.warning('Unable to connect to %s: %s', host, repr(exc))
            continue
        try:
            if is_master_conn(conn):
                return conn
            conn.close()
        except psycopg2.Error as exc:
            log.warning('Checking for master of %s failure: %s', host, exc)
            try:
                if not conn.closed:
                    conn.close()
            except psycopg2.Error as close_exc:
                log.warning('Unable to close orphan conn with %s: %s', host, close_exc)

    raise DatabaseConnectionError('Unable to get connection with master')


@contextmanager
def get_cursor(dsn_prefix: str, host_list: list[str], log: logging.Logger):
    connection = get_master_conn(dsn_prefix, host_list, log)
    with closing(connection) as conn_obj:
        with conn_obj:
            yield conn_obj.cursor()
