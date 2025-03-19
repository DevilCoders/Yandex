# -*- coding: utf-8 -*-
"""
Database interaction module
"""

import logging
from contextlib import suppress
from threading import local

import psycopg2
from psycopg2.extensions import TRANSACTION_STATUS_IDLE, TRANSACTION_STATUS_INTRANS, register_adapter
from psycopg2.extras import Json, RealDictCursor
from dbaas_common import tracing
import cloud.mdb.internal.python.query_conf
from .config import app_config
from .metrics import QueryObserver


class DBError(Exception):
    """
    Common database exception
    """


QUERIES = cloud.mdb.internal.python.query_conf.load_queries(__name__, 'query_conf')
CONN_CONTEXT = local()
register_adapter(dict, Json)


@tracing.trace('DBMDB Connect')
def get_cursor():
    """
    Get cursor from context or create new
    """
    conn = getattr(CONN_CONTEXT, 'conn', None)
    if not conn:
        config = app_config()
        conn = psycopg2.connect(
            '{base} host={hosts} target_session_attrs=read-write'.format(
                base=config['DB_CONN'], hosts=config['DB_HOSTS']
            ),
            cursor_factory=RealDictCursor,
        )
        CONN_CONTEXT.conn = conn
    cur = conn.cursor()
    if conn.get_transaction_status() == TRANSACTION_STATUS_IDLE:
        cur.execute('BEGIN')
    return cur


def rollback():
    """
    Do rollback in current transaction
    """
    conn = getattr(CONN_CONTEXT, 'conn', None)
    if conn:
        with suppress(Exception):
            conn.rollback()


def clean_context():
    """
    Close dirty connections
    """
    conn = getattr(CONN_CONTEXT, 'conn', None)
    if conn and conn.get_transaction_status() != TRANSACTION_STATUS_IDLE:
        CONN_CONTEXT.conn = None
        with suppress(Exception):
            conn.close()


@tracing.trace('DBMDB Commit')
def commit():
    """
    Commit pending transaction
    """
    conn = getattr(CONN_CONTEXT, 'conn', None)
    if not conn:
        return
    status = conn.get_transaction_status()
    if status == TRANSACTION_STATUS_IDLE:
        return
    if status != TRANSACTION_STATUS_INTRANS:
        raise DBError('Unexpected transaction status: {status}'.format(status=status))
    conn.commit()


@tracing.trace('DBMDB Execute')
def execute(name, fetch=True, **kwargs):
    """
    Execute query (optionally fetching result)
    """
    tracing.set_tag('db.type', 'sql')
    tracing.set_tag('db.statement.name', name)

    log = logging.getLogger('flask.app')
    cur = get_cursor()
    mogrified = cur.mogrify(QUERIES[name], kwargs)
    tracing.set_tag('db.statement', mogrified)
    log.debug(' '.join(mogrified.decode('utf-8').split()))

    with QueryObserver(name):
        cur.execute(mogrified)
        return cur.fetchall() if fetch else None
