# -*- coding: utf-8 -*-
"""
DBaaS Internal API MetaDB
"""

import logging
from enum import Enum
from json import JSONEncoder
from uuid import UUID

from flask import g
from werkzeug.local import LocalProxy
from psycopg2.extensions import TRANSACTION_STATUS_INTRANS, register_adapter
from psycopg2.extras import Json, register_uuid

from . import PoolGovernor, QueryCache

# Change default json encoder to our custom which can
# serialize uuid.UUID type as simple string
OLD_JSON_ENCODER = JSONEncoder.default


def json_encoder(self, obj):
    """
    Json encoder which encodes
    uuid.UUID as string
    enum.Enum as string
    and other types like default JSONEncoder
    """
    if isinstance(obj, UUID):
        return str(obj)
    if isinstance(obj, Enum):
        return str(obj.value)
    if isinstance(obj, LocalProxy):
        return str(obj)
    return OLD_JSON_ENCODER(self, obj)


# [mypy] error: Cannot assign to a method
JSONEncoder.default = json_encoder  # type: ignore


class MetaDB:
    """
    MetaDB Interface
    """

    def __init__(self):
        self.governor = None
        self.query_conf = None
        self.logger = None
        register_adapter(dict, Json)
        register_uuid()

    def init_metadb(self, config, logger_name, creds=None):
        """
        Configure metadb
        """
        pool_config = config.copy()
        if creds is not None:
            pool_config["user"] = creds.get("user")
            pool_config["password"] = creds.get("password")

        self.governor = PoolGovernor(pool_config, logger_name)
        self.governor.start()
        self.query_conf = QueryCache(logger_name)
        self.logger = logging.getLogger(logger_name)

    def init_context(self, master=False):
        """
        Attach cursor to request
        """
        if not self.governor:
            raise RuntimeError('Initialize MetaDB with init_metadb')
        pool = self.governor.getpool(master)
        conn = pool.getconn()
        cur = conn.cursor()
        if not master:
            cur.execute('BEGIN TRANSACTION READ ONLY')
        context = {'master': master, 'pool': pool, 'conn': conn, 'cur': cur}
        g.metadb_context = context

    def close_context(self):
        """
        Detach cursor from request and run rollback if conn is dirty
        """
        g.metadb_context['pool'].putconn(g.metadb_context['conn'])
        g.metadb_context = None

    def commit(self):
        """
        Commit current transaction
        """
        if getattr(g, 'metadb_context', None):
            if g.metadb_context['conn'].get_transaction_status() != TRANSACTION_STATUS_INTRANS:
                raise RuntimeError('Commit outside of transaction')
            g.metadb_context['conn'].commit()

    def rollback(self):
        """
        Rollback current transaction
        """
        if getattr(g, 'metadb_context', None):
            g.metadb_context['conn'].rollback()

    def query(self, query, fetch=True, master=False, **query_args):
        """
        Run query and (optionally) fetch result
        """
        if getattr(g, 'metadb_context', None):
            if master and not g.metadb_context['master']:
                self.close_context()
                self.init_context(master)
        else:
            self.init_context(master)

        return self.query_conf.query(g.metadb_context['cur'], query, query_args=query_args, fetch=fetch)
