#!/usr/bin/env python
{% from "components/postgres/pg.jinja" import pg with context %}

import psycopg2
import sys
import os
import json
import datetime
import argparse
import subprocess
import time
from contextlib import closing


STATE_FILE_PATH = os.path.expanduser('/tmp/pg_xlog.prev')
STATE_XLOG_LAST_ARCHIVED_TIME = 'last_archived_time'
STATE_XLOG_LAST_FAILED_TIME = 'last_failed_time'
STATE_WAL_VERIFY_LAST_RUN_TS = 'wal_verify_run_ts'


def die(code=0, comment="OK"):
    if code == 0:
        print '0;OK'
    else:
        print('%d;%s' % (code, comment))
    sys.exit(0)


def count_files(path):
    proc = subprocess.Popen(
        "sudo -u postgres ls -1 %s 2>/dev/null | wc -l" % path,
        shell=True,
        stdout=subprocess.PIPE
    )
    stdout, _ = proc.communicate()
    return int(stdout.rstrip())


def count_xlogs(cur):
    wals_path = "{{ salt['pillar.get']('data:backup:archive:walsdir', pg.data + '/wals') }}",
{% if salt['pillar.get']('data:use_walg', True) %}
    cur.execute("select setting::int from pg_settings where name = 'server_version_num'")
    (version, ) = cur.fetchone()
    max_wals_query = None
    if version < 90500:
        xlog_path = "{{ pg.data + '/pg_xlog' }}"
        # See https://www.postgresql.org/docs/9.4/static/wal-configuration.html
        max_wals_query = """
            select greatest(
                (2 + current_setting('checkpoint_completion_target')::float) * current_setting('checkpoint_segments')::int + 1,
                current_setting('checkpoint_segments')::int + current_setting('wal_keep_segments')::int + 1
            )
        """
    else:
        xlog_path = "{{ pg.data + '/pg_wal' }}"
        max_wals_query = "select setting::int from pg_settings where name = 'max_wal_size'"
    cur.execute(max_wals_query)
    (max_wals, ) = cur.fetchone()
    xlogs = max(0, count_files(xlog_path) - max_wals)
{% else %}
    xlogs = 0
{% endif %}
    wals = count_files(wals_path)
    return xlogs + wals


def check_xlogs_count(warn_limit, crit_limit, conn, state):
    try:
        with closing(conn.cursor()) as cur:
            wals_count = count_xlogs(cur)
            if wals_count >= crit_limit:
                return 2, '%d not archived WALs.' % wals_count
            elif wals_count >= warn_limit:
                return 1, '%d not archived WALs.' % wals_count

            cur.execute("SELECT pg_is_in_recovery()")
            if cur.fetchone()[0] is True:
                return 0, "OK"

            cur.execute("select last_archived_time, last_failed_time " +
                        "from pg_stat_archiver;")
            res = cur.fetchone()
            if res[0] is None:
                res = (datetime.datetime(1970, 1, 1, 0), res[1])
            if res[1] is None:
                res = (res[0], datetime.datetime(1970, 1, 1, 0))

            prev_last_archived = state[STATE_XLOG_LAST_ARCHIVED_TIME]
            last_archived = state[STATE_XLOG_LAST_ARCHIVED_TIME] = int(res[0].strftime("%s"))
            last_failed = state[STATE_XLOG_LAST_FAILED_TIME] = int(res[1].strftime("%s"))

            if last_archived == prev_last_archived:
                if last_failed > last_archived + crit_limit:
                    return 2, "Archiver stuck at " + res[0].strftime("%F %H:%M:%S")
                elif last_failed > last_archived + warn_limit:
                    return 1, "Archiver stuck at " + res[0].strftime("%F %H:%M:%S")
                else:
                    return 0, "OK"

            return 0, "OK"
    except Exception:
        return 1, "Could not get info about not archived xlogs"


