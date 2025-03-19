#!/usr/bin/env python

import argparse
import logging
import subprocess
import sys
import psycopg2


def die(code=0, comment="OK"):
    print('%d;%s' % (code, comment))
    sys.exit(0)


def check(args):
    rcode = 0
    msg = []
    unapplied = []
    # Gather unapplied settings
    conn = psycopg2.connect('dbname=postgres user=monitor connect_timeout=1 host=localhost')
    cur = conn.cursor()
    cur.execute("SELECT name, error FROM get_unapplied_settings()")
    unapplied = cur.fetchall()

    # Check postgresql.auto.conf
    cmd = "sudo cat %s | awk '/^[^#]/ {print $1}'" % args.pgdata
    proc = subprocess.Popen(
        cmd,
        shell=True,
        stdout=subprocess.PIPE,
        stderr=open('/dev/null', 'a'),
    )
    parameters = proc.communicate()[0].splitlines()
    exclude_names = args.ignore.split(',')

    res = len([i for i in parameters if i not in exclude_names])

    if not res and not unapplied:
        die(0, 'OK')

    if res:
        rcode = 1
        msg.append('%d setting(s) are redefined in postgresql.auto.conf' % res)

    if unapplied:
        rcode = 1
        named_settings = []
        for name, error in unapplied:
            if name:
                named_settings.append(name)
            else:
                msg.append(error)

        if named_settings:
            msg.append('needs restart: %s' % ','.join(named_settings))

    die(rcode, ' and '.join(msg))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--ignore',
                        type=str,
                        default='synchronous_standby_names',
                        help='Comma-separated list of parameters to ignore')
    parser.add_argument('-p', '--pgdata',
                        type=str,
                        default="/var/lib/pgsql/9.4/data/postgresql.auto.conf",
                        help='Path to postgresql.auto.conf')
    parser.add_argument('-d', '--debug',
                        action="store_true",
                        help='Enable debug output.')
    args = parser.parse_args()

    if args.debug:
        logging.basicConfig(level='DEBUG')
    else:
        logging.basicConfig(level='CRITICAL')

    try:
        check(args)
    except Exception:
        logging.exception('')
        die(1, "unable to check auto-conf or unapplied settings")

main()
