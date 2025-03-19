"""
Utilities for dealing with Internal API
"""
from collections import defaultdict

import psycopg2
import pymysql
import redis
import requests
from pkg_resources import parse_version
from rediscluster import RedisCluster, exceptions
from tests.helpers import docker, workarounds


class InternalAPIError(RuntimeError):
    """
    General API error exception
    """


def get_base_url(context):
    """
    Get base URL for sending requests to Internal API.
    """
    host, port = docker.get_exposed_port(
        docker.get_container(context, 'internal-api01'), context.conf['projects']['internal-api']['expose']['https'])

    return 'https://{0}:{1}'.format(host, port)


def get_iam_token(context):
    """
    Get iam token
    """
    host, port = docker.get_exposed_port(
        docker.get_container(context, 'fake_iam01'), context.conf['projects']['fake_iam']['expose']['identity'])

    url = 'http://{host}:{port}/iam/public/v1/tokens'.format(host=host, port=port)

    res = requests.post(
        url,
        headers={
            'Accept': 'application/json',
            'Content-Type': 'application/json',
        },
        json={'oauthToken': 'dummy'})

    return res.json()['iamToken']


# pylint: disable=not-callable
@workarounds.retry(wait_fixed=1000, stop_max_attempt_number=10)
def request(context, handle, method='GET', deserialize=False, **kwargs):
    """
    Perform request to Internal API and check response on internal server
    errors (error 500).

    If deserialize is True, the response will also be checked on all
    client-side (4xx) and server-side (5xx) errors, and then deserialized
    by invoking Response.json() method.
    """
    req_kwargs = {
        'headers': {
            'Accept': 'application/json',
            'Content-Type': 'application/json',
            'X-YaCloud-SubjectToken': get_iam_token(context),
        },
    }
    req_kwargs.update(kwargs)
    res = requests.request(
        method,
        '{base}/{handle}'.format(
            base=get_base_url(context),
            handle=handle,
        ),
        verify=False,
        **req_kwargs,
    )

    try:
        res.raise_for_status()
    except requests.HTTPError as exc:
        if deserialize or res.status_code == 500:
            msg = '{0} {1} failed with {2}: {3}'.format(method, handle, res.status_code, res.text)
            raise InternalAPIError(msg) from exc

    return res.json() if deserialize else res


def decrypt(context, data, version=1):
    """
    Decrypt a dict and replace it with string
    """
    is_cyphertext = isinstance(data, dict) \
        and int(data.get('encryption_version', -1)) == version \
        and data.get('data')
    if not is_cyphertext:
        return data
    box = context.conf['dynamic']['internal_api']['box']
    return box.decrypt_utf(data['data'])


def get_clusters(context, folder_id, cluster_type=None, deserialize=True):
    """
    Get clusters.
    """
    if cluster_type is None:
        cluster_type = context.cluster_type
    handle = 'mdb/{0}/v1/clusters'.format(cluster_type)
    params = {'folderId': folder_id}

    return request(context, handle=handle, params=params, deserialize=deserialize)


def get_cluster(context, cluster_name, folder_id):
    """
    Get cluster by name.
    """
    cluster_id = _get_cluster_id(context, cluster_name, folder_id)
    handle = 'mdb/{0}/v1/clusters/{1}'.format(context.cluster_type, cluster_id)
    response = request(context, handle=handle).json()
    if 'id' in response:
        return response

    raise InternalAPIError('Failed to get cluster: {0}'.format(response))


def _get_cluster_id(context, cluster_name, folder_id):
    """
    Get cluster id by name.
    """
    for cluster in get_clusters(context, folder_id)['clusters']:
        if cluster['name'] == cluster_name:
            return cluster['id']

    raise InternalAPIError('cluster {0} not found'.format(cluster_name))


def get_hosts(context, cluster_id, deserialize=True):
    """
    Get cluster hosts.
    """
    handle = '/mdb/{0}/v1/clusters/{1}/hosts'.format(context.cluster_type, cluster_id)
    return request(context, handle, deserialize=deserialize)


def get_host_config(context, hostname):
    """
    Get host config (pillar) by name.
    """
    return request(
        context,
        'api/v1.0/config/{fqdn}'.format(fqdn=hostname),
        headers={
            'Access-Id': context.conf['dynamic']['salt']['access_id'],
            'Access-Secret': context.conf['dynamic']['salt']['access_secret'],
        }).json()


def load_cluster_into_context(context, cluster_name=None, with_hosts=True):
    """
    Reload cluster with related objects and update corresponding context data.
    """
    if not cluster_name:
        cluster_name = context.cluster['name']

    context.cluster = get_cluster(context, cluster_name, context.folder['folder_ext_id'])

    if with_hosts:
        context.hosts = get_hosts(context, context.cluster['id'])['hosts']


def load_cluster_type_info_into_context(context):
    """
    Reload cluster type information and update corresponding context data.
    """
    params = {'folderId': context.folder['folder_ext_id']}
    response = request(context, '/mdb/1.0/console/clusters:types', params=params, deserialize=True)

    for obj in response["clusterTypes"]:
        if obj['type'] != context.cluster_type:
            continue

        context.versions = sorted(obj.get('versions', []), key=parse_version)


