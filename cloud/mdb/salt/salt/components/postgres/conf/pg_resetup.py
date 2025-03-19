#!/usr/bin/env python
"""
Check if local postgresql is dead long enough and resetup it
"""

import argparse
import json
import logging
import os
import subprocess
import socket
import time
from datetime import datetime

import psycopg2
from kazoo.client import KazooClient

try:
    from ConfigParser import ConfigParser
except ImportError:
    from configparser import ConfigParser

REWIND_FAIL_FLAG = '/tmp/.pgsync_rewind_fail.flag'

STATUS_FILE = '/var/lib/postgresql/.pg_resetup_status.json'
DEFAULT_STATUS_JSON = {
    'time': 0.,
    'pg_status': {'v': None, 't': 0.},
    'pg_last_wal_replay_lsn': {'v': None, 't': 0.},
    'pg_latest_checkpoint': {'v': None, 't': 0.},
    'pg_wal_receiver_streaming': {'v': None, 't': 0.},
}

DEFAULT_CONFIG = {'stream_from': None}

logging.basicConfig(level=logging.DEBUG, format='%(asctime)s %(levelname)-8s: %(message)s')
LOG = logging.getLogger()


def load_last_status():
    status = None
    if os.path.exists(STATUS_FILE):
        with open(STATUS_FILE) as f:
            status = json.loads(f.read())
    status = status or DEFAULT_STATUS_JSON.copy()
    LOG.info('Load status: %s', json.dumps(status))
    return status


def save_status(status_json=None):
    status = status_json or DEFAULT_STATUS_JSON.copy()
    status['time'] = time.time()
    tmp_file = STATUS_FILE + '.tmp'
    with open(tmp_file, 'w') as f:
        f.write(json.dumps(status, sort_keys=True, indent=2))
        f.write('\n')
    os.rename(tmp_file, STATUS_FILE)
    LOG.info('Status file saved')


def parse_timeout(timeout):
    if type(timeout) == int:
        return timeout
    elif timeout.endswith('ms'):
        return int(timeout[:-2]) / 1000
    elif timeout.endswith('s'):
        return int(timeout[:-1])
    elif timeout.endswith('min'):
        return int(timeout[:-3]) * 60
    elif timeout.endswith('h'):
        return int(timeout[:-1]) * 60 * 60
    elif timeout.endswith('d'):
        return int(timeout[:-1]) * 60 * 60 * 24
    else:
        return 5 * 60   # default


def timed_value(value, now=None):
    return {
        'v': value,
        't': now or time.time()
    }


def get_latest_checkpoint(args):
    controldata = os.path.join('/usr/lib/postgresql', args.version, 'bin/pg_controldata')
    data_dir = os.path.join('/var/lib/postgresql', args.version, 'data')
    for c in str(subprocess.check_output([controldata, '-D', data_dir], stderr=subprocess.PIPE)).split('\n'):
        if c.startswith('Time of latest checkpoint'):
            dt = datetime.strptime(c[26:].strip(), '%a %d %b %Y %I:%M:%S %p %Z')
            return dt.strftime('%Y-%m-%d %H:%M%S')


def is_waiting_consistency(args, status):
    now = time.time()
    checkpoint_timeout = parse_timeout(args.checkpoint_timeout)

    try:
        current_latest_checkpoint_v = get_latest_checkpoint(args)
    except Exception:
        status['pg_status'] = timed_value('FAILED')
        return False

    recent_latest_checkpoint = status['pg_latest_checkpoint']
    if recent_latest_checkpoint['v']:
        LOG.info('Recent checkpoint: %s. Latest checkpoint: %s',
                 recent_latest_checkpoint['v'], current_latest_checkpoint_v)

        if recent_latest_checkpoint['v'] == current_latest_checkpoint_v:
            time_left = now - recent_latest_checkpoint['t']
            return time_left < checkpoint_timeout * 3

    status['pg_latest_checkpoint'] = timed_value(current_latest_checkpoint_v)
    return True


def get_wal_replay_lsn(cursor):
    try:
        cursor.execute('SELECT pg_last_wal_replay_lsn()')
        return cursor.fetchone()[0]
    except Exception:
        LOG.exception('Could not fetch pg_last_wal_replay_lsn.')
        raise


def check_wal_receiver_status(cursor):
    try:
        cursor.execute("SELECT * FROM pg_stat_wal_receiver WHERE status = 'streaming'")
        return bool(cursor.fetchone())
    except Exception:
        LOG.exception('Could not fetch pg_stat_wal_receiver status.')
        raise


