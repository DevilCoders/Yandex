#!/usr/bin/pymds

from __future__ import print_function

import os
import json
import logging
import time
import pymongo
import argparse
import re

from kazoo.client import KazooClient
from kazoo.exceptions import NoNodeError
from functools import partial


logger = logging.getLogger('get-kz-mongo')
CONFIG_PATH = '/etc/elliptics/mastermind.conf'


def zk_walk(kz, path, min_depth, depth=1):
    try:
        children = kz.get_children(path)
    except NoNodeError:
        raise NoNodeError('node {0} not found'.format(path))

    for k in children:
        full_path = '{0}/{1}'.format(path, k)
        try:
            value, stats = kz.get(full_path)
        except NoNodeError:
            logger.debug('{0} disappeared'.format(full_path))
            continue
        if stats.numChildren > 0:
            for res in zk_walk(kz, full_path, min_depth, depth + 1):
                yield res
        elif depth < min_depth:
            logger.debug('Skipping {0} because depth {1} < {2}'.format(stats, depth, min_depth))
        else:
            yield full_path, value, stats


def get_mastermind_config(path):
    try:
        with open(path, 'r') as config_file:
            config = json.load(config_file)
        return config
    except Exception as e:
        raise ValueError('Failed to load config file %s: %s' % (CONFIG_PATH, e))


def get_locks(kz, paths, min_depth=1):
    return (res for path in paths for res in zk_walk(kz, path, min_depth))


def check_job(lock, kz, db):
    lock_path, lock_data, lock_stats = lock
    match = re.match(r'Job ([^,]+),', lock_data)
    if match:
        job_id = match.group(1)
    else:
        job_id = lock_data
    lock_age = (time.time() - lock_stats.ctime / 1000) / 3600
    job = db.jobs.find_one({'id': job_id})
    message = None
    if not job:
        message = "job '{0}' is lost for path {1}".format(job_id, lock_path)
    elif job['status'] in ('cancelled', 'completed'):
        message = "job {0} path: {1} type: {2} status: {3} age: {4:.2f} hours".format(
            job_id, lock_path, job['type'], job['status'], lock_age)
    elif job['status'] in ('pending',):
        match_task_lock = re.match(r'^Job ([a-z0-9]+), task ([a-z0-9]+)$', lock_data)
        if match_task_lock:
            message = "job {0} path: {1} type: {2} status: {3}, holds task locks, age: {4:.2f} hours".format(
                job_id, lock_path, job['type'], job['status'], lock_age)
    return message


def check_lock(lock, kz, age_limit, db):
    lock_path, _, lock_stats = lock
    lock_age = (time.time() - lock_stats.ctime / 1000) / 3600
    # /mastermind/locks/job/1-40ca07867ed84f8bb62f4dd59eb7a88a
    job_id = lock[0].split('/')[-1]
    job = db.jobs.find_one({'id': job_id})
    message = None
    if lock_age > age_limit:
        # FIXME: magic number based on /mastermind/locks hardcode
        short_path = '/'.join(lock_path.split('/')[3:])
        message = 'Job type {0} path {1} age: {2:.2f} hours'.format(job['type'], short_path, lock_age)
    return message


