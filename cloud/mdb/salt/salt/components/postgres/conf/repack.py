#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import absolute_import, print_function, unicode_literals

import argparse
import collections
import logging
import os
import re
import subprocess
import sys
import time
from ConfigParser import SafeConfigParser

import psycopg2
from psycopg2.extras import RealDictCursor


def get_porto_connection():
    try:
        global porto
        import porto
    except ImportError:
        log.error('Porto requested but python-portopy is not installed.')
        sys.exit(1)

    conn = porto.Connection()
    conn.connect()
    return conn


def get_free_space():
    """
    Get free disk space
    """
    conn = psycopg2.connect('user=postgres dbname=postgres')
    conn.autocommit = True
    cur = conn.cursor()
    cur.execute('SHOW data_directory')
    pgdata = cur.fetchone()[0]
    stat = os.statvfs(pgdata)
    free = stat.f_bavail * stat.f_frsize
    return free


def load_query(bloat_type):
    """
    Load query from file
    """
    path = '/usr/local/yandex/sqls'
    sqls = {'table_bloat': '{}/table_bloat.sql'.format(path), 'index_bloat': '{}/index_bloat.sql'.format(path)}
    with open(sqls[bloat_type]) as inp_file:
        return inp_file.read()


def check_extension_version(dbname, repack_path):
    log = logging.getLogger('ext version check ' + dbname)
    installed_version = None
    conn = psycopg2.connect('user=postgres dbname=' + dbname)
    conn.autocommit = True
    try:
        cur = conn.cursor()
        cur.execute(
            "SELECT installed_version FROM pg_available_extensions "
            "WHERE installed_version IS NOT NULL AND name = 'pg_repack'"
        )
        installed_version = cur.fetchone()[0]
    except Exception:
        pass

    log.debug('Running "' + repack_path + ' --version"')

    repack = subprocess.Popen([repack_path, '--version'], stdout=subprocess.PIPE)

    repack.wait()
    expected_version = repack.stdout.readline().split()[1].rstrip()

    if installed_version is None:
        log.warning('pg_repack is not installed in %s' % dbname)
        cur = conn.cursor()
        cur.execute('CREATE EXTENSION pg_repack')
    elif expected_version != installed_version:
        log.warning('Old pg_repack installed in %s' % dbname)
        cur = conn.cursor()
        cur.execute('DROP EXTENSION pg_repack CASCADE')
        cur.execute('CREATE EXTENSION pg_repack')
    
def run_prep(config):
    log = logging.getLogger('prepare')
    conn = psycopg2.connect('user=postgres dbname=postgres')
    conn.autocommit = True
    cur = conn.cursor()
    cur.execute('SELECT pg_is_in_recovery()')
    ro = cur.fetchone()[0] is True
    if ro:
        log.info('Replica. Nothing to do here.')
        return []

    cur.execute('SELECT datname FROM pg_database')

    filter_out = set(['postgres', 'template0', 'template1'])
    dbs = []

    for dbname in filter(lambda x: x not in filter_out, [i[0] for i in cur.fetchall()]):
        check_extension_version(dbname, config.get('main', 'repack_path'))
        log.info('Processing ' + dbname)
        dbs.append(dbname)


    if config.get('main', 'use_porto') == 'yes':
        conn = get_porto_connection()
        try:
            container = conn.Find('self/index_repack')
        except porto.exceptions.ContainerDoesNotExist:
            container = conn.Create('self/index_repack')

        if container:
            if container.GetData('state') == 'stopped':
                container.SetProperty('isolate', False)
            container.SetProperty('cpu_limit', config.get('main', 'cpu_limit'))
            container.SetProperty('io_limit', config.get('main', 'io_limit'))
            if container.GetData('state') == 'stopped':
                container.Start()

    return dbs


def analyse_bloat_table(config, db):
    perc_min = config.getint('main', 'bloat_perc_min')
    bytes_min = config.getint('main', 'bloat_bytes_min')
    conn = psycopg2.connect('user=postgres dbname=' + db)
    cur = conn.cursor(cursor_factory=RealDictCursor)
    cur.execute(load_query('table_bloat'))
    res = cur.fetchall()
    to_repack = {}
    for row in res:
        if row['bloat_size'] > bytes_min and row['bloat_ratio'] > perc_min:
            table = '{}.{}'.format(row['schemaname'], row['tblname'])
            # index size after repack
            to_repack[table] = int(row['real_size']) - int(row['bloat_size'])
    return to_repack


