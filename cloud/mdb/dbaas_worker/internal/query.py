# -*- coding: utf-8 -*-
"""
Query execution helper
"""

import logging
import os

import library.python.resource as resource


def _load_queries():
    ret = {}
    prefix = 'sqls/'
    for key, value in resource.iteritems(prefix, strip_prefix=True):
        ret[os.path.splitext(key)[0]] = str(value, encoding='utf-8')

    return ret


QUERIES = _load_queries()


def execute(cur, name, fetch=True, **kwargs):
    """
    Execute query (optionally fetching result)
    """
    log = logging.getLogger('query')
    if name not in QUERIES:
        raise RuntimeError('{name} not found in query cache'.format(name=name))
    mogrified = cur.mogrify(QUERIES[name], kwargs)
    log.debug(
        ' '.join(mogrified.decode('utf-8').split()),
        extra={
            'metadb_host': cur.connection.get_dsn_parameters().get(
                'host',
                'localhost',
            ),
        },
    )

    cur.execute(mogrified)

    return cur.fetchall() if fetch else None