def main(args):
    action = args.action
    paths = args.paths
    human_friendly = args.human_friendly
    delete = args.delete
    age = args.age
    min_depth = args.min_depth
    force = args.force

    # FIXME: arguments mess
    config = get_mastermind_config(CONFIG_PATH)
    kz = KazooClient(config['sync']['host'], timeout=6)
    kz.start()

    if action == 'list':
        list_children(kz, paths)
        return

    locks = get_locks(kz, paths, min_depth)
    mongo_client = pymongo.mongo_replica_set_client.MongoReplicaSetClient(config['metadata']['url'])
    db = mongo_client[config['metadata']['jobs']['db']]
    if action == 'job':
        check = partial(check_job, kz=kz, db=db)
    elif action == 'lock':
        check = partial(check_lock, kz=kz, age_limit=age, db=db)
    else:
        raise ValueError('Unknown action "{0}"'.format(action))

    results = ((lock[0], check(lock=lock)) for lock in locks)
    fails = (f for f in results if f[1] is not None)

    if human_friendly:
        was_fails = False
        for lock_path, message in fails:
            was_fails = True
            logger.info('Fail found: {}'.format(message))
            print(message)
            if delete:
                # https://st.yandex-team.ru/MDS-6827
                if 'couples build' in message:
                    if force:
                        logger.warning('DELETE LOCK {0}'.format(lock_path))
                        kz.delete(lock_path)
                    else:
                        logger.warning('CAN\'T DELETE LOCK {0}. Use --force flag'.format(lock_path))
                else:
                    logger.warning('DELETE LOCK {0}'.format(lock_path))
                    kz.delete(lock_path)
        if not was_fails:
            print('All locks are good!')
    else:
        monrun_output(fails)


def monrun_output(fails):
    fails = list(fails)
    if fails:
        print('2;' + ', '.join(m for _, m in fails))
    else:
        print('0;OK')


def list_children(kz, paths):
    for path in paths:
        header = 'Children of {0}:'.format(path)
        try:
            children = kz.get_children(path)
        except NoNodeError:
            print('{0} not found'.format(path))
        else:
            print('\n  '.join([header] + children))


def get_args():
    parser = argparse.ArgumentParser(description='Check zk locks',
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('-p', '--paths', nargs='+', default=['planner'])
    parser.add_argument('-a', '--action', choices=['job', 'lock', 'list'], required=True, type=str,
                        help='job: treat lock value as job id and check it against db, '
                             'lock: check lock age, '
                             'list: show children')
    parser.add_argument('--age', type=float, default=2.0, help='consider locks older then age in hours as "bad"')
    parser.add_argument('--delete', action='store_true',
                        help='Delete locks detected as "bad". Only in human friendly mod')
    parser.add_argument('--min-depth', default=0, type=int, help='like "find"')
    parser.add_argument('-hf', '--human-friendly', action='store_true', help='Output for humans, not monrun')
    parser.add_argument('-f', '--force', action='store_true',
                        help='Force delete locks detected as "bad". Only in human friendly mod')
    parser.add_argument('--log-level', type=str, default=None, help='log level for stderr')
    parser.add_argument('--log-file', type=str, default='/var/log/get_kz_mongo.log')
    return parser.parse_args()


def setup_logging(log_level, log_file):
    root_logger = logging.getLogger()
    root_logger.setLevel('DEBUG')
    fmt = logging.Formatter(fmt='[%(asctime)s][%(name)s][%(levelname)s]: %(message)s')
    if log_level is not None:
        ch = logging.StreamHandler()
        ch.setLevel(log_level)
        ch.setFormatter(fmt)
        root_logger.addHandler(ch)
        root_logger.addHandler(ch)

    # always log to file for investigation
    fh = logging.FileHandler(filename=log_file)
    fh.setLevel('INFO')
    fh.setFormatter(fmt)
    root_logger.addHandler(fh)
    logger.debug('Log level set to debug')


if __name__ == '__main__':
    args = get_args()
    log_level, log_file = args.log_level, args.log_file
    if log_level is None and args.human_friendly:
        log_level = 'WARN'
    setup_logging(log_level, log_file)
    config = get_mastermind_config(CONFIG_PATH)
    base_path = config['sync']['lock_path_prefix']
    args.paths = [os.path.join(base_path, p) for p in args.paths]
    try:
        logger.info('run with args: {}'.format(args))
        main(args)
    except NoNodeError as e:
        logger.exception('script failed')
        print('0;script failed: {0}'.format(e))
    except Exception as e:
        logger.exception('script failed')
        print('1;script failed: {0}'.format(e))
