#!/usr/bin/pymds
# -*- coding: utf-8

from __future__ import print_function

import os
import ctypes
import argparse
import logging
import time
from glob import glob
from eblob_kit import files
from eblob_kit import find_duplicates
from eblob_kit import remove_duplicates
from subprocess import check_output
from concurrent.futures import ThreadPoolExecutor as Executor
from concurrent.futures import as_completed
from collections import defaultdict


logger = logging.getLogger()


def become_io_idle():
    return ctypes.CDLL('libc.so.6').syscall(251, 1, os.getpid(), 3 << 13)


def check_age(d):
    ids_file = os.path.join(d, 'kdb', 'ids')
    dupes_file = os.path.join(d, 'kdb', 'dupes')
    if os.path.exists(dupes_file):
        mtime = os.path.getmtime(dupes_file)
    elif os.path.exists(ids_file):  # assume this is date when group was 'created'
        mtime = os.path.getmtime(ids_file)
    else:
        mtime = 0
    return time.time() - mtime


def longest_path_match(s, matches):
    best_match = None
    for m in matches:
        if s.startswith(m):
            if best_match is None or len(m) > len(best_match):
                best_match = m
    return best_match


def collect_all_dupes(dirs, force=False):
    for d in dirs:
        collect_dupes(d, force)


def get_mounts():
    return set(l.split()[1] for l in open('/proc/mounts'))


def group_by_fs_and_order_by_time(dirs):
    mounts = get_mounts()
    res = defaultdict(list)
    for d in dirs:
        res[longest_path_match(d, mounts)].append(d)
    for fs in sorted(res, key=lambda x: max(check_age(d) for d in res[x]), reverse=True):
        yield sorted(res[fs], key=check_age, reverse=True)



def collect_dupes_by_fs(dirs, concurrency=10, force=False):
    with Executor(max_workers=concurrency) as pool:
        tasks = [
            pool.submit(collect_all_dupes, fs_dirs, force=force)
            for fs_dirs in group_by_fs_and_order_by_time(dirs)
        ]
        for task in as_completed(tasks):
            try:
                task.result()
            except Exception:
                logger.exception('Task failed')


def is_group(d):
    return os.path.exists(os.path.join(d, 'kdb'))


def collect_dupes(d, force=False):
    dupes_file = os.path.join(d, 'kdb', 'dupes')
    if os.path.exists(dupes_file):
        if not force:
            logger.info('{0} has dupes file and force flag not set. skipping'.format(d))
            return
    logger.info('Collect dupes for {0} with force={1}'.format(d, force))
    try:
        dupes = find_duplicates(files(os.path.join(d, 'data')))
        logger.info('Found {} dupes for {}'.format(len(dupes), d))

        for key in dupes:
            logger.debug('Found key: %s in blobs: %s', key.encode('hex'), ','.join(set([path for path, _ in dupes[key]])))

        with open(dupes_file, 'w') as f:
            f.write(str(len(dupes)))
    except Exception:
        logger.exception('Collect dupes for {0} failed'.format(d))


def find_dupes(dirs):
    dupes = []
    for d in dirs:
        if is_group(d):
            dupes_file = os.path.join(d, 'kdb', 'dupes')
            if os.path.exists(dupes_file):
                with open(dupes_file) as f:
                    try:
                        value = int(f.read())
                    except ValueError as e:
                        value = 'failed to read dupes count: {}'.format(e)
                    if value:
                        dupes.append((d, value))
    return dupes


def count_old(dirs, age):
    old, total = 0, 0
    for d in dirs:
        if is_group(d):
            days_old = check_age(d) // 86400
            if days_old > age:
                old += 1
            total += 1
    return old, total


def do_check(dirs, age):
    dupes = find_dupes(dirs)
    old, total = count_old(dirs, age)
    message = ''
    status = 0
    if old:
        status = 1
        message = 'Old checks: {} out of {}'.format(old, total) + ';' + message
    if dupes:
        status = 2
        message = ';'.join('{}: {}'.format(d, value) for d, value in dupes) + ';' + message
    if status == 0:
        message = 'OK'
    print('{};{}'.format(status, message))


def backend_id_by_dir(d):
    cmd = '''elliptics-node-info.py | grep 'datadir={0} ' | grep -Po 'backend_id=\K\d+' '''.format(d)
    return check_output(cmd, shell=True).strip()


def disable_backend(backend_id):
    logger.info('Disable backend {0}'.format(backend_id))
    logger.info(check_output('dnet_client backend disable --backend={0} --wait-timeout=1200'.format(backend_id), shell=True))


def enable_backend(backend_id):
    logger.info('Enable backend {0}'.format(backend_id))
    logger.info(check_output('dnet_client backend enable --backend={0} --wait-timeout=1200'.format(backend_id), shell=True))


def fix_dupes(dirs):
    dupes = find_dupes(dirs)
    for d, _ in dupes:
        backend_id = backend_id_by_dir(d)
        disable_backend(backend_id)

        logger.info('Start remove_duplicates for {0}'.format(d))
        remove_duplicates(files(os.path.join(d, 'data')))
        with open(os.path.join(d, 'kdb', 'dupes'), 'w') as f:
            f.write('0')

        enable_backend(backend_id)


def recollect(dirs):
    dirs_with_dupes = [d for d, v in find_dupes(dirs)]
    collect_all_dupes(dirs_with_dupes, force=True)


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('paths', nargs='*', default=glob('/srv/storage/*/*'))
    parser.add_argument('--check', action='store_true', help='do monrun check')
    parser.add_argument('--recollect', action='store_true', help='redo dupes collecting for paths with dupes')
    parser.add_argument('--age', type=int, default=3, help='if dupes file older then `age` days threat it as "not checked"')
    parser.add_argument('--force', action='store_true', help='force dupes collection even if dupes file present')
    parser.add_argument('--fix', action='store_true', help='run remove_dupes if dupes file contains non zero')
    parser.add_argument('--concurrency', default=5, type=int, help='Number of parallel collect_dupes to run')
    parser.add_argument('--log-level', default='DEBUG')
    return parser.parse_args()


if __name__ == '__main__':
    args = get_args()
    become_io_idle()
    logging.basicConfig(format='%(asctime)s %(process)d %(thread)d %(levelname)s: %(message)s', level=args.log_level)
    assert sum([args.check, args.fix, args.force, args.recollect]) <= 1, '--check, --fix, --recollect and --force must not be used together'
    dirs = [d for d in args.paths if is_group(d)]
    if args.check:
        do_check(dirs, args.age)
    elif args.fix:
        fix_dupes(dirs)
    elif args.recollect:
        recollect(dirs)
    else:
        collect_dupes_by_fs(dirs, args.concurrency, args.force)
