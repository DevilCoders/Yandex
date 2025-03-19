{%- from "components/greenplum/map.jinja" import gpdbvars with context -%}
#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, unicode_literals

import argparse
import logging
import subprocess
import datetime
import sys
import psycopg2
from psycopg2.extras import RealDictCursor
import threading
import os

GP_HOME = '{{ gpdbvars.gphome }}-{{ gpdbvars.gpmajver }}'
LOG_FILE = '{{ gpdbvars.gplog }}/daily_operations.log'

GP_ENV = os.environ.copy()
GP_ENV['PYTHONPATH'] = GP_HOME + '/lib/python'
GP_ENV['MASTER_DATA_DIRECTORY'] = '{{ gpdbvars.masterdir }}/master/{{ gpdbvars.segprefix }}-1'
GP_ENV['PATH'] = GP_HOME + '/bin:' + GP_ENV['PATH']

def load_query(get_tables):
    """load query from file"""
    path = '/usr/local/yandex/sqls'
    sqls = {'table_list': '{}/get_table_list_by_last_vacuum.sql'.format(path),
            'catalog_table_list':'{}/get_catalog_table_list.sql'.format(path)}
    with open(sqls[get_tables]) as inp_file:
        return inp_file.read()


def need_stop(start, stop):
    nowdt = datetime.datetime.now()
    now = nowdt.time()
    if not ( start < stop and now >= start and now < stop or start > stop and (now >= start or now < stop) ):
        conn = psycopg2.connect('user=gpadmin dbname=postgres')
        conn.autocommit = True
        cur = conn.cursor()
        cur.execute("SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE usename='gpadmin' AND pid!=pg_backend_pid() AND query~*'vacuum|analyze'")
        log.info('Its more than %s hour, will now stop. Bye!', stop)
        sys.exit()


def run_prep():
    log = logging.getLogger('PREPARE')
    conn = psycopg2.connect('user={{ gpdbvars.gpadmin }} dbname=postgres')
    conn.autocommit = True
    cur = conn.cursor()
    cur.execute('SELECT pg_is_in_recovery()')
    ro = (cur.fetchone()[0] is True)
    if ro:
        log.info('Replica. Nothing to do here.')
        return []
    cur.execute('SELECT datname FROM pg_database')
    filterOut = set(['template0', 'template1'])
    dbs = filter(lambda x: x not in filterOut, [i[0] for i in cur.fetchall()])
    conn.close()
    return dbs


def write_status_into_log_table(db, schema, table_name, action, status, start_time, stop_time):
    with psycopg2.connect('user={{ gpdbvars.gpadmin }} dbname=postgres') as conn:
        with conn.cursor() as cur:
            cur.execute("""
                INSERT INTO mdb_toolkit.db_daily_operations_log
                VALUES (%s,%s,%s,%s,%s,%s,%s)
            """,
            [db, schema, table_name, action, status, start_time, stop_time]
            )


def is_table_locked(conn, table, schema):
    with conn.cursor() as cur:
        cur.execute("""
            select count(*) > 0
            from pg_locks l
                join pg_class c on (relation = c.oid)
                join pg_namespace nsp on (c.relnamespace = nsp.oid)
            where l.locktype = 'relation'
                and ( c.relname = %(table)s or c.relname ~ %(tablepattern)s )
                and nsp.nspname = %(schema)s
                and mode in ('AccessExclusiveLock','ExclusiveLock','ShareUpdateExclusiveLock','ShareRowExclusiveLock','ShareLock')
        """,
        { 'table': table, 'tablepattern': '^' + table + '_.*', 'schema': schema}
        )
        is_locked = cur.fetchone()[0]
    return is_locked


def run_vacuum(db):
    with semaphore:
        action = 'VACUUM'
        log = logging.getLogger(action)
        log.info('STARTING %s ON DATABASE %s', action, db)
        with psycopg2.connect(user='gpadmin', dbname=db) as conn:
            with conn.cursor(cursor_factory=RealDictCursor) as cur:
                if not args.system:
                    cur.execute(load_query('table_list'))
                else:
                    cur.execute(load_query('catalog_table_list'))
                res = cur.fetchall()
        proc_per_db = args.max_processes_per_db
        threads_db = []
        for list_tables in [res[i::proc_per_db] for i in range(proc_per_db)]:
            thread_db = threading.Thread(
                target=run_vacuum_per_db,
                args=(list_tables, db, )
            )
            threads_db.append(thread_db)
            thread_db.start()
        for thread_db in threads_db:
            thread_db.join()
        log.info('FINISH %s ON DATABASE %s', action, db)


