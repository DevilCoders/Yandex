"""
Utilities for dealing with mongodb
"""

import copy
import time
from datetime import datetime
from pprint import pformat
from urllib.parse import quote_plus

import bson
import pymongo
from tenacity import (retry, retry_if_exception_type, stop_after_attempt, wait_fixed)

from . import crypto, docker

DB_COUNT = 1
TABLE_COUNT = 1
ROWS_COUNT = 1

WRAP_OPLOG_DB = 'wrap'
WRAP_STR_LEN = 1000000
WRAP_BATCH_SIZE = 1000

RS_NAME = 'replSet'
TIMEOUT = 10

STEP_DOWN_SECS = 60
SECONDARY_CATCH_UP_PERIOD_SECS = 30


def get_conn_opts_string(timeout=None):
    """
    Return connection options string
    """
    if timeout is None:
        timeout = TIMEOUT
    timeout_ms = timeout * 1000
    return '&'.join('{opt}={val}'.format(opt=opt, val=timeout_ms)
                    for opt in ('connectTimeoutMS', 'socketTimeoutMS', 'serverSelectionTimeoutMS'))


def get_mongod_port(context):
    """
    Get mongodb port from configuration.py
    """
    return context.conf['projects']['mongodb']['expose']['mongod']


def get_host_port(context, node):
    """
    Get host port of given container hostname
    """
    return '{host}.{net}:{port}'.format(host=node, net=context.conf['network_name'], port=get_mongod_port(context))


def mongod_connect(context, node_name, auth=True, write_concern=None):
    """
    Connect to mongodb container
    """
    if write_concern is None:
        write_concern = 2

    creds = context.conf['projects']['mongodb']['users']['admin'] if auth else None

    host, port = docker.get_exposed_port(docker.get_container(context, node_name), get_mongod_port(context))

    if creds:
        connect_uri = 'mongodb://{user}:{password}@{host}:{port}/{dbname}?{opts}'.\
            format(
                user=quote_plus(creds['username']),
                password=quote_plus(creds['password']),
                host=host,
                port=port,
                dbname=quote_plus(creds['dbname']),
                opts=get_conn_opts_string(),
            )
    else:
        connect_uri = 'mongodb://{host}:{port}'.format(host=host, port=port)

    return pymongo.MongoClient(connect_uri, j=False, w=write_concern)


@retry(retry=retry_if_exception_type(RuntimeError), wait=wait_fixed(3), stop=stop_after_attempt(10))
def mongod_primary_connect(context, node_list, ignore_unreach=False, write_concern=None):
    """
    Find in given node list and connect
    """
    primary_nodes = []
    connections = [mongod_connect(context, node, write_concern=write_concern) for node in node_list]

    for conn in connections:
        try:
            if conn['admin'].command('isMaster')['ismaster']:
                primary_nodes.append(conn)
        except pymongo.errors.ServerSelectionTimeoutError:
            if not ignore_unreach:
                raise

    if len(primary_nodes) != 1:
        raise RuntimeError('No or multiple primaries were found: {node_list}'.format(node_list=primary_nodes))

    return next(iter(primary_nodes))


def insert_test_data(conn, mark=None):
    """
    Fill test schema with data
    """
    data = {}
    if mark is None:
        mark = ''
    for db_num in range(1, DB_COUNT + 1):
        db_name = 'test_db_{db_num:02d}'.format(db_num=db_num)
        data[db_name] = {}
        for table_num in range(1, TABLE_COUNT + 1):
            rows = []
            table_name = 'test_table_{table_num:02d}'.\
                format(table_num=table_num)
            for row_num in range(1, ROWS_COUNT + 1):
                rows.append(gen_test_data_document(row_num=row_num, str_prefix=mark))

            conn[db_name][table_name].insert_many(rows)
            data[db_name][table_name] = rows[:]
    return data


