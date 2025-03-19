#!/usr/bin/env python

import argparse
import json
import os
import time
import logging
import re

import redis
from redis import exceptions as rex

REDIS_RENAMES_CONF = '/etc/redis/redis-main.conf'
REDIS_DATADIR = '/var/lib/redis'
AOF_DIR_PATH = os.path.join(REDIS_DATADIR, 'appendonlydir')
AOF_PATH_PRE_7 = '{0}/appendonly.aof'.format(REDIS_DATADIR)
REDIS_PASS_FILE = '~/.redispass'
CONNECT_TIMEOUT = 1
GIGABYTE = 1024 ** 3
SIZE_CLEARANCE = 0.4  # 40 percent clearance just in case
REWRITE_INPROG = 'rewriting already in progress'
REDIS_VERSION_WITH_SPLITTED_AOF = 7
AOF_FILENAME_BASE_PATTERN = re.compile(r'appendonly.aof.\d+.base.rdb')
AOF_FILENAME_AOF_PATTERN = re.compile(r'appendonly.aof.\d+.incr.aof')

LOG = logging.getLogger('redis-force-aof')


def get_conf_file_lines(path):
    with open(path) as conf_file:
        for line in conf_file:
            yield line


def resolve_renamed_command(command, renames_file=REDIS_RENAMES_CONF, get_lines=get_conf_file_lines):
    for line in get_lines(renames_file):
        if line.startswith('rename-command') and command in line:
            _, _, cmd = line.split()
            assert cmd
            return cmd


def get_disk_free_space(path=REDIS_DATADIR):
    # Taken from py3`s shutil.disk_usage(), which is not available in py2.
    st = os.statvfs(path)
    return st.f_bavail * st.f_frsize


def get_password():
    with open(os.path.expanduser(REDIS_PASS_FILE)) as passfile:
        return json.load(passfile)['password']


def get_memory_limit(default=10 * GIGABYTE):
    try:
        with open('/etc/dbaas.conf') as conf_file:
            return json.load(conf_file)['flavor']['memory_limit']
    except IOError:
        return default


def connect(password, retry_attempts=30, **options):
    for attempt in range(retry_attempts):
        try:
            conn = redis.Redis(db=0, password=password, socket_connect_timeout=CONNECT_TIMEOUT, **options)
            if conn.ping():
                return conn
            raise RuntimeError('Connection is broken: initial ping failed')
        except (rex.ConnectionError, rex.TimeoutError) as err:
            if attempt >= retry_attempts - 1:
                raise
            LOG.warning('connection attempt failed with %s', type(err).__name__)
            time.sleep(1)


def rewrite_aof():
    password = get_password()
    rewrite_command = resolve_renamed_command('BGREWRITEAOF')
    conn = connect(password)
    try:
        conn.execute_command(rewrite_command)
        LOG.info('background rewrite command issued')
    # Happens during restarts
    except rex.BusyLoadingError:
        return
    # May happen while password is being changed by hs
    except rex.AuthenticationError:
        LOG.exception('Auth error')
    # Rewrite is already in progress
    except rex.ResponseError as err:
        if REWRITE_INPROG in str(err):
            LOG.info('background rewrite already in progress')
            return
        raise


def get_aof_size(redis_version):
    if redis_version < REDIS_VERSION_WITH_SPLITTED_AOF:
        return os.path.getsize(AOF_PATH_PRE_7)
    else:
        return sum([os.path.getsize(os.path.join(AOF_DIR_PATH, filename)) for filename in os.listdir(AOF_DIR_PATH) if (AOF_FILENAME_BASE_PATTERN.match(filename) or AOF_FILENAME_AOF_PATTERN.match(filename))])


def get_args():
    parser = argparse.ArgumentParser(description='Redis rewrite AOF tool')
    parser.add_argument('--redis_version', help="Target Redis version", default=6, type=int)
    args = parser.parse_args()
    return args


def main():
    """
    Make two checks:
     - Overall disk usage must be above the memory limit + some clearance to be sure.
     - AOF size must be above the memory limit
    """
    args = get_args()
    space_left = get_disk_free_space()
    memory_limit = get_memory_limit()
    aof_size = get_aof_size(args.redis_version)
    size_threshold = (memory_limit + memory_limit * SIZE_CLEARANCE)

    if space_left > size_threshold:
        return

    if aof_size < size_threshold:
        return

    LOG.info(
        'commencing background rewrite: %sG left, AOF is %sG',
        space_left / GIGABYTE,
        aof_size / GIGABYTE)
    return rewrite_aof()


if __name__ == '__main__':
    logging.basicConfig(
        level=logging.DEBUG,
        format='%(asctime)s [%(levelname)s] %(name)s: %(message)s')
    try:
        main()
    except Exception as exc:
        LOG.exception(exc)