def run_vacuum_per_db(list_tables, db):
    action = 'VACUUM'
    log = logging.getLogger(action)
    with psycopg2.connect(user='gpadmin', dbname=db) as conn:
        conn.autocommit = True
        with conn.cursor() as cur:
            for row in list_tables:
                if args.stop_time:
                    need_stop(start, stop)
                schema = row['table_schema'].decode('utf-8')
                table = row['table_name'].decode('utf-8')
                log.info('%s: Processing table %s.%s', db, schema, table)
                start_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                if not is_table_locked(conn,table,schema):
                    try:
                        cur.execute('VACUUM "{}"."{}"'.format(schema, table))
                        status = 'SUCCESS'
                    except psycopg2.DatabaseError as error:
                        log.exception(error)
                        status = 'ERROR'
                else:
                    status = 'SKIPPED,EXCLUSIVELY LOCKED'
                stop_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                write_status_into_log_table(db, schema, table, action, status, start_time, stop_time)


def run_analyzedb(db):
    with semaphore:
        action='ANALYZEDB'
        log = logging.getLogger(action)
        log.info('Processing %s database', db)
        start_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        schema = ''
        if args.system:
            schema = '-spg_catalog'
        analyze = subprocess.Popen([GP_HOME + '/bin/analyzedb', '-a', '--full', '-p', args.max_processes, '-d', db, schema],
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE,
                                   env=GP_ENV)
        out, error = analyze.communicate()
        if analyze.returncode == 0:
            log.info('%s: Analyze completed', db)
            status = 'SUCCESS'
        else:
            log.error('%s: Analyze failed', db)
            status = 'ERROR'
        print(out, error)
        stop_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        write_status_into_log_table(db, schema, 'all', action, status, start_time, stop_time)
        log.info('FINISH %s ON DATABASE %s', action, db)

def run_reindex_catalog(db):
    with semaphore:
        action='REINDEXCATALOG'
        log = logging.getLogger(action)
        log.info('Processing %s database', db)
        start_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        reindex = subprocess.Popen([GP_HOME + '/bin/reindexdb', '--system', '-d', db],
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
        out, error = reindex.communicate()
        if reindex.returncode == 0:
            log.info('%s: Catalog reindex completed', db)
            status = 'SUCCESS'
        else:
            log.error('%s: Catalog reindex failed', db)
            status = 'ERROR'
        print(out, error)
        stop_time = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        write_status_into_log_table(db, 'pg_catalog', '', action, status, start_time, stop_time)


if __name__ == '__main__':
    arg = argparse.ArgumentParser(description="""
             Daily operations for Greenplum
             """)
    arg.add_argument(
        '-a',
        '--action',
        dest='action',
        #default='vacuum',
        help='set action, eg vacuum'
    )
    arg.add_argument(
        '-s',
        '--system',
        action='store_const',
        const=True,
        help='process system catalog only'
    )
    arg.add_argument(
        '--stop-time',
        dest='stop_time',
        help='set stop time H\:M'
    )
    arg.add_argument(
        '-r',
        '--reindex-catalog',
        action='store_const',
        const=True,
        help='reindex system catalog after vacuum'
    )
    arg.add_argument(
        '--max-processes',
        dest='max_processes',
        default=1,
        help='set max concurrent processes',
        type=int
    )
    arg.add_argument(
        '--max-processes-per-db',
        dest='max_processes_per_db',
        default=1,
        help='set max concurrent processes per database',
        type=int
    )
    arg.add_argument(
        '--analyzedb',
        action='store_const',
        const=True,
        help='run analyze after vacuum'
    )
    args = arg.parse_args()

    logging.basicConfig(
        filename=LOG_FILE,
        filemode='a',
        level='DEBUG',
        format='%(asctime)s [%(levelname)s] %(name)s:\t%(message)s')
    log = logging.getLogger('MAIN')

    if args.stop_time:
        start = datetime.datetime.now().time()
        stop = datetime.datetime.strptime(args.stop_time, '%H:%M').time()
        need_stop(start, stop)

    semaphore = threading.BoundedSemaphore(args.max_processes)
    threads = []

    if args.action == 'vacuum':
        for db in run_prep():
            thread = threading.Thread(
                target=run_vacuum,
                args=(db,)
            )
            threads.append(thread)
            thread.start()
        for thread in threads:
            thread.join()

    if args.reindex_catalog:
        for db in run_prep():
            thread = threading.Thread(
                target=run_reindex_catalog,
                args=(db,)
            )
            threads.append(thread)
            thread.start()
        for thread in threads:
            thread.join()

    if args.analyzedb:
        for db in run_prep():
            thread = threading.Thread(
                target=run_analyzedb,
                args=(db,)
            )
            threads.append(thread)
            thread.start()
        for thread in threads:
            thread.join()
