"""
SQLServer helpers
"""

try:
    import pyodbc

    HAS_PYODBC_LIB = True
except ImportError:
    HAS_PYODBC_LIB = False
from tests.helpers import go_internal_api, workarounds
from tests.helpers.metadb import get_cluster_hosts_with_role


def get_sqlserver_cluster_hosts(context, hosts):
    """
    Filter out witness hosts, leaving real sqlserver hosts
    """
    sqlserver_hosts = set(r[0] for r in get_cluster_hosts_with_role(context, 'sqlserver_cluster'))
    return [h for h in hosts if h['name'] in sqlserver_hosts]


@workarounds.retry(wait_fixed=5, stop_max_attempt_number=6)
def connect(
    context, cluster_config, hosts, master: bool = False, replica: bool = False, dbname=None, geo=None, **kwargs
):
    """
    Connect to a SQLServer host
    """
    if master and replica:
        raise RuntimeError('both "master" and "replica" cannot be true!')

    if not HAS_PYODBC_LIB:
        raise ModuleNotFoundError('No pyodbc lib available')
    clusterHosts = get_sqlserver_cluster_hosts(context, hosts)
    clusterHosts = go_internal_api.get_cluster_hosts(context, clusterHosts, geo=geo)
    if len(clusterHosts) == 0:
        raise RuntimeError('no matching host found. hosts: {}. geo: {}'.format(hosts, geo))

    if not dbname:
        dbname = 'master'

    if 'user' in kwargs and 'password' in kwargs:
        user_creds = {'uid': kwargs['user'], 'pwd': kwargs['password']}
    else:
        user_creds, *_ = [
            {
                'uid': x['name'],
                'pwd': x['password'],
            }
            for x in cluster_config['userSpecs']
        ]

    sql_port = 1433
    for host in clusterHosts:
        sql_host = host.replace('mdb.cloud-preprod', 'db')

        if sql_host in context.tunnels_pool:
            # we will use opened tunnel (local launch)
            sql_port = context.tunnels_pool[sql_host].local_bind_port
            sql_host = '127.0.0.1'

        conn = pyodbc.connect(  # pylint: disable=c-extension-no-member
            driver='{ODBC Driver 17 for SQL Server}',  # /usr/local/lib/libmsodbcsql.17.dylib
            server='{},{}'.format(sql_host, sql_port),
            database='{}'.format(dbname),
            encrypt='no',
            timeout=60,
            **user_creds
        )
        host_is_master = is_master(conn)
        if master and not host_is_master:
            continue
        if replica and host_is_master:
            continue
        return conn
    raise RuntimeError('unable to connect to cluster or cluster hosts are not running')


def is_master(conn):
    """
    Returns true if current host is a primary replica in AG
    """
    cur = conn.cursor()
    cur.execute('''exec msdb.dbo.mdb_is_replica''')
    return cur.fetchone()[0] == 0


def db_query(context, query='SELECT 1', ignore_results=False, **kwargs):
    """
    Execute query on SQLServer server given a config.
    """
    conn = connect(context, **kwargs)
    try:
        cur = conn.cursor()
        cur.execute(query)
        if not ignore_results:
            # perhaps it will be better just to return list(i)... left for backward compatibility
            context.query_result = {'result': [i[0] if len(i) == 1 else list(i) for i in cur.fetchall()]}
        else:
            conn.commit()
            while cur.nextset():
                continue
    finally:
        conn.close()


def is_access_violation_error(err):
    return isinstance(err, pyodbc.ProgrammingError) and err.args[1].find('permission was denied') > 0


def is_login_failed_error(err):
    login_failure = isinstance(err, pyodbc.InterfaceError) and err.args[1].find('Login failed') > 0
    dbopen_failure = isinstance(err, pyodbc.ProgrammingError) and err.args[1].find('The login failed') > 0
    login_timeout = isinstance(err, pyodbc.OperationalError) and err.args[1].find('Login timeout expired') > 0
    # for some reason after user delete odbc return timeout error instead of normal
    return login_failure or dbopen_failure or login_timeout
