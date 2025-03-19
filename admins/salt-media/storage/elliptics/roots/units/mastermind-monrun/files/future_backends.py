#!/usr/bin/pymds

import time
import json
import socket
from pymongo import MongoClient
from collections import defaultdict
import argparse

from mds.admin.library.python.sa_scripts import utils
from mds.admin.library.python.sa_scripts import mm

utils.exit_if_file_exists('/var/tmp/disable_mastermind_monrun')

# query limit
n = 21
DAY = 86400
HOSTNAME = socket.gethostname()


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--monrun', action='store_true')
    parser.add_argument('--by_host', action='store_true')
    parser.add_argument('--check_path_and_remove', action='store_true')
    parser.add_argument('--dryrun', action='store_true')
    parser.add_argument('--hostname', type=str, default=None)
    args = parser.parse_args()
    return args


def get_db():
    with open('/etc/elliptics/mastermind.conf') as f:
        config = json.load(f)
        url = config['metadata']['url']
        db_name = config['metadata']['future_backends']['db']
    repl = MongoClient(url)
    return repl[db_name]


def get(hostname=None):
    db = get_db()
    if hostname:
        future = db.future_backends.find({'hostname': hostname})
    else:
        future = db.future_backends.find({})

    return future


def get_dc(hostname):
    if 'iva' in hostname:
        dc = 'iva'
    elif 'vla' in hostname:
        dc = 'vla'
    elif 'sas' in hostname:
        dc = 'sas'
    elif 'myt' in hostname:
        dc = 'myt'
    else:
        dc = 'man'

    return dc


@mm.mm_retry()
def search_by_path(mm_client, path, host):
    params = {
        'host': host,
        'path': path,
        'last': True
    }

    job_data = mm_client.request("search_history_by_path", [params])

    return job_data


@mm.mm_retry()
def remove_future_backend_record(path, previous_group_id, host, mm_client, dryrun):
    # path /srv/storage/22/8/
    # fs_path: /srv/storage/22; relative_path: 8
    fs_path, relative_path, _ = path.rsplit('/', 2)

    params = {
        'hostname': host,
        'fs_path': fs_path,
        'relative_path': relative_path,
        'previous_group_id': previous_group_id,
        'remover': 'future_backends',
    }

    if dryrun:
        remove_result = "DRYRUN"
    else:
        remove_result = mm_client.request("remove_future_backend_record", params)
    print "Remove {} future backends result: {}".format(params, remove_result)


def check_path_and_remove(p, k, info, host, mm_client, dryrun):
    path = "{}/{}/".format(p, k)
    new_info = search_by_path(mm_client, path, host)
    try:
        new_path = new_info[0]['set'][0]['path']
    except IndexError:
        new_path = ''
    try:
        new_group = new_info[0]['group']
    except Exception:
        new_group = 0

    group = info['previous_group_id']

    if path == new_path and group != new_group:
        remove_future_backend_record(path, group, host, mm_client, dryrun)
    else:
        print path, info, new_info


def check(hosts, hostname=None, by_host=False, fix=False, dryrun=False):
    result = defaultdict(int)

    if fix:
        mm_client = mm.mastermind_service_client(mm_hosts=[HOSTNAME], timeout=20)

    for host in hosts:
        not_removed = 0
        for path, b in host['future_backends'].iteritems():
            for k, fs_path in b.iteritems():
                create_ts = fs_path['created_ts']
                if int(time.time()) - create_ts > n * DAY:
                    not_removed += 1
                    if fix:
                        check_path_and_remove(path, k, fs_path, host['hostname'], mm_client, dryrun)
                    elif args.hostname:
                        print "{}/{} {}".format(path, k, fs_path)

        if by_host:
            dc = host['hostname']
        else:
            dc = get_dc(host['hostname'])

        result[dc] += not_removed
        result['all'] += not_removed

    return result


if __name__ == '__main__':
    args = get_args()

    backends = check(get(hostname=args.hostname), hostname=args.hostname, by_host=args.by_host, fix=args.check_path_and_remove, dryrun=args.dryrun)

    if args.monrun:
        all_backends = backends.get('all', 0)
        if all_backends > 0:
            print "2;{} old future backends".format(all_backends)
        else:
            print "0;OK"
    else:
        for dc, count in backends.iteritems():
            print dc, count