def check_wal_verify(checks, refresh_time, conn, state):
    prev_run_ts = int(state.get(STATE_WAL_VERIFY_LAST_RUN_TS, 0))
    cur_ts = int(time.time())
    time_delta = cur_ts - prev_run_ts
    if time_delta < refresh_time:
        return 0, "OK"

    try:
        with closing(conn.cursor()) as cur:
            cur.execute("select pg_is_in_recovery()")
            if cur.fetchone()[0] is True:
                return 0, "OK"

            output = run_wal_verify(checks)
            # wal-verify returns a list of check results
            # [ "check1" : { *result* }, "check2" : { *result* }, ... ]
            results = [parse_wal_verify_result(c_type, c_res) for c_type, c_res in output.items()]
            status, msg = merge_results(results)
            if status == 0: # update the last run time only if status is OK
                state[STATE_WAL_VERIFY_LAST_RUN_TS] = cur_ts
            return status, msg
    except Exception:
        return 1, "Failed to run wal-g wal-verify check"


def run_wal_verify(checks):
    args = ['/usr/bin/timeout', '45',
            '/usr/bin/wal-g', 'wal-verify',
            # list of checks will be inserted here
            '--config', '/etc/wal-g/wal-g-monitor.yaml',
            '--json']
    args[4:4] = checks  # insert provided checks
    proc = subprocess.Popen(args, stdout=subprocess.PIPE, stderr=open('/dev/null', 'a'))

    output = proc.stdout.read()
    return json.loads(output.decode("utf-8"))


def parse_wal_verify_result(check_type, check_details):
    status = check_details.get('status', None)
    if status is not None:
        if status == "OK":
            return 0, "OK"

        if status == "WARNING":
            return 1, "%s check: WARNING" % check_type

        if status == "FAILURE":
            return 2, "%s check: FAILURE" % check_type

    return 1, "Failed to parse %s check result" % check_type


def merge_results(check_results):
    check_results.sort(key=lambda r: r[0], reverse=True) # order by status
    messages = []
    worst_status = check_results[0][0]

    for status, message in check_results:
        if status != 0: # skip OK results
            messages.append(message)

    msg = ", ".join(messages) if len(messages) else "OK"
    return worst_status, msg


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-w', '--warn',
                        type=int,
                        default=300,
                        help='xlog check: warning limit')
    parser.add_argument('-c', '--crit',
                        type=int,
                        default=900,
                        help='xlog check: critical limit')
    parser.add_argument('-v', '--wal-verify-checks',
                        nargs="+",
                        required=False,
                        default=[],
                        help='wal-verify: list of checks to run')
    parser.add_argument('-t', '--wal-verify-refresh-time',
                        type=int,
                        default=86400, #24 hours
                        help='wal-verify: minimal delay between subsequent runs (in seconds)')

    return parser.parse_args()


def load_state():
    state = None
    if os.path.exists(STATE_FILE_PATH):
        with open(STATE_FILE_PATH, 'r') as f:
            try:
                state = json.loads(''.join(f.readlines()))
            except ValueError:
                state = None

    # check previous state integrity
    if state is not None\
        and any (k not in state for k in (STATE_XLOG_LAST_ARCHIVED_TIME,
                                          STATE_XLOG_LAST_FAILED_TIME,
                                          STATE_WAL_VERIFY_LAST_RUN_TS)):
        state = None

    if state is None:
        state = {
            STATE_XLOG_LAST_ARCHIVED_TIME: 0,
            STATE_XLOG_LAST_FAILED_TIME: 0,
            STATE_WAL_VERIFY_LAST_RUN_TS: 0,
        }

    return state


def save_state(state):
    with open(STATE_FILE_PATH, 'w') as f:
        f.write(json.dumps(state))


def main():
    args = parse_args()
    check_results = []
    try:
        with closing(psycopg2.connect('dbname=postgres user=monitor connect_timeout=1 host=localhost')) as conn:
            state = load_state()
            # XLogs check is always enabled
            status, msg = check_xlogs_count(args.warn, args.crit, conn, state)
            check_results.append((status, "xlogs check: " + msg))

            # Run wal-verify check, if specified
            if args.wal_verify_checks:
                status, msg = check_wal_verify(
                    args.wal_verify_checks, args.wal_verify_refresh_time, conn, state)
                check_results.append((status, "wal-verify: " + msg))

            # save the current state to local filesystem
            save_state(state)
            die_status, die_msg = merge_results(check_results)
            die(die_status, die_msg)
    except Exception:
        die(1, "Failed to run pg_xlog_files check")

if __name__ == '__main__':
    main()