def check_pg_status(dsn, status, read_timeout, lsn_timeout):
    """
    Check if postgresql is alive by running query
    and checking status file mtime
    """
    now = time.time()

    try:
        conn = psycopg2.connect(dsn)
        cursor = conn.cursor()
        cursor.execute('SELECT pg_is_in_recovery()')
        replica = cursor.fetchone()[0]
        if not replica:
            return True

        status['pg_status'] = timed_value('OK')

        last_lsn = status['pg_last_wal_replay_lsn']
        last_streaming = status['pg_wal_receiver_streaming']

        curr_lsn_v = get_wal_replay_lsn(cursor)
        curr_streaming_v = check_wal_receiver_status(cursor)

        if curr_lsn_v == last_lsn['v'] and curr_streaming_v == last_streaming['v'] and not curr_streaming_v:
            time_left = now - last_lsn['t']
            LOG.info("PG wal receiver not streaming and replay lsn is't changing for a %s s.", time_left)
            return time_left < lsn_timeout

        if curr_lsn_v != last_lsn['v']:
            status['pg_last_wal_replay_lsn'] = timed_value(curr_lsn_v)
        if curr_streaming_v != last_streaming['v'] or curr_streaming_v:
            status['pg_wal_receiver_streaming'] = timed_value(curr_streaming_v)

        return True

    except Exception:
        LOG.exception('PostgreSQL is dead.')
        last_pg_status = status['pg_status']

        if last_pg_status['v'] == 'FAILED':
            dead_time = now - last_pg_status['t']
            LOG.info('Pg dead for a %s seconds.', dead_time)
            return dead_time < read_timeout

        if last_pg_status['v'] == 'UNKNOWN':
            # We are in initial setup phase
            LOG.info('No alive pg in recorded history')
            return True

        status['pg_status'] = timed_value('FAILED')
        return True


def get_master(config):
    """
    Get master from zookeeper
    """
    hosts = config.get('global', 'zk_hosts')
    prefix = config.get('global', 'zk_lockpath_prefix')
    try:
        zk_conn = KazooClient(hosts=hosts)
        zk_conn.start()
        lock = zk_conn.Lock(os.path.join(prefix, 'leader'))
        contenders = lock.contenders()
        zk_conn.stop()
        if contenders:
            return contenders[0]
    except Exception:
        LOG.exception('Unable to get master from zk: ')
        return None


def resetup(config, args, timeout):
    """
    Drop local postgresql and run highstate with master arg
    """

    cmds = [
        'service pgsync stop',
        'service {} stop || true'.format(args.pooler),
        'service postgresql stop'
    ]

    for prefix in ['/var/lib', '/etc']:
        cmds.append('rm -rf ' + os.path.join(prefix, 'postgresql', args.version, 'data'))

    cmds.append('rm -f {flag_path}'.format(flag_path=REWIND_FAIL_FLAG))

    stream_from = config.get('global', 'stream_from')
    master = stream_from or get_master(config)
    if not master:
        raise RuntimeError('No one holds the leader lock')
    if master == socket.getfqdn():
        raise RuntimeError('Me and master are same host, this shouldn''t happen')

    with open(os.devnull, 'w') as out:
        for cmd in cmds:
            LOG.info('Executing %s', cmd)
            subprocess.check_call(cmd, stdout=out, stderr=out, preexec_fn=os.setsid, shell=True)

        hs_cmd = [
            'timeout', str(timeout), 'salt-call', 'state.highstate', 'queue=True',
            'pillar={value}'.format(value=json.dumps({
                'pg-master': master,
                'walg-restore': True,
                'sync-timeout': timeout,
            })),
        ]
        LOG.info(hs_cmd)
        try:
            subprocess.check_call(hs_cmd, stdout=out, stderr=out, preexec_fn=os.setsid)
        except subprocess.CalledProcessError:
            LOG.exception('HS Failed.')
            raise

    LOG.info('Done.')


def is_pg_dead(pgsync_conf, args, last_status):
    """
    Check if postgresql is dead and update last_status
    """
    if os.path.exists(REWIND_FAIL_FLAG):
        LOG.info('Rewind fail flag found.')
        return True

    dsn = '{dsn} sslmode=allow host=localhost'.format(dsn=pgsync_conf.get('global', 'append_rewind_conn_string'))
    timeout = pgsync_conf.getint('replica', 'recovery_timeout') * 10
    replay_timeout = parse_timeout(args.wal_replay_timeout)

    if check_pg_status(dsn, last_status, read_timeout=timeout, lsn_timeout=replay_timeout):
        return False

    return not is_waiting_consistency(args, last_status)


def main():
    pgsync_conf = ConfigParser(DEFAULT_CONFIG)
    pgsync_conf.read('/etc/pgsync.conf')

    parser = argparse.ArgumentParser()
    parser.add_argument(
        '-c', type=str, default='5min', dest='checkpoint_timeout',
        help="Resetup replica if checkpoint hasn't changed in checkpoint_timeout * 3."
    )
    parser.add_argument(
        # Long default needs for long analytical queries on replica.
        # Max query time is 12 hours, so 13 hours seems to be enough.
        '-w', type=str, default='13h', dest='wal_replay_timeout',
        help="Resetup replica if wal replay lsn hasn't changed in wal_replay_timeout."
    )
    parser.add_argument('-m', '--version', type=str, required=True, help='Major pg version.')
    parser.add_argument(
        '-p', '--pooler', type=str, choices=('pgbouncer', 'odyssey'), required=True,
        help='Connection pooler.'
    )
    args = parser.parse_args()

    status = load_last_status()
    if is_pg_dead(pgsync_conf, args, status):
        resetup(pgsync_conf, args, timeout=86370)
        status = DEFAULT_STATUS_JSON  # resetup -> reset status file
    save_status(status_json=status)


if __name__ == '__main__':
    main()
