#!/usr/bin/env python
import argparse
import json
import logging
import os
import redis
import time

from redis import exceptions as rex


REDIS_DATADIR = '/var/lib/redis'
REDIS_PASS_FILE = '~/.redispass'
REDIS_PORT = {{ salt.mdb_redis.get_redis_notls_port() }}

LOG = logging.getLogger('redis-userfault')


def get_password():
    try:
        with open(os.path.expanduser(REDIS_PASS_FILE)) as passfile:
            return json.load(passfile)['password']
    except IOError as err:
        LOG.error('failed to read redispass file %s', REDIS_PASS_FILE)
        raise


def no_memory_left(memory_threshold):
    password = get_password()

    try:
        redis_conn = redis.Redis(port=REDIS_PORT, password=password)
    except rex.ConnectionError as err:
        LOG.error('connection attempt failed with %s', type(err).__name__)
        raise

    info = redis_conn.info()
    policy = info['maxmemory_policy']
    if policy != 'noeviction':
        return False
    memory_left = info['maxmemory'] - info['used_memory']
    if memory_left <= memory_threshold:
        return True
    return False


def touch(fname):
    open(fname, 'a').close()


def drop(path):
    try:
        os.remove(path)
    except IOError as err:
        LOG.error('failed to remove file marker %s', path)
        raise


def get_disk_free_space(path=REDIS_DATADIR):
    # Taken from py3`s shutil.disk_usage(), which is not available in py2.
    st = os.statvfs(path)
    return st.f_bavail * st.f_frsize


def no_disk_left(disk_threshold):
    return get_disk_free_space() <= disk_threshold


def run(path, disk_threshold, memory_threshold):
    if no_disk_left(disk_threshold):
        if not os.path.exists(path):
            LOG.warning('no disk left on host, userfault mode on')
        touch(path)
    elif no_memory_left(memory_threshold):
        if not os.path.exists(path):
            LOG.warning('no memory left on host, userfault mode on')
        touch(path)
    elif os.path.exists(path):
        LOG.info('userfault mode off')
        drop(path)


def _get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-d',
        '--disk-threshold',
        type=int,
        default=128,
        help='Lower border for disk in Mb')
    parser.add_argument(
        '-m',
        '--memory-threshold',
        type=int,
        default=8,
        help='Lower border for memory in Mb')
    parser.add_argument(
        '-p',
        '--path',
        type=str,
        default='/var/run/lock/instance_userfault_broken',
        help='Userfault file marker path')
    args = parser.parse_args()
    to_bytes = lambda x: 1048576 * x
    return args.path, to_bytes(args.disk_threshold), to_bytes(args.memory_threshold)


if __name__ == "__main__":
    logging.basicConfig(
        level=logging.DEBUG,
        format='%(asctime)s [%(levelname)s] %(name)s: %(message)s')
    try:
        path, disk_threshold, memory_threshold = _get_args()
        run(path, disk_threshold, memory_threshold)
    except Exception as exc:
        LOG.exception(exc)
