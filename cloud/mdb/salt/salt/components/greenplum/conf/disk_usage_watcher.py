#!/usr/bin/env python3
"""
Closes master if disk usage is too high
"""
import shutil
import logging
import os
import sys
import argparse
import psycopg2
import subprocess
from json import loads

RO_FLAG = '/tmp/.gp_ro'
CTRL_FILE = '/var/run/lock/instance_userfault_broken'
SOFT_LIMIT_DEFAULT = 97.0
SCRIPT = '/usr/local/yandex/disk_usage_watcher.py'


class ReplicaException(Exception):
    """
    Raised on connect to replica.
    """
    pass


def touch(fname):
    if os.path.exists(fname):
        return
    open(fname, 'a').close()


def kill_sessions(cur):
    """
    Kills all user sessions
    """
    cur.execute("""
        SELECT pg_terminate_backend(pid)
        FROM pg_stat_activity
        WHERE usename NOT IN ('gpadmin', 'monitor')
    """)


def get_db_and_status(cur):
    """
    Get all non-system dbs and default_transaction_read_only status
    """
    databases = {}
    cur.execute("""
        SELECT datname,
        COALESCE(setconfig @> '{default_transaction_read_only=true}'::text[], false) AS read_only
        FROM pg_database
        LEFT JOIN pg_db_role_setting
        ON (setdatabase=oid)
        WHERE  datname NOT IN ( 'postgres', 'diskquota' ) AND NOT datistemplate""")
    for row in cur.fetchall():
        escaped_dbname = '"{}"'.format(row[0])
        databases[escaped_dbname] = row[1]
    return databases


def close_database(cur, dbname):
    """
    SET default_transaction_read_only by default in specified db
    """
    cur.execute('ALTER DATABASE {} SET default_transaction_read_only TO true'.format(dbname))


def open_database(cur, dbname):
    """
    RESET default_transaction_read_only default in specified db
    """
    cur.execute('ALTER DATABASE {} RESET default_transaction_read_only'.format(dbname))


def usage(opts):
    """
    Returns used disk space in percents.
    """
    disk_usage = shutil.disk_usage(opts.path)
    used_ratio = disk_usage.used / disk_usage.total
    return used_ratio * 100


def usage_by_host(opts, log):
    usage_percents = []
    with open('/etc/dbaas.conf', 'r') as f:
        dbaas_conf = loads(f.read())
        cluster_hosts = dbaas_conf['cluster_hosts']
        for host in cluster_hosts:
            cmd = ['ssh', host, SCRIPT, '-p ', opts.path, '-d']
            cmd_run = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            out, err = cmd_run.communicate()
            log.debug(
                "cmd: '{}': code: {}, out: '{}', err: '{}'".format(cmd, cmd_run.returncode, out, err)
            )
            if out:
                usage_percents.append(float(out))
        return max(usage_percents)


def get_cur(host='localhost', database='postgres', user='gpadmin'):
    try:
        conn = psycopg2.connect(host=host, user=user, dbname=database, options='-c log_statement=none')
        conn.autocommit = True
        return conn.cursor()
    except psycopg2.OperationalError as e:
        return None


def acquire_master_cursor():
    """
    Connects to master
    """
    cur = get_cur()
    if cur is not None:
        return cur
    else:
        sys.exit(1)


def enforce_space_usage(log, opts):
    """
    1. Check used space.
    2. If usage is at or above `soft` limit, kill all sessions and set default
       transaction to be RO.
    """
    cursor = acquire_master_cursor()
    disk_used = usage_by_host(opts, log)
    dbs_status = get_db_and_status(cursor)
    log.debug('Used: %.3f', disk_used)
    kill = False
    # 1. Usage is under the limit.
    #    This a normal situation.
    #    Ensure no limits are enforced.
    if disk_used < opts.soft:
        for dbname in dbs_status:
            if dbs_status[dbname]:
                kill = False
                open_database(cursor, dbname)
                log.info('reset default_transaction_read_only for {}'.format(dbname))
        # MDB-9527
        if os.path.exists(CTRL_FILE):
            os.remove(CTRL_FILE)

    # 2. Usage is above soft limit.
    #    Set all user transactions to be RO by default.
    #    If there is no flag-file kill user sessions and create it.
    #    Flag file is used to prevent killing users` query on every invocation.
    #
    #    Note that this setting can be overridden by user.
    elif opts.soft <= disk_used:
        for dbname in dbs_status:
            if not dbs_status[dbname]:
                kill = True
                log.info('enforcing soft limit on %.3f%%', disk_used)
                close_database(cursor, dbname)
                log.info('set default_transaction_read_only for {}'.format(dbname))
        # MDB-9527
        touch(CTRL_FILE)

    if kill:
        log.info('kill all sessions after changing read-only status')
        kill_sessions(cursor)


def parse_args():
    """
    Process cmdline arguments.
    """
    arg = argparse.ArgumentParser(description="""
        Disk usage enforcing script
        """)
    arg.add_argument(
        '-s',
        '--soft',
        type=int,
        default=SOFT_LIMIT_DEFAULT,
        help='Soft limit. RO mode is activated and can be overridden by user',
    )
    arg.add_argument(
        '-p',
        '--path',
        type=str,
        default='/var/lib/greenplum',
        help='Pgdata root path',
    )
    arg.add_argument(
        '-f',
        '--flag',
        type=str,
        default=RO_FLAG,
        help='RO flag file path',
    )
    arg.add_argument(
        '-d',
        '--dry-run',
        action='store_true',
        help='Just disk usage print and exit'
    )
    parsed = arg.parse_args()
    return parsed


def main():
    """
    Entry point
    """
    logging.basicConfig(level=logging.DEBUG, format='%(message)s')
    log = logging.getLogger('disk_usage_limiter')
    try:
        args = parse_args()
        if args.dry_run:
            disk_usage = usage(args)
            print(f'{disk_usage:.3f}')
            sys.exit(0)
        enforce_space_usage(log, args)
    except Exception as exc:
        log.exception('error: %s', exc)
        sys.exit(1)


if __name__ == '__main__':
    main()