def get_host_port(context, host, port):
    """
    Returns container's exposed host and port
    """
    container_name, *_ = host['name'].split('.')
    return docker.get_exposed_port(
        docker.get_container(context, container_name),
        port,
    )


def get_cluster_ports(context, hosts, port, host_type=None, geo=None):
    """
    Get real ports of cluster given cluster name and properties
    """
    ports = []
    for host in hosts:
        if host_type:
            if host.get('type') != host_type:
                continue
        if geo:
            if host.get('zoneId') != geo:
                continue

        ports.append((get_host_port(context, host, port)))
    return ports


# pylint: disable=too-many-locals
@workarounds.retry(wait_fixed=1000, stop_max_attempt_number=10)
def postgres_connect(context, cluster_config, hosts, master=False, dbname=None, geo=None, **kwargs):
    """
    Connect to postgresql cluster given a config.
    Returns psycopg2.connection instance
    """
    hostports = get_cluster_ports(context, hosts, port=6432, geo=geo)
    if dbname is None:
        # Get first db if none is specified
        dbname, *_ = [x['name'] for x in cluster_config['databaseSpecs']]
    if 'user' in kwargs and 'password' in kwargs:
        user_creds = dict()
    else:
        user_creds, *_ = [{
            'user': x['name'],
            'password': x['password'],
        } for x in cluster_config['userSpecs']]

    for host, port in hostports:
        connect_args = {
            'host': host,
            'port': port,
            'dbname': dbname,
            **user_creds,
            **kwargs,
        }
        conn = psycopg2.connect(**connect_args)
        if not master:
            return conn
        conn.set_session(autocommit=True)
        with conn.cursor() as cur:
            cur.execute('SELECT pg_is_in_recovery()')
            is_replica, *_ = cur.fetchone()
            if not is_replica:
                return conn
        conn.close()
    raise RuntimeError('unable to connect to cluster or cluster hosts are not running')


def clickhouse_query(context,
                     query,
                     cluster_config=None,
                     shard_name=None,
                     zone_name=None,
                     client_options=None,
                     all_hosts=True,
                     check_response=True,
                     assume_positive=True,
                     user=None,
                     password=None):
    """
    Execute query on hosts of ClickHouse cluster.
    """
    # pylint: disable=too-many-arguments
    hosts = [host for host in context.hosts if host['type'] == 'CLICKHOUSE']
    if shard_name:
        hosts = [
            host for host in hosts
            if host.get('shardName', '') == shard_name or host.get('shard_name', '') == shard_name
        ]
    if zone_name:
        hosts = [host for host in hosts if host.get('zoneId', '') == zone_name or host.get('zone_id', '') == zone_name]

    if cluster_config is None:
        cluster_config = context.cluster_config

    if client_options is None:
        client_options = context.client_options

    hostports = get_cluster_ports(context, hosts, port=8443)
    if all_hosts is False:
        hostports = [hostports[0]]

    if user is None:
        user_spec = cluster_config['userSpecs'][0]
        user = user_spec['name']
        password = user_spec['password']

    if isinstance(password, str):
        password = password.encode('utf-8')

    result = []
    for host_id, (host, port) in zip(hosts, hostports):
        resp = requests.post(
            f'https://{host}:{port}/',
            params={
                'query': query,
                **client_options,
            },
            verify=False,
            auth=(user, password))

        if check_response:
            if assume_positive:
                assert resp.status_code == 200, 'Error: {code} {text}'.format(code=resp.status_code, text=resp.text)
            else:
                assert resp.status_code != 200, 'Error: {code} {text}'.format(code=resp.status_code, text=resp.text)

        result.append(dict(host=host_id, value=resp.text.strip()))

    return result


def get_redis_query_result(conn, query):
    """
    Get redis query result in list[str]
    :param conn:
    :param query:
    :return:
    """
    result = conn.execute_command(query)
    if isinstance(result, bytes):
        result = result.decode().splitlines()
    elif isinstance(result, list):
        result = [s.decode() if isinstance(s, bytes) else s for s in result]
    return result


# pylint: disable=too-few-public-methods
class RedisConnProvider:
    """
    Factory class for redis connection for both sharded and sentinel clusters
    """

    def __init__(self, sharded=False, password=None, tls_enabled=None, ssl_ca_certs=None):
        if sharded:
            self.cls = RedisCluster
            self.kwargs = dict(password=password, skip_full_coverage_check=True)
        else:
            self.cls = redis.Redis
            self.kwargs = dict(password=password, ssl=tls_enabled, ssl_ca_certs=ssl_ca_certs)

    def __call__(self, host, port):
        self.kwargs.update(dict(host=host, port=port))
        return self.cls(**self.kwargs)