def gen_test_data_document(row_num=0, str_len=5, str_prefix=None):
    """
    Generate test record
    """
    if str_prefix is None:
        str_prefix = ''
    else:
        str_prefix = '{prefix}_'.format(prefix=str_prefix)

    rand_str = crypto.gen_plain_random_string(str_len)
    return {
        'datetime': datetime.now(),
        'int_num': row_num,
        'str': '{prefix}{rand_str}'.format(prefix=str_prefix, rand_str=rand_str),
    }


def get_newest_oplog_rec(conn):
    """
    Get newest oplog record
    """
    return next(iter(conn['local']['oplog.rs'].find().sort([('$natural', -1)]).limit(1)))


def get_oldest_oplog_rec(conn):
    """
    Get oldest oplog record
    """
    return next(iter(conn['local']['oplog.rs'].find().sort([('$natural', 1)]).limit(1)))


def wrap_oplog(conn, db_name=None):
    """
    Wrap oplog
    """
    if db_name is None:
        db_name = WRAP_OPLOG_DB

    oldest_rec_ts = get_oldest_oplog_rec(conn)['ts'].time
    expected_rec_ts = get_newest_oplog_rec(conn)['ts'].time

    heavy_data = 'х' * WRAP_STR_LEN
    batch = [{'str': heavy_data} for _ in range(WRAP_BATCH_SIZE)]

    while oldest_rec_ts < expected_rec_ts:
        conn[db_name]['data'].insert_many(copy.deepcopy(batch), ordered=False)
        oldest_rec_ts = get_oldest_oplog_rec(conn)['ts'].time
    conn.drop_database(db_name)


def get_all_user_data(conn):
    """
    Retrieve all user data
    """
    user_data = []
    for db in sorted(conn.database_names()):
        tables = conn[db].collection_names(include_system_collections=False)
        for table in sorted(tables):
            if db in ['local', 'config']:
                continue
            for row in conn[db][table].find().sort([("_id", pymongo.ASCENDING)]):
                user_data.append((db, table, row))
    return user_data


def wait_primary_exists(conn, timeout=30, check_delay=1):
    """
    Wait until primary appears
    """
    start_ts = time.time()
    while True:
        try:
            cmd = conn.admin.command('replSetGetStatus')
            for member in cmd['members']:
                if member['stateStr'] == 'PRIMARY':
                    return
            raise RuntimeError('Primary was not found: {rs}'.format(rs=cmd))
        except Exception as exc:
            if time.time() - start_ts > timeout:
                raise exc
        time.sleep(check_delay)


def is_rs_initialized(conn):
    """
    Check replSet is initialized
    """
    try:
        return bool(conn.admin.command('replSetGetStatus'))
    except pymongo.errors.PyMongoError as err:
        if 'no replset config has been received' in str(err):
            return False
        raise RuntimeError('Unable retrieve replset status: {0}'.format(err))


def wait_secondaries_sync(primary_conn, timeout=30, check_delay=1):
    """
    Wait while secondaries sync all master data
    """
    start_ts = time.time()
    while True:
        try:
            rs_status = primary_conn['admin'].command({
                'replSetGetStatus': 1,
            })
            primary_optime = rs_status['optimes']['lastCommittedOpTime']
            for host in rs_status['members']:
                assert host['optime'] == primary_optime and host['stateStr'] in ['PRIMARY', 'SECONDARY'], \
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


def step_down(conn, freeze=None):
    """
    Run stepdown command
    """
    if freeze is None:
        freeze = STEP_DOWN_SECS
    try:
        cmd = bson.son.SON([('replSetStepDown', freeze), ('secondaryCatchUpPeriodSecs',
                                                          SECONDARY_CATCH_UP_PERIOD_SECS)])
        conn['admin'].command(cmd)
    except pymongo.errors.AutoReconnect:
        pass


def get_rs_member(conn, host_port):
    """
    Get host_port dictionary in from rs.status
    """
    cmd = conn['admin'].command('replSetGetStatus')
    for member in cmd['members']:
        if member['name'] == host_port:
            return member
    raise KeyError('Host {host_port} was not found in replset: {rs}'.format(host_port=host_port, rs=cmd))
