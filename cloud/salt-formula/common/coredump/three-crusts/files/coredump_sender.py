#!/usr/bin/env python

import argparse
import glob
import logging
import logging.handlers
import os
import re
import requests
import shutil
import socket
import subprocess
import sys
import time

from threading import Thread, Lock

import salt.config
import salt.loader


PY3 = sys.version_info >= (3, 0)
ARGS = None
LOG = logging.getLogger('main')
LOG.setLevel(logging.DEBUG)
CACHE_LOCK = Lock()


def _get_cluster_environment():
    with CACHE_LOCK:
        cache_file = '/tmp/coredump_sender_environment'
        if os.path.isfile(cache_file):
            return open(cache_file).read()

        opts = salt.config.minion_config('/etc/salt/minion')
        grains = salt.loader.grains(opts)
        environ = grains.get('cluster_map', {}).get('environment', 'unknown')
        open(cache_file, 'w').write(environ)
        return environ


class Core(object):
    compress_ext = '.lz4'
    compress_cmd = ['lz4', '-qz']

    def __init__(self, path):
        self.path = path

        # pattern is core.%E.%p.%s.%t[.lz4]
        r = re.match(
            r'core\.(?P<path>.+)(?: \(deleted\))?\.\d+\.\d+\.(?P<timestamp>\d+)(?:\.(?:lz4|gz|xz))?',
            path,
        )
        if r:
            # kernel replaces / with ! in full binary path
            self.exe_path = r.group('path').replace('!', '/')
            self.timestamp = r.group('timestamp')

        self.bad_name = (r is None)

        # .gz/xz -- old dumps from systemd-coredump
        self.compressed = path.endswith((self.compress_ext, '.gz', '.xz'))

        self.mtime = os.stat(path).st_mtime
        self.size = os.stat(path).st_size

    def _need_trace_send(self):
        if not ARGS.patterns:
            return True
        return any(regex.search(self.exe_path) for regex in ARGS.patterns)

    def process(self):
        ''' Send traceback if matches pattern, then compress '''

        if self.compressed:
            LOG.info('%s already compressed', self)
            return

        if not self.bad_name and self._need_trace_send():
            try:
                LOG.info('Generating traceback for %s', self)
                self.send_traceback()
            except Exception:
                LOG.exception('Failed sending trace for %s, not compressing', self)
                return

        try:
            LOG.info('Compressing %s', self)
            self.compress()
        except Exception:
            LOG.exception('exception while compressing %s', self)

    def _get_traceback(self):
        if not os.path.isfile(self.exe_path):
            # we cannot determine binary location
            LOG.warning('%s is not a file, cannot send traceback for %s', self.exe_path, self)
            return

        cmd = [
            'gdb',
            '--core=' + self.path,
            '--batch',
            '--eval-command=thread apply all bt',
            self.exe_path,
        ]
        LOG.debug('Running %s', cmd)
        return subprocess.check_output(cmd)

    def send_traceback(self):
        tb = self._get_traceback()
        if not tb:
            # No traceback
            return
        LOG.debug('Traceback: %s', tb.decode('utf8'))

        path_split = self.exe_path.split('/')
        generic_names = ['python', 'python3', 'python2', 'usr', 'bin']

        for part in reversed(path_split):
            if part not in generic_names:
                file_name = part
                break
        else:
            file_name = path_split[-1]

        params = {
            'ctype': _get_cluster_environment(),
            'service': file_name,
            'server': socket.gethostname(),
            'time': self.timestamp,
        }
        LOG.debug('Posting data with params %s', params)
        LOG.info('Sending traceback for %s', self)

        for _ in range(5):
            r = requests.post(ARGS.report_uri, params=params, data=tb)
            LOG.debug('Server returned %s', r)
            if r.status_code == 200:
                break
            time.sleep(3)
        else:
            r.raise_for_status()

    def compress(self):
        LOG.debug('Running %s', self.compress_cmd + [self.path])
        archive_fname = self.path + self.compress_ext
        subprocess.check_call(self.compress_cmd + [self.path, archive_fname])
        os.remove(self.path)

        self.compressed = True
        self.path = archive_fname

    def remove(self):
        os.remove(self.path)

    def __repr__(self):
        return '<Core from {}>'.format(self.path)


def find_cores():
    for path in glob.glob(ARGS.coredump_path + '/core.*'):
        if os.path.isfile(path):
            yield Core(path)


def get_disk_usage():
    if PY3:
        total, used, _ = shutil.disk_usage(ARGS.coredump_path)
        return used / total
    else:
        usage = os.statvfs(ARGS.coredump_path)
        return 1 - float(usage.f_bavail) / usage.f_blocks


def purge_old_cores(cores):
    cur_time = int(time.time())
    secs_ttl = ARGS.days_ttl * 24 * 60 * 60

    for core in cores[:]:
        if cur_time - core.mtime > secs_ttl:
            LOG.info('Removing core by TTL, %s', core.path)
            core.remove()
            cores.remove(core)

    cores.sort(key=lambda c: c.mtime)

    while get_disk_usage() > ARGS.usage_threshold:
        usage_perc = get_disk_usage() * 100
        if not cores:
            LOG.warning('No more cores, but usage is still high: %.2f%%', usage_perc)
            break
        oldest = cores[0]
        LOG.info('Removing core by usage threshold (%.2f%%), %s', usage_perc, core.path)
        oldest.remove()
        cores.remove(oldest)


def parse_args():
    def parse_regexes(value):
        return list(map(re.compile, value.split(',')))

    parser = argparse.ArgumentParser()
    parser.add_argument('--coredump-path', '-c', default='/var/crashes')
    parser.add_argument('--report-uri', '-u',
                        default='http://localhost:31337/corecomes')
    parser.add_argument('--patterns', '-p', default='', type=parse_regexes,
                        help='regexes of process filenames to send, comma-separated')
    parser.add_argument('--usage-threshold', '-t', default=0.8, type=float,
                        help='remove old cores while partition usage >thresh')
    parser.add_argument('--days-ttl', '-d', default=7, type=int,
                        help='after N days coredumps will be removed regardless of usage')
    parser.add_argument('--debug', '-v', action='store_true', help='print log to stdout')

    global ARGS
    ARGS = parser.parse_args()


def setup_logs():
    if ARGS.debug:
        handler = logging.StreamHandler(sys.stdout)
        handler.setLevel(logging.DEBUG)
        stdout_formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
        handler.setFormatter(stdout_formatter)
        LOG.addHandler(handler)

    syslog_formatter = logging.Formatter('coredump_sender %(levelname)s %(message)s')
    handler = logging.handlers.SysLogHandler(address='/dev/log')
    handler.setLevel(logging.INFO)
    handler.setFormatter(syslog_formatter)

    LOG.addHandler(handler)


def main():
    parse_args()
    setup_logs()

    cores = list(find_cores())
    LOG.debug('Found %s', cores)
    new_cores = [c for c in cores if not c.compressed]
    LOG.info('New cores %s', new_cores)

    threads = []
    for core in new_cores:
        thr = Thread(target=core.process)
        thr.start()
        threads.append(thr)

    for thr in threads:
        thr.join()

    purge_old_cores(cores)
    LOG.info('Done')


if __name__ == '__main__':
    main()
