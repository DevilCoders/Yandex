#!/usr/bin/env python

import psycopg2
import argparse
from contextlib import closing

parser = argparse.ArgumentParser()

parser.add_argument('-w', '--warn-limits',
                    type=str,
                    default='20 100',
                    help='Warning limit')

parser.add_argument('-c', '--crit-limits',
                    type=str,
                    default='1000 10000',
                    help='Critical limit')

parser.add_argument('-e', '--exclude',
                    type=str,
                    default='',
                    help='Names of error codes to ignore')

parser.add_argument('-r', '--exclude-on-replicas',
                    type=str,
                    default='',
                    help='Names of error codes to ignore in replicas')

parser.add_argument('-d', '--debug',
                    help='Raise exception if any.',
                    action='store_true')


def char2int(c):
    if '0' <= c <= '9':
        return int(c)
    return ord(c) - ord('A') + 17


def sqlstate2int(s):
    a = [char2int(e) for e in s]
    return sum([64**i*a[i] for i in range(len(a))])


def is_sqlstate(s):
    for e in s:
        if not ('0' <= e <= '9' or 'A' <= e <= 'Z'):
            return False
    return len(s) == 5


def replace_sqlstates_with_errnumbers(arr):
    for i in range(len(arr)):
        if is_sqlstate(arr[i]):
            arr[i] = 'NOT_KNOWN_ERROR: {0}'.format(sqlstate2int(arr[i]))


time_interval = 600
args = parser.parse_args()

exclude = args.exclude.split(',')
exclude_on_replicas = args.exclude_on_replicas.split(',')

replace_sqlstates_with_errnumbers(exclude)
replace_sqlstates_with_errnumbers(exclude_on_replicas)

fatal_warn_limit, error_warn_limit = [int(e) for e in args.warn_limits.split()]
fatal_crit_limit, error_crit_limit = [int(e) for e in args.crit_limits.split()]


def die(code=0, comment="OK"):
    if code == 0:
        print('0;OK')
    else:
        print('%d;%s' % (code, comment))


def top_n(_type, cur, n, is_replica):
    cur.execute("SELECT * from pg_log_errors_stats() WHERE type='{_type}'"
                " and time_interval={time_interval} ORDER BY count DESC "
                "LIMIT {n}"
                .format(_type=_type, time_interval=time_interval, n=n))
    result = cur.fetchall()
    stat = []
    for line in result:
        _, _, message, count = line[:4]
        if message in exclude:
            continue
        if is_replica and message in exclude_on_replicas:
            continue
        stat.append((message, count))

    return ', '.join('{0}({1})'.format(e[0], e[1]) for e in stat)


def kcache_check(cur):
    cur.execute("select extversion from pg_extension where extname='pg_stat_kcache'")
    version = cur.fetchone()[0]
    if version != '2.1.2':
        return ""
    cur.execute(
        "select saved_strings_count, available_strings_count from pgsk_get_buffer_stats()")
    saved_strings_count, available_strings_count = cur.fetchone()
    if saved_strings_count / available_strings_count > 0.75:
        return "kcache: saved_strings_count is too high"
    return ""


def calculate_count(_type, cur, is_replica):
    cur.execute("SELECT * from pg_log_errors_stats() WHERE type='{_type}' "
                "and time_interval={time_interval}".format(time_interval=time_interval, _type=_type))

    result = cur.fetchall()

    total = 0
    for line in result:
        _, _, message, count = line[:4]
        if message in exclude:
            continue
        if is_replica and message in exclude_on_replicas:
            continue
        total += count
    return total


try:
    with closing(psycopg2.connect('dbname=postgres user=monitor connect_timeout=1 host=localhost')) as conn:
        with closing(conn.cursor()) as cur:
            cur.execute("select pg_is_in_recovery()")
            is_replica = cur.fetchone()[0]
            fatals = calculate_count("FATAL", cur, is_replica)
            errors = calculate_count("ERROR", cur, is_replica)
            message = 'OK'
            if fatals > fatal_crit_limit or errors > error_crit_limit:
                die_code = 2
            elif fatals > fatal_warn_limit or errors > error_warn_limit:
                die_code = 1
            else:
                die_code = 0

            if die_code > 0:
                message = []
                if fatals > fatal_warn_limit:
                    message.append('FATALS: {0}'.format(top_n("FATAL", cur, 5, is_replica)))
                if errors > error_warn_limit:
                    message.append('ERRORS: {0}'.format(top_n("ERROR", cur, 5, is_replica)))
                message = '; '.join(message)
            kcache_message = kcache_check(cur)
            if kcache_message != "":
                die_code = 2
                message += kcache_message
            die(die_code, message)

except Exception as e:
    if args.debug:
        raise(e)
    die(1, "Could not get log_errors")
