{%- set config_map = salt['mdb_greenplum.get_map']() -%}
#!/usr/bin/env python

import psycopg2
import sys
import os
import json
import datetime
import argparse
import subprocess
from socket import getfqdn
from contextlib import closing


GP_MAP = {{ config_map }}
STATE_FILE_PATH = os.path.expanduser('/tmp/pg_xlog.prev')
STATE_XLOG_LAST_ARCHIVED_TIME = 'last_archived_time'
STATE_XLOG_LAST_FAILED_TIME = 'last_failed_time'


def die(code=0, comment="OK"):
    if code == 0:
        print('0;OK')
    else:
        print('%d;%s' % (code, comment))
    sys.exit(0)


def get_host_info(fqdn):
    # returns list of (role, port, datadir)
    result = []
    for role, hosts in GP_MAP.items():
        for host in hosts:
            if host.startswith(fqdn):
                result.append((role, host.split('~')[2], host.split('~')[3]))
    return result


def get_connstring(role, port):
    if role in {'master', 'standby'}:
        return "host=localhost dbname=postgres user=gpadmin"
    elif role in {'primary', 'mirror'}:
        return "host=localhost dbname=postgres user=gpadmin port={} options='-c gp_session_role=utility'".format(port)


def count_files(path):
    proc = subprocess.Popen(
        "ls -1 %s 2>/dev/null | wc -l" % path,
        shell=True,
        stdout=subprocess.PIPE
    )
    stdout, _ = proc.communicate()
    return int(stdout.rstrip())


def check_xlogs_count(warn_limit, crit_limit, conn, datadir, state):
    try:
        wals_count = count_files(datadir + '/pg_xlog')
        msg = "{} not archived WALs at {}".format(wals_count, datadir)
        if wals_count >= crit_limit:
            return 2, msg
        elif wals_count >= warn_limit:
            return 1, msg

        if conn:
            with closing(conn.cursor()) as cur:
                cur.execute("select last_archived_time, last_failed_time from pg_stat_archiver;")
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
        return 0, "OK"
    except Exception:
        return 1, "Could not get info about not archived xlogs"


def merge_results(check_results):
    if not check_results:
        return 0, "OK"

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
                        default=100,
                        help='xlog check: warning limit')
    parser.add_argument('-c', '--crit',
                        type=int,
                        default=300,
                        help='xlog check: critical limit')
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
    if state is not None and any (k not in state for k in (STATE_XLOG_LAST_ARCHIVED_TIME, STATE_XLOG_LAST_FAILED_TIME)):
        state = None

    if state is None:
        state = {
            STATE_XLOG_LAST_ARCHIVED_TIME: 0,
            STATE_XLOG_LAST_FAILED_TIME: 0,
        }
    return state


def save_state(state):
    with open(STATE_FILE_PATH, 'w') as f:
        f.write(json.dumps(state))


def main():
    args = parse_args()
    check_results = []

    host_info = get_host_info(getfqdn())
    try:
        for role, port, datadir in host_info:
            try:
                conn = psycopg2.connect(get_connstring(role, port))
            except psycopg2.OperationalError:
                conn = None
            state = load_state()
            status, msg = check_xlogs_count(args.warn, args.crit, conn, datadir, state)
            check_results.append((status, "xlogs check: " + msg))
            save_state(state)
        die_status, die_msg = merge_results(check_results)
        die(die_status, die_msg)
    except Exception as e:
        die(1, "Failed to run pg_xlog_files check %s" % repr(e).replace('\n', ' '))

if __name__ == '__main__':
    main()
