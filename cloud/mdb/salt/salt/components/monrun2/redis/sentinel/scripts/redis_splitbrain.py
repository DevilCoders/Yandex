#!/usr/bin/env python
"""
Check if local redis is in splitbrain
"""
import argparse
import json
import os
import socket
import sys
import time

from redis import Redis


STATE_FILE = '{{ salt.mdb_redis.get_splitbrain_state_file() }}'
DBAAS_PATH = '/etc/dbaas.conf'
REDISPASS = '~/.redispass'
REDIS_PORT = 6379
SENTINEL_PORT = 26379
NOTIFY_CODE = 2


class SplitbrainState:
    def __init__(self, path=STATE_FILE):
        self.path = path

    def set(self, data, code=NOTIFY_CODE):
        if not os.path.isfile(self.path):
            tmp_path = '{base}.tmp'.format(base=self.path)
            with open(tmp_path, 'w') as fobj:
                full_data = json.dumps({'msg': data, 'code': code})
                fobj.write(full_data)
            os.rename(tmp_path, self.path)

    def get(self):
        try:
            with open(self.path) as fobj:
                msg = fobj.read()
                if not msg:
                    code = 1
                    msg = 'Empty state file'
                else:
                    dct = json.loads(msg)
                    code = dct['code']
                    msg = dct['msg']
                return code, msg
        except Exception:
            return 1, 'Check state file format'

    def detect(self):
        hosts = get_cluster_hosts()
        password = get_password()
        conns = {host: get_conn(host, password=password) for host in hosts}
        masters_from_redis = get_redis_masters(conns)
        if len(masters_from_redis) > 1:
            self.set("redis masters: {}".format(sorted(masters_from_redis)))
            return

        if len(masters_from_redis) == 0:
            self.set("no redis masters found", code=1)
            return

        conns = {host: get_conn(host, port=SENTINEL_PORT) for host in hosts}
        masters_from_sentinel = get_sentinel_masters(conns)
        if len(masters_from_sentinel) > 1:
            msg = "sentinel masters: {}".format(sorted(masters_from_sentinel))
            self.set(msg, code=1)
            return

        if len(masters_from_sentinel) == 0:
            self.set("no sentinel masters found", code=1)
            return

        common_masters = masters_from_redis.union(masters_from_sentinel)
        if len(common_masters) > 1:
            msg = "possible RO: redis masters: {}; sentinel masters: {}".format(
                sorted(masters_from_redis), sorted(masters_from_sentinel))
            self.set(msg, code=1)
            return

        self.clear()

    def process(self, timeout):
        if file_is_not_old_enough(self.path, timeout):
            return 0, "OK"
        return self.get()

    def clear(self):
        if os.path.isfile(self.path):
            os.remove(self.path)


def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def file_is_not_old_enough(path, timeout):
    if not os.path.exists(path):
        return True
    return time.time() - os.stat(path).st_mtime <= timeout


def get_password():
    try:
        with open(os.path.expanduser(REDISPASS)) as redispass:
            return json.load(redispass)['password']
    except Exception as exc:
        die(1, "error reading {}: {}".format(REDISPASS, repr(exc)))


def get_conn(host, port=REDIS_PORT, password=None):
    conn = None
    try:
        conn = Redis(host=host, port=port, db=0, password=password)
    except:
        pass
    return conn


def get_cluster_hosts():
    if not os.path.exists(DBAAS_PATH):
        die(NOTIFY_CODE, 'no {}'.format(DBAAS_PATH))
    try:
        with open('/etc/dbaas.conf') as dbaas_conf:
            config = json.load(dbaas_conf)
            return config['cluster_hosts']
    except Exception as exc:
        die(1, "error reading {}: {}".format(DBAAS_PATH, repr(exc)))


def get_redis_masters(conns):
    masters = set()
    for host, conn in [(host, conn) for host, conn in conns.items() if conn is not None]:
        try:
            info = conn.info()
            if info['role'] == 'master' and not info['loading']:
                masters.add(host)
        except:
            continue
    return masters


def get_sentinel_masters(conns):
    masters = set()
    for host, conn in [(host, conn) for host, conn in conns.items() if conn is not None]:
        try:
            sentinel_masters = conn.sentinel_masters()
            for data in sentinel_masters.values():
                master_ip = data['ip']
                master_hostname = socket.gethostbyaddr(master_ip)[0]
                masters.add(master_hostname)
        except:
            continue
    return masters


def _main(path, timeout):
    splitbrain_state = SplitbrainState(path)
    splitbrain_state.detect()
    status, msg = splitbrain_state.process(timeout)
    die(status, msg)


def _get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-t',
        '--timeout',
        type=int,
        default=300,
        help='Time to restore container after moving, get quorum back and repair automatically')
    parser.add_argument(
        '-p',
        '--path',
        type=str,
        default=STATE_FILE,
        help='State file created by repair_sentinel.py utility on splitbrain detection')
    args = parser.parse_args()
    return args.path, args.timeout


if __name__ == '__main__':
    path, timeout = _get_args()
    _main(path, timeout)