def analyse_bloat_index(config, db):
    perc_min = config.getint('main', 'bloat_perc_min')
    bytes_min = config.getint('main', 'bloat_bytes_min')
    conn = psycopg2.connect('user=postgres dbname=' + db)
    cur = conn.cursor(cursor_factory=RealDictCursor)
    cur.execute(load_query('index_bloat'))
    res = cur.fetchall()
    to_repack = {}
    for row in res:
        if row['bloat_bytes'] > bytes_min and row['bloat_pct'] > perc_min:
            index = '{}.{}'.format(row['schema_name'], row['index_name'])
            # index size after repack
            to_repack[index] = int(row['index_bytes'] - row['bloat_bytes'])
    return to_repack


def skip_obj(config, obj):
    if config.has_option('main', 'except_obj_list'):
        raw_list = config.get('main', 'except_obj_list')
    else:
        raw_list = '^$'
    skip_regexp_list = raw_list.split(',')
    for skip_regexp in skip_regexp_list:
        if re.search(skip_regexp, obj):
            return True
    return False


def repack_table(config, db, table):
    log = logging.getLogger(table + ' repack')
    log.info('starting table repack')

    cmdline = [
        config.get('main', 'repack_path'),
        '--wait-timeout=' + config.get('main', 'wait-timeout'),
        '--table=' + table,
        db,
    ]
    return run_repack(config, db, table, cmdline)


def repack_index(config, db, index):
    log = logging.getLogger(index + ' repack')
    log.info('starting index repack')

    cmdline = [
        config.get('main', 'repack_path'),
        '--wait-timeout=' + config.get('main', 'wait-timeout'),
        '--index=' + index,
        db,
    ]
    return run_repack(config, db, index, cmdline)


def run_repack(config, db, obj, cmdline):
    log = logging.getLogger(obj + ' repack')
    log.info('starting repack')

    conn = psycopg2.connect('user=postgres dbname=postgres')
    conn.autocommit = True
    cur = conn.cursor()

    repack = subprocess.Popen(cmdline)

    if config.get('main', 'use_porto') == 'yes':
        conn = get_porto_connection()

    while repack.poll() is None:
        cur.execute(
            "SELECT pid FROM pg_stat_activity "
            "WHERE application_name = 'pg_repack' AND datname = '" + db + "'"
        )
        pids = [i[0] for i in cur.fetchall()]
        if pids:
            if config.get('main', 'use_porto') == 'yes':
                for pid in pids:
                    try:
                        conn.AttachProcess('self/index_repack', pid, config.get('main', 'postgres_comm'))
                    except porto.exceptions.PermissionError:
                        pass  # PID is already in the subcontainer

        time.sleep(10)

    if repack.returncode != 0:
        log.error('%s repack failed' % db)
    else:
        log.info('Repack of %s db completed.' % db)


if __name__ == '__main__':
    config = SafeConfigParser()
    arg = argparse.ArgumentParser(
        description="""
            pg_repack smart runner
            """
    )

    scope = arg.add_mutually_exclusive_group(required=True)

    scope.add_argument(
        '-t',
        '--tables',
        action="store_const",
        dest="scope",
        const="table",
        help='operate on tables'
    )

    scope.add_argument(
        '-i',
        '--indices',
        action="store_const",
        dest="scope",
        const="index",
        help='operate on tables` indices'
    )

    arg.add_argument(
        '-c',
        '--config',
        type=str,
        required=False,
        metavar='<path>',
        default='/etc/pg_index_repack.conf',
        help='path to config',
    )

    arg.set_defaults(scope='index')

    cmdargs = arg.parse_args()
    cmdvars = vars(cmdargs)

    config.read(cmdvars.get('config'))
    for arg, value in cmdvars.items():
        config.set(section='main', option=arg, value=value)

    logging.basicConfig(
        level=getattr(logging, config.get('main', 'log_level').upper()),
        format='%(asctime)s [%(levelname)s] %(name)s:\t%(message)s',
    )
    log = logging.getLogger('main')

    for db in run_prep(config):
        analyze_func = {'index': analyse_bloat_index, 'table': analyse_bloat_table}
        repack_func = {'index': repack_index, 'table': repack_table}
        reserv_perc = (100 - config.getfloat('main', 'reserv_perc')) / 100
        # analyse_bloat_index(...) or analyse_bloat_table(...)
        analyze_dict = analyze_func[cmdargs.scope](config, db)
        analyze = collections.OrderedDict(sorted(analyze_dict.items(), key=lambda t: t[1]))
        for obj in analyze:
            # reserve 3% free space, need check it before each repack
            free_space = int(get_free_space() * reserv_perc)
            # neeed 2x object size while repack
            if not skip_obj(config, obj) and free_space > analyze[obj] * 2:
                repack_func[cmdargs.scope](config, db, obj)
            else:
                log.info('Skip object [%s]' % repr(obj))

    if config.get('main', 'use_porto') == 'yes':
        conn = get_porto_connection()
        try:
            container = conn.Find('self/index_repack')
            if container.GetProperty('process_count') == '0':
                container.Destroy()
        except porto.exceptions.ContainerDoesNotExist:
            pass
