#!/usr/bin/env python

import psycopg2
import logging
import argparse

LOG_SUPPRESS_OPTIONS = (
    '-c log_statement=none -c log_min_messages=panic '
    '-c log_min_error_statement=panic -c log_min_duration_statement=-1'
)
UTILITY_OPTION = ' -c gp_session_role=utility'


def get_connection(host='localhost', user='gpadmin', database='postgres', port=5432, options=LOG_SUPPRESS_OPTIONS):
    try:
        conn = psycopg2.connect(
            dbname=database,
            user=user,
            host=host,
            port=port,
            options=options
        )
        return conn
    except psycopg2.OperationalError as e:
        logging.error('Could not connect to host %s:%s with error %s', host, port, e)
        exit(1)


def parse_args():
    arg = argparse.ArgumentParser(description='Greenplum orphan processes killer script')
    arg.add_argument(
        '--dry-run',
        required=False,
        action='store_true',
        help='Just print in log PIDs of orphaned processes and exit')
    arg.add_argument(
        '-l',
        '--log-dir',
        type=str,
        required=False,
        default='/var/log/greenplum',
        help='Folder to store logs')
    return arg.parse_args()


def search_and_destroy():
    conn = get_connection()
    with conn:
        with conn.cursor() as cur:
            cur.execute(
                """
                select
                    distinct l.mppsessionid,
                    l.pid,
                    gsc.hostname,
                    gsc.port
                from pg_catalog.pg_locks l
                left join pg_stat_activity a on a.sess_id = l.mppsessionid
                left join gp_segment_configuration gsc on gsc.content = l.gp_segment_id
                where l.locktype = 'relation'
                    and l.mppsessionid not in (
                                select sess_id
                                from pg_catalog.pg_stat_activity
                                where pid = pg_backend_pid()
                                )
                    and a.sess_id is null
                    and gsc.role = 'p'
                """
            )
            for mppsessionid, pid, hostname, port in cur.fetchall():
                logging.info('Found orphan process with pid %s on host %s', pid, hostname)
                if not args.dry_run:
                    conn = get_connection(host=hostname, port=port, options=LOG_SUPPRESS_OPTIONS+UTILITY_OPTION)
                    with conn:
                        with conn.cursor() as cur:
                            try:
                                cur.execute('select pg_terminate_backend({})'.format(pid))
                                logging.info('Orphan process with pid %s was killed on host %s', pid, hostname)
                            except psycopg2.DatabaseError as e:
                                logging.error('Can not kill orphan process with pid %s on host %s. Error is %s', pid, hostname, e)


if __name__ == '__main__':
    args = parse_args()
    logging.basicConfig(
        filename=args.log_dir + '/orphans_killer.log',
        filemode='a',
        level='INFO',
        format='%(asctime)s [%(levelname)s] %(name)s:\t%(message)s')
    logging.info('Starting search orphaned processes')
    search_and_destroy()
    logging.info('Finish')
