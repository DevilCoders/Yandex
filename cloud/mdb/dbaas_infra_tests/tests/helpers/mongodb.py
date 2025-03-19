"""
Utilities for dealing with MongoDB
"""
import re
import time
from collections import Counter, namedtuple
from pprint import pformat
from urllib.parse import quote_plus

import pymongo
from hamcrest import assert_that, empty, equal_to, is_not

from tests.helpers.internal_api import (build_host_type_dict, decrypt, get_host_config, get_host_port)

MONGOD_HOST_TYPE = 'MONGOD'
MONGOS_HOST_TYPE = 'MONGOS'
MONGOCFG_HOST_TYPE = 'MONGOCFG'
MONGOINFRA_HOST_TYPE = 'MONGOINFRA'

MONGO_ADMIN_USER = 'admin'
MONGOD_DEFAULT_SHARD = 'rs01'

SERVICE_TYPE_PORT = {
    MONGOS_HOST_TYPE: 27017,
    MONGOD_HOST_TYPE: 27018,
    MONGOCFG_HOST_TYPE: 27019,
    MONGOINFRA_HOST_TYPE: 27017,
    # We can check only one service, so for mongoinfra we'll check MongoS only
}


class OplogTimestamp(namedtuple('OplogTimestamp', ['ts', 'inc'])):
    """
    Oplog timestamp
    """

    @classmethod
    def from_string(cls, str_ts):
        """
        Build OplogTimestamp from string
        """
        strs = str_ts.split(".")
        assert len(strs) == 2, "Invalid oplog ts string: {}".format(str_ts)
        return OplogTimestamp(ts=int(strs[0]), inc=int(strs[1]))


class OplogArchive(namedtuple('OplogArchive', ['start_ts', 'end_ts'])):
    """
    Oplog archive
    """

    @classmethod
    def from_string(cls, str_archive):
        """
        Build OplogArchive from string
        """
        matches = re.search(r'(?P<type>oplog|gap)_(?P<start_ts>[0-9]+\.[0-9]+)_(?P<end_ts>[0-9]+\.[0-9]+)\.',
                            str_archive)
        if matches is None:
            raise RuntimeError('Bad archive name: {0}'.format(str_archive))
        if matches.group('type') == 'gap':
            raise RuntimeError('Unexpected gap archive: {0}'.format(str_archive))

        return OplogArchive(
            start_ts=OplogTimestamp.from_string(matches.group('start_ts')),
            end_ts=OplogTimestamp.from_string(matches.group('end_ts')))

    def includes_ts(self, ts):
        """
        Check if archive includes given ts
        """
        return self.start_ts < ts <= self.end_ts


def mongo_get_conns(context, username, host_type=None, shard_name=None):
    """
    Get connextions with mongo hosts
    """
    hosts = mongo_get_hosts(context, host_type, shard_name)
    creds = mongodb_get_creds(context, username)
    connections = mongo_connect(context, hosts, creds)
    assert_that(connections, is_not(empty()))
    return connections


def mongo_get_primary_conn(connections):
    """
    Get primary conn from rs connections
    """
    for conn in connections:
        if conn['mc'].is_primary:
            return conn
    raise RuntimeError('Primary connection was not found: {conns}'.format(conns=connections))


# pylint: disable=too-many-locals
def mongo_connect(context, hosts, user_creds, port=None):
    """
    Connect to mongodb cluster given a config.
    Returns list of pymongo.MongoClient
    """
    assert hosts, 'Unexpected empty hosts list, all hosts: {0}'.format(context.hosts)

    user_name = user_creds['name']
    user_password = user_creds['password']
    user_dbname = user_creds['dbname']

    connections = []
    for host in hosts:
        host_type = host['type']
        srv_port = SERVICE_TYPE_PORT[host_type] if not port else port
        docker_host, docker_port = get_host_port(context, host, srv_port)
        connect_uri = 'mongodb://{user}:{password}@{host}:{port}/{dbname}'.\
            format(
                user=quote_plus(user_name),
                password=quote_plus(user_password),
                host=docker_host,
                port=docker_port,
                dbname=user_dbname)
        mongo_client = pymongo.MongoClient(connect_uri)
        role = 'primary' if mongo_client.is_primary else 'secondary'
        connections.append({
            'mc': mongo_client,
            'role': role,
            'is_primary': mongo_client.is_primary,
            'dbname': user_dbname,
            'host': docker_host,
            'port': docker_port,
            'host_type': host_type,
            'fqdn_port': '{0}:{1}'.format(host['name'], srv_port),
        })

    return connections


def mongodb_get_admin_creds(context):
    """
    Get mongodb admin credentials from loaded cluster
    """
    host_config = get_host_config(context, context.hosts[0]['name'])
    admin_password = decrypt(
        context,
        host_config['data']['mongodb']['users']['admin']['password'],
    )
    return {
        'name': 'admin',
        'password': admin_password,
        'dbname': 'admin',
    }


def mongodb_get_creds(context, username):
    """
    Get mongodb user credentials from loaded cluster
    """
    host_config = get_host_config(context, context.hosts[0]['name'])
    user_data = host_config['data']['mongodb']['users'][username]
    admin_password = decrypt(
        context,
        user_data['password'],
    )
    dbname = next(iter(user_data['dbs'])) if username != 'admin' else 'admin'
    return {
        'name': username,
        'password': admin_password,
        'dbname': dbname,
    }


