#!/usr/bin/env python3
"""
Disk usage watcher (PG only for now)
Closes master if disk usage is too high
"""
import shutil
import logging
import os
import sys
import argparse

import psycopg2

RO_FLAG = '/tmp/.pg_ro'
CTRL_FILE = '/var/run/lock/instance_userfault_broken'
SOFT_LIMIT_DEFAULT = 97.0

logging.basicConfig(level=logging.DEBUG, format='%(message)s')
log = logging.getLogger('disk_usage_limiter')


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
        WHERE usename NOT IN ('postgres', 'monitor')
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
        WHERE  datname != 'postgres' AND NOT datistemplate""")
    for row in cur.fetchall():
        escaped_dbname = '"{}"'.format(row[0])
        databases[escaped_dbname] = row[1]
    return databases


def close_database(cur, dbname):
    """
    SET default_transaction_read_only by default in all non-system dbs
    SET repl_mon.interval to zero for stop write wal.
    """
    cur.execute('ALTER DATABASE {} SET default_transaction_read_only TO true'.format(dbname))
    try:
        cur.execute('ALTER SYSTEM SET repl_mon.interval TO 0')
    except psycopg2.DataError:
        log.warning('repl_mon is out of date: cannot set interval to 0', exc_info=True)
    cur.execute('SELECT pg_reload_conf()')


def open_database(cur, dbname):
    """
    RESET default_transaction_read_only default in all non-system dbs
    RESET repl_mon.interval setting to default
    """
    cur.execute('ALTER DATABASE {} RESET default_transaction_read_only'.format(dbname))
    cur.execute('ALTER SYSTEM RESET repl_mon.interval')
    cur.execute('SELECT pg_reload_conf()')


def usage(opts):
    """
    Returns used disk space in percents.
    """
    disk_usage = shutil.disk_usage(opts.path)
    used_ratio = disk_usage.used / disk_usage.total
    return used_ratio * 100


def acquire_master_cursor(database='postgres'):
    """
    Connects to master ot throws an exception if localhost is a replica.
    """

    def is_master(conn):
        """
        Check if pg is master.
        """
        cur = conn.cursor()
        cur.execute('SELECT pg_is_in_recovery()')
        (is_replica,) = cur.fetchone()
        if is_replica or is_replica is None:
            return False
        return not is_replica

    conn = psycopg2.connect('dbname=%s options=\'-c log_statement=none\'' % database)
    conn.autocommit = True
    if is_master(conn):
        return conn.cursor()
    raise ReplicaException('Cannot operate on replica')


def enforce_space_usage(log, opts):
    """
    1. Check used space.
    2. If usage is at or above `soft` limit, kill all sessions and set default
       transaction to be RO.
    """
    cursor = acquire_master_cursor()
    disk_used = usage(opts)
    dbs_status = get_db_and_status(cursor)
    log.debug('Used: %.3f', disk_used)
    kill = False
    # 1. Usage is under the limit.
    #    This a normal situation.
    #    Ensure no limits are enforced.
    if disk_used < opts.soft:
        for dbname in dbs_status:
            if dbs_status[dbname]:
                kill = True
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
        default='/',
        help='Pgdata root path',
    )
    arg.add_argument(
        '-f',
        '--flag',
        type=str,
        default=RO_FLAG,
        help='RO flag file path',
    )
    parsed = arg.parse_args()
    return parsed


def main():
    """
    Entry point
    """
    try:
        args = parse_args()
        enforce_space_usage(log, args)
    except ReplicaException:
        # Not master. Bail out.
        sys.exit(0)
    except Exception as exc:
        log.exception('error: %s', exc)
        sys.exit(1)


if __name__ == '__main__':
    main()
