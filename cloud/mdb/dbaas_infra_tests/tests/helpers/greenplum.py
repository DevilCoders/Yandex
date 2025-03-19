"""
Utilities for dealing with Greenplum
"""

import psycopg2

from tests.helpers import workarounds
from tests.helpers.internal_api import get_cluster_ports


# pylint: disable=too-many-locals
@workarounds.retry(wait_fixed=1000, stop_max_attempt_number=10)
def greenplum_connect(context, cluster_config, hosts, geo=None, **kwargs):
    """
    Connect to greenplum cluster given a config.
    Returns psycopg2.connection instance
    """
    hostports = get_cluster_ports(context, hosts, port=5432, geo=geo)
    dbname = 'postgres'
    if cluster_config is None:
        cluster_config = context.cluster_config
    user_creds = {
        'user': kwargs.get('user', cluster_config['user_name']),
        'password': kwargs.get('password', cluster_config['user_password']),
    }

    for host, port in hostports:
        connect_args = {
            'host': host,
            'port': port,
            'dbname': dbname,
            **user_creds,
            **kwargs,
        }
        try:
            conn = psycopg2.connect(**connect_args)
        except psycopg2.OperationalError:
            continue
        conn.set_session(autocommit=True)
        return conn
    raise RuntimeError('unable to connect to cluster or cluster hosts are not running')


def greenplum_query(context, query='SELECT 1', ignore_results=False, **kwargs):
    """
    Execute query on greenplum server given a config.
    """
    conn = greenplum_connect(context, **kwargs)
    with conn as txn:
        cur = txn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
        cur.execute(query)
        # after DELETE or CREATE we can't fetch result
        # http://initd.org/psycopg/docs/cursor.html#cursor.fetchall
        if not ignore_results:
            context.query_result = {'result': cur.fetchall()}
    conn.close()


def check_greenplum(context, **kwargs):
    """
    Execute check query.
    """
    greenplum_query(context, query='SELECT 1', cluster_config=context.cluster_config, hosts=context.hosts, **kwargs)
