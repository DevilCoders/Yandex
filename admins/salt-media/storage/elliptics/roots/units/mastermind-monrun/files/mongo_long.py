#!/usr/bin/pymds

import json
import os
import pymongo
import sys
import time

from collections import defaultdict

from mds.admin.library.python.sa_scripts import utils

utils.exit_if_file_exists('/var/tmp/disable_mastermind_monrun')

DAY = 86400

# Reading config

CONFIG_PATH = '/etc/elliptics/mastermind.conf'
try:
    with open(CONFIG_PATH, 'r') as config_file:
        config = json.load(config_file)
except Exception as e:
    print ('1;Failed to load config file {}: {}'.format(CONFIG_PATH, e))
    sys.exit(1)


# Connecting to database


def get_mongo_client():
    if not config.get('metadata', {}).get('url', ''):
        print ("1;Can't connect to MongoDB")
        sys.exit(1)
    return pymongo.MongoReplicaSetClient(config['metadata']['url'] + "&tls=true")


def check_fix_eblobs_job(job):
    t = (time.time() - job['create_ts']) / (1 * DAY)

    # MDS-9643
    # Last task - dnet_recovery.

    tasks = job['tasks']
    if (
        tasks[6]['type'] == 'dnet_client_backend_cmd'
        and tasks[6]['status'] == 'completed'
        and tasks[8]['type'] == 'recover_dc_group_task'
        and tasks[8]['status']
        in ['skipped', 'completed', 'need_resources', 'executing', 'executing']
    ):
        return 0

    return t


def get_jobs(days):
    mc = get_mongo_client()
    db_name = config.get('metadata', {}).get('jobs')['db']
    coll_name = 'jobs'
    coll = mc[db_name][coll_name]
    ts = int(time.time()) - DAY * days
    other_jobs = coll.find(
        {
            'status': {
                '$in': ['executing', 'new', 'pending', 'broken', 'not_approved', 'need_resources']
            },
            'type': {'$nin': ['move_job', 'restore_group_job', 'fix_eblobs_job']},
            'create_ts': {'$lt': ts},
        }
    )
    obligatory_jobs = coll.find(
        {
            'status': {
                '$in': ['executing', 'new', 'pending', 'broken', 'not_approved', 'need_resources']
            },
            'type': {'$in': ['move_job', 'restore_group_job', 'fix_eblobs_job']},
        }
    )
    return (obligatory_jobs, other_jobs)


def check(days):
    long_jobs = defaultdict(list)
    long_other_jobs = defaultdict(list)

    (jobs, other) = get_jobs(days)
    for job in jobs:
        if job['type'] == 'restore_group_job':
            t = (time.time() - job['create_ts']) / (4 * DAY)
        elif job['type'] == 'fix_eblobs_job':
            t = check_fix_eblobs_job(job)
        else:
            t = (time.time() - job['create_ts']) / (24 * DAY)

        if t > 1:
            long_jobs[job['type']].append(job['id'])
    for job in other:
        long_other_jobs[job['type']].append(job['id'])
    msg = ''
    if long_jobs:
        msg += 'Jobs run too long: {}. '.format(
            ['{}: {}'.format(str(j), ' '.join(long_jobs[j])) for j in long_jobs]
        )
    if long_other_jobs:
        msg += 'Jobs older than {} days: {}'.format(
            days, ['{}: {}'.format(str(j), ' '.join(long_other_jobs[j])) for j in long_other_jobs]
        )

    print ('2;{}'.format(msg) if msg else '0;OK')


if __name__ == '__main__':
    try:
        check(int(sys.argv[1]))
    except IndexError:
        print 'Type N - number of days to find jobs that are older than N'
