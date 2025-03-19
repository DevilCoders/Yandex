"""
DeployDB data generation helper
"""

import os
import re
from contextlib import contextmanager

import psycopg2
from psycopg2.extras import DictCursor

from .docker import get_container, get_exposed_port


def _connect(context):
    deploydb_opts = context.conf['projects']['deploy_db']['db']

    host, port = get_exposed_port(
        get_container(context, 'deploy_db01'),
        context.conf['projects']['deploy_db']['deploy_db01']['expose']['pgbouncer'])

    dsn = ('host={host} port={port} dbname={dbname} '
           'user={user} password={password} sslmode=require').format(
               host=host,
               port=port,
               dbname=deploydb_opts['dbname'],
               user=deploydb_opts['user'],
               password=deploydb_opts['password'])

    return psycopg2.connect(dsn, cursor_factory=DictCursor)


@contextmanager
def _query_master(context, query, *args):
    """
    Run query on master yields cursor
    """
    with _connect(context) as conn:
        cur = conn.cursor()
        cur.execute(query, args)
        yield cur


def get_version(context):
    """
    Get actual version of deploydb
    """
    with _query_master(context, 'SELECT max(version) AS v from public.schema_version') as cur:
        return cur.fetchone()['v']


def get_latest_version(context):
    """
    Get latest migration version
    """
    migration_file_re = re.compile(r'V(?P<version>\d+)__(?P<description>.+)\.sql$')
    migrations_dir = os.path.join(
        context.conf['staging_dir'],
        'code',
        'go-mdb',
        'deploydb',
        'migrations',
    )
    files = os.listdir(migrations_dir)
    max_version = 0
    for migration_file in files:
        match = migration_file_re.match(migration_file)
        if match is None:
            pass
        version = int(match.group('version'))
        if version > max_version:
            max_version = version

    if max_version > 0:
        return max_version
    raise RuntimeError('Malformed deploydb migrations dir')
