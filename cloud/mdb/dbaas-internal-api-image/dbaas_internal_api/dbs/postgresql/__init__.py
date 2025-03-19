# -*- coding: utf-8 -*-
"""
DBaaS Internal API PostgreSQL DB
"""

from functools import wraps

from flask import g

from .pool import PoolGovernor  # noqa
from .query import QueryCache  # noqa
from .db import MetaDB

DB = MetaDB()


def metadb_middleware(callback):
    """
    DB Middleware
    """

    @wraps(callback)
    def db_wrapper(*args, **kwargs):
        """
        DB wrapper function (Attaches db to request context and closes conn)
        """
        g.metadb = DB
        try:
            res = callback(*args, **kwargs)
            return res
        finally:
            if getattr(g, 'metadb_context', None):
                try:
                    DB.close_context()
                except Exception as exc:
                    DB.logger.error('Unable to close context: %s', repr(exc))

    return db_wrapper
