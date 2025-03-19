#!/usr/bin/python

from contextlib import closing
import argparse
import logging
import os
import time
import MySQLdb


def get_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('value',
        choices=['ON', 'OFF'],
        help="Specify desired innodb_status_output value")

    parser.add_argument('-d', '--delay-duration',
        type=int,
        default=15*60,
        help="How long wait before changing innodb_status_output to desired value")

    return parser.parse_args()


def get_conn():
    return MySQLdb.connect(read_default_file=os.path.expanduser("~mysql/.my.cnf"), db='mysql')


def get_innodb_status_output(conn):
    cur = conn.cursor()
    cur.execute('SELECT @@innodb_status_output')
    if int(cur.fetchone()[0]) > 0:
        return True
    return False


def set_innodb_status_output(conn, value):
    cur = conn.cursor()
    cur.execute('SET GLOBAL innodb_status_output = %(value)s', { 'value': value })


if __name__ == '__main__':
    args = get_args()
    try:
        with closing(get_conn()) as conn:
            actual = get_innodb_status_output(conn)
            logging.info('actual  innodb_status_output value {0}'.format('ON' if actual else 'OFF'))
            logging.info('desired innodb_status_output value {0}'.format('ON' if args.value else 'OFF'))
            if actual != (args.value == 'ON'):
                time.sleep(args.delay_duration)
                set_innodb_status_output(conn, args.value)

    except MySQLdb.Error as err:
        logging.exception(err)