def redis_query_all(context, query='ping', sharded=False, expected=None, **creds):
    """
    Execute query on all Redis hosts given a config.
    """
    if 'password' in creds:
        context.conf['redis_creds'] = creds
    password = context.conf['redis_creds']['password']

    hosts = context.hosts
    host_ports = get_cluster_ports(context, hosts, port=6379)

    conn_getter = RedisConnProvider(sharded=sharded, password=password)
    result = {}
    for (host, port), host_data in zip(host_ports, hosts):
        conn = conn_getter(host, port)
        key = (host_data['name'], host_data['shardName'])
        try:
            result[key] = get_redis_query_result(conn, query)
        except exceptions.RedisClusterException:
            if expected is None:
                pass

    return result


def redis_query(context, query='ping', master=False, geo=None, shard_name=None, tls_enabled=False, **creds):
    """
    Execute query on Redis server given a config.
    """
    hosts = context.hosts
    if shard_name:
        hosts = [host for host in hosts if host['shardName'] == shard_name]

    src_port = 6380 if tls_enabled else 6379
    ssl_ca_certs = 'staging/images/fake_certificator/config/pki/CA.pem' if tls_enabled else None
    hostports = get_cluster_ports(context, hosts, port=src_port, geo=geo)
    if 'password' in creds:
        context.conf['redis_creds'] = creds
    password = context.conf['redis_creds']['password']

    found_master = False
    for host, port in hostports:
        conn = redis.StrictRedis(
            host=host, port=port, db=0, password=password, ssl=tls_enabled, ssl_ca_certs=ssl_ca_certs)
        if master:
            role = conn.info()['role']
            if role != 'master':
                continue
        result = get_redis_query_result(conn, query)
        context.query_result = {'result': result}
        found_master = True
    if master and not found_master:
        raise InternalAPIError('Failed to find master.')


def build_host_type_dict(context):
    """
    Get map of host_type => host
    """
    hosts = defaultdict(list)
    for host in context.hosts:
        host_type = host.get('type', context.cluster_type.upper())
        shard_name = host.get('shardName')
        k = shard_name if shard_name else host_type
        hosts[k].append(host)
    return hosts


def group_hosts_by_type(context):
    """
    Get classified list of context.hosts
    """
    host_map = build_host_type_dict(context)
    return list(host_map.values())


def patch_create_ssl_ctx(func):
    """
    Monkey patch for pymysql library, allows to localhost IP using SSL
    """

    def wrapper(self, *args, **kwargs):
        ctx = func(self, *args, **kwargs)
        ctx.check_hostname = False
        return ctx

    return wrapper


# pylint: disable=protected-access
pymysql.connections.Connection._create_ssl_ctx = patch_create_ssl_ctx(pymysql.connections.Connection._create_ssl_ctx)


def mysql_query(context, query='SELECT 1', master=False, geo=None, **kwargs):
    """
    Execute query on MySQL server given a config.
    """
    hostports = get_cluster_ports(context, context.hosts, geo=geo, port=3306)

    creds = {}
    if 'user' in kwargs:
        creds['user'] = kwargs['user']
    else:
        creds['user'] = context.cluster_config['userSpecs'][0]['name']
    if 'password' in kwargs:
        creds['password'] = kwargs['password']
    elif creds['user'] == 'admin':
        host_config = get_host_config(context, context.hosts[0]['name'])
        creds['password'] = decrypt(
            context,
            host_config['data']['mysql']['users']['admin']['password'],
        )
    else:
        all_user_specs = context.cluster_config['userSpecs']
        cred_user_specs = [userSpec for userSpec in all_user_specs if userSpec['name'] == creds['user']]
        creds['password'] = cred_user_specs[0]['password']
    if 'dbname' in kwargs:
        creds['db'] = kwargs['dbname']
    else:
        creds['db'] = [
            s['permissions'][0]['databaseName'] for s in context.cluster_config['userSpecs']
            if s['name'] == creds['user']
        ][0]

    for host, port in hostports:
        if host == "localhost":
            host = "127.0.0.1"
        if isinstance(creds['password'], str):
            creds['password'] = creds['password'].encode('utf-8')
        conn = pymysql.connect(
            host=host,
            port=int(port),
            autocommit=True,
            cursorclass=pymysql.cursors.DictCursor,
            ssl={'ca': 'staging/images/fake_certificator/config/pki/CA.pem'},
            **creds)
        with conn.cursor() as cur:
            if master:
                cur.execute("SHOW SLAVE STATUS")
                result = cur.fetchall()
                if result:
                    continue
            cur.execute(query)
            context.query_result = {'result': list(cur.fetchall())}
        conn.close()


def postgres_query(context, query='SELECT 1', ignore_results=False, **kwargs):
    """
    Execute query on PostgreSQL server given a config.
    """
    conn = postgres_connect(context, **kwargs)
    with conn as txn:
        cur = txn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
        cur.execute(query)
        # after DELETE or CREATE we can't fetch result
        # http://initd.org/psycopg/docs/cursor.html#cursor.fetchall
        if not ignore_results:
            context.query_result = {'result': cur.fetchall()}
    conn.close()


def convert_version_to_spec(version):
    """
    Convert postgresql version to config spec format (postgresqlConfig_9_6)
    Examples:
        '9.6' -> '9_6'
        '10'  -> '10'
    """
    return version.replace('.', '_')