def mongo_get_hosts(context, service_type=None, shard_name=None):
    """
    Returns list hosts of service type & shard
    """
    if shard_name is not None:
        return [h for h in context.hosts if h['shardName'] == shard_name]

    if service_type is not None:
        if not isinstance(service_type, list):
            service_type = [service_type]
        return [h for h in context.hosts if h['type'] in service_type]

    return context.hosts


def mongo_alive(context, hosts, **creds):
    """
    Connect to each host and count statuses
    """
    host_type = hosts[0]['type']
    connections = mongo_connect(context, hosts, creds, port=SERVICE_TYPE_PORT[host_type])
    # We have to force port of first host so We'll treat MongoInfra as MongoCFG
    # in case of we are working wit MongoCfg replset

    rs_roles = Counter()
    for conn in connections:
        mclient = conn['mc']
        dbname = creds['dbname']
        rs_roles[conn['role']] += 1
        try:
            mclient[dbname]['test'].find({}).count()
        except pymongo.errors.PyMongoError as exc:
            raise Exception('Error while querying "{0}/{1}" ({2}:{3}) via conn {4}: {5}'.format(
                conn['fqdn_port'], conn['dbname'], conn['host'], conn['port'], conn, repr(exc)))

    roles_expected = {
        'primary': {
            MONGOD_HOST_TYPE: 1,
            MONGOCFG_HOST_TYPE: 1,
            MONGOS_HOST_TYPE: len(hosts),
            MONGOINFRA_HOST_TYPE: len(hosts),
        },
        'secondary': {
            MONGOD_HOST_TYPE: len(hosts) - 1,
            MONGOCFG_HOST_TYPE: len(hosts) - 1,
            MONGOS_HOST_TYPE: 0,
            MONGOINFRA_HOST_TYPE: 0,
        },
    }

    for role, srv in roles_expected.items():
        assert rs_roles[role] == srv[host_type],\
            '{0} count expected "{1}" for srv {2}, but current roles: {3}'.\
            format(role, srv[host_type], host_type, rs_roles)


def wait_secondaries_sync(connections, timeout=30, check_delay=1):
    """
    Wait while secondaries sync all master data
    """
    primary_conn = next(iter(c for c in connections if c['is_primary']))
    start_ts = time.time()
    while True:
        try:
            rs_status = primary_conn['mc']['admin'].command({
                'replSetGetStatus': 1,
            })
            primary_optime = rs_status['optimes']['lastCommittedOpTime']
            for host in rs_status['members']:
                assert host['optime'] == primary_optime, \
                    'Host {0}: unexpected optime ts {1}, ' \
                    'primary optime was {2}\n' \
                    'rs.status: {3}'.\
                    format(host['name'], host['optime'], primary_optime,
                           pformat(rs_status))
            return
        except Exception as exc:
            if time.time() - start_ts > timeout:
                raise exc
            time.sleep(check_delay)


def check_mongos(context):
    """
    Connect to mongos and perform sharding checks
    """

    host_map = build_host_type_dict(context)
    host_map.pop(MONGOS_HOST_TYPE, None)
    host_map.pop(MONGOCFG_HOST_TYPE, None)
    host_map.pop(MONGOINFRA_HOST_TYPE, None)
    api_shards = {k: sorted([h['name'] for h in v]) for k, v in host_map.items()}

    connections = mongo_get_conns(context, MONGO_ADMIN_USER, [MONGOS_HOST_TYPE, MONGOINFRA_HOST_TYPE])
    for conn in connections:
        mongos_shards = {}
        for shard in conn['mc']['config']['shards'].find({}):
            # shard = rs01/man-r1xee1sh26lytdrv.pperekalov.local:27018,sas-...
            shard_id = shard['_id']
            rs_name, rs_hosts = shard['host'].split('/')
            assert_that(shard_id, equal_to(rs_name), 'shard id != replset name: {0}'.format(shard))
            mongos_shards[rs_name] = sorted([h.split(':')[0] for h in rs_hosts.split(',')])
        assert_that(api_shards, equal_to(mongos_shards), 'Shards state is different on host {0}'.format(conn['host']))


def mongodb_alive(context, **creds):
    """
    Connect to each host and count statuses
    """

    def get_user_creds(config):
        """ Get creds tp2-style """
        user = config['userSpecs'][0]
        user['dbname'] = user['permissions'][0]['databaseName']
        return user

    cluster_config = context.cluster_config

    if not creds:
        creds = get_user_creds(cluster_config)

    host_map = build_host_type_dict(context)

    # If we have both MongoCFG and MongoInfra, add MongoInfra to MongoCfg list
    # To check, that MOngoCfg replset has only one master
    if MONGOINFRA_HOST_TYPE in host_map and MONGOCFG_HOST_TYPE in host_map:
        host_map[MONGOCFG_HOST_TYPE] += host_map[MONGOINFRA_HOST_TYPE]
    for hosts in host_map.values():
        mongo_alive(context, hosts, **creds)

    if host_map.get(MONGOS_HOST_TYPE) or host_map.get(MONGOINFRA_HOST_TYPE):
        check_mongos(context)
