"""
Greenplum helpers
"""

import psycopg2
from tests.helpers import workarounds


@workarounds.retry(wait_fixed=30, stop_max_attempt_number=1)
def connect(context, cluster_config, hostnames, dbname='postgres', **kwargs):
    """
    Connect to a Greenplum host
    """
    user = cluster_config['user_name']
    password = cluster_config['user_password']
    local_host = '127.0.0.1'
    for host in hostnames:
        local_port = context.tunnels_pool[host.replace('mdb.cloud-preprod', 'db')].local_bind_port
        conn = psycopg2.connect(  # pylint: disable=c-extension-no-member
            host=local_host, port=local_port, dbname=dbname, user=user, password=password
        )
        if conn:
            conn.set_session(autocommit=True)
            return conn
    raise RuntimeError('unable to connect to cluster or cluster hosts are not running')


def db_query(context, query='SELECT 1', ignore_results=False, **kwargs):
    """
    Execute query on Greenplum server given a config.
    """
    conn = connect(context, **kwargs)
    with conn as txn:
        cur = txn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
        cur.execute(query)
        if not ignore_results:
            context.query_result = {'result': cur.fetchall()}
    conn.close()
