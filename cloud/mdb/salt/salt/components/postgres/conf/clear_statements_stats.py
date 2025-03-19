#!/usr/bin/env python

import psycopg2
import os

LIMIT_MB = 8

def main():
    conn = psycopg2.connect('dbname=postgres connect_timeout=1 options=\'-c log_statement=none\'')
    conn.autocommit = True
    cur = conn.cursor()
    cur.execute('SHOW stats_temp_directory;')
    temp_dir = cur.fetchone()[0]

    cur.execute('SHOW data_directory;')
    data_dir = cur.fetchone()[0]
    temp_data_dir = os.path.join(data_dir, 'pg_stat_tmp')

    if not temp_dir.startswith('/'):
        temp_dir = os.path.join(data_dir, temp_dir)

    size = 0
    for root, dirs, files in os.walk(temp_dir):
        size += sum([os.path.getsize(os.path.join(root, name))
                for name in files])
    for root, dirs, files in os.walk(temp_data_dir):
        size += sum([os.path.getsize(os.path.join(root, name))
                for name in files])

    if size < LIMIT_MB * 1024 * 1024:
        return None

    cur.execute('SHOW shared_preload_libraries;')
    libs = cur.fetchone()[0]
    if 'pg_stat_statements' in libs:
        cur.execute('SELECT pg_stat_statements_reset();')
    if 'pg_stat_kcache' in libs:
        cur.execute('SELECT pg_stat_kcache_reset();')

    cur.close()
    conn.close()

if __name__ == '__main__':
    main()

