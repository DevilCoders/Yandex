#!/usr/bin/env python3

import argparse
import json
import os
import warnings
from collections import namedtuple

import pymongo

# Ignore ssl-related warnings if any
warnings.simplefilter("ignore")

Result = namedtuple('Result', 'code, message')
DBAAS_CONF = '/etc/dbaas.conf'
USERFAULT_BROKEN_FLAG_FILE = '/tmp/load-monitor-userfault.flag'

DEFAULT_TIMEOUT_MS = 5000

SRV_TO_PORT = {
    'mongod': 27018,
    'mongocfg': 27019,
}


class NotDBAASException(Exception):
    pass


def readJSONFile(fname):
    with open(fname) as fobj:
        return json.load(fobj)


def get_mongodb_connection(uri):
    return pymongo.MongoClient(
        uri,
        socketTimeoutMS=DEFAULT_TIMEOUT_MS,
        connectTimeoutMS=DEFAULT_TIMEOUT_MS,
        serverSelectionTimeoutMS=DEFAULT_TIMEOUT_MS,
        waitQueueTimeoutMS=DEFAULT_TIMEOUT_MS,
    )


def get_subcluster_hosts_by_roles(dbaas, roles):
    """
    Filter subcluster by roles and return it's hosts
    """
    for data in dbaas.get('cluster', {}).get('subclusters', {}).values():
        if set(data['roles']).intersection(roles):
            return data['hosts'].keys()


def get_shard_hosts(srv):
    '''
    Get list of shard hosts from dbaas.conf
    '''
    try:
        dbaas = readJSONFile(DBAAS_CONF)
    except FileNotFoundError:
        raise NotDBAASException('Not mdb-dataplane cluster')

    if srv == 'mongod':
        return dbaas['shard_hosts']

    return get_subcluster_hosts_by_roles(dbaas, {'mongodb_cluster.mongocfg', 'mongodb_cluster.mongoinfra'})


def count_hosts(hosts, srv, uri_template):
    '''
    Return Primaries and Secondaries count
    '''
    port = SRV_TO_PORT.get(srv, None)
    if port is None:
        raise Exception('Unknown srv: {}'.format(srv))

    primary_cnt = 0
    secondary_cnt = 0
    unreachable_cnt = 0
    for host in hosts:
        try:
            with get_mongodb_connection(uri_template.format(host=host, port=port)) as conn:
                is_master = conn['admin'].command('isMaster')
                if is_master.get('ismaster', None):
                    primary_cnt += 1
                if is_master.get('secondary', None):
                    secondary_cnt += 1
        except (
            pymongo.errors.OperationFailure,
            pymongo.errors.ServerSelectionTimeoutError,
            pymongo.errors.NetworkTimeout,
            pymongo.errors.ConnectionFailure,
        ):
            unreachable_cnt += 1

    return (primary_cnt, secondary_cnt, unreachable_cnt)


def calculate_result(hosts_cnt, primary_cnt, secondary_cnt, unreachable_cnt):
    '''
    Calculate check result
    '''
    code = 0
    extra_msg = 'OK'
    if primary_cnt < 1:
        code = 1
        extra_msg = 'No primaries found'
    elif primary_cnt > 1:
        code = 2
        extra_msg = 'More than one primary found'
    elif unreachable_cnt > 0:
        code = 1
        extra_msg = 'Some hosts are unreachable'

    message = '{} (Primaries: {}, Secondaries: {}, Unreachable: {}, Hosts: {})'.format(
        extra_msg,
        primary_cnt,
        secondary_cnt,
        unreachable_cnt,
        hosts_cnt,
    )
    return Result(code=code, message=message)


def print_result(result):
    '''
    Print Result() as it expected by monrun
    '''
    print('%d;%s' % (result.code, result.message.replace('_', ' ').replace('\n', '')))


def _main():
    arg = argparse.ArgumentParser(description="""
                                  MongoDB Shard Primaries checker.
                                  """)
    arg.add_argument('-s', '--srv', type=str, required=True, help='MongoDB service to run check for', choices=['mongod', 'mongocfg'])
    arg.add_argument('-u', '--uri-template', required=True, type=str, metavar='<str>', help='URI to use when connecting')
    settings = vars(arg.parse_args())

    try:
        if os.path.exists(USERFAULT_BROKEN_FLAG_FILE):
            print_result(Result(code=0, message='Muted by userfault flag'))
        else:
            srv = settings.get('srv')
            uri_template = settings.get('uri_template')

            shard_hosts = get_shard_hosts(srv)

            (primary_cnt, secondary_cnt, unreachable_cnt) = count_hosts(shard_hosts, srv, uri_template)

            print_result(calculate_result(len(shard_hosts), primary_cnt, secondary_cnt, unreachable_cnt))
    except NotDBAASException as exc:
        print_result(Result(code=1, message=exc.message))
    except Exception as exc:
        print_result(Result(code=1, message=repr(exc)))


if __name__ == '__main__':
    _main()
