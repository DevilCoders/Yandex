#!/usr/bin/env python

import json
import sys

import psycopg2


def die(code=0, comment=''):
    """
    Properly format output for monitoring
    """
    if code == 0:
        print('0;OK')
    else:
        print('{code};{comment}'.format(code=code, comment=comment))
    sys.exit(0)


def get_conn(host, dsn):
    """
    Open connection with metadb
    """
    try:
        conn = psycopg2.connect('host={host} {dsn}'.format(host=host, dsn=dsn))
        cursor = conn.cursor()
        cursor.execute('select 1')
        if cursor.fetchone()[0] == 1:
            return conn
    except Exception:
        pass


def get_acquire_timeout():
    """
    Parse worker config and get tasks spent more then 60s in queue
    """
    with open('/etc/dbaas-worker.conf') as inp:
        config = json.load(inp)

    for host in config['main']['metadb_hosts']:
        conn = get_conn(host, config['main']['metadb_dsn'])
        if conn:
            break

    if not conn:
        die(1, 'Unable to get conn with metadb')

    with conn as txn:
        cursor = txn.cursor()
        cursor.execute(
            """
            SELECT task_id FROM dbaas.worker_queue
            WHERE start_ts IS NULL
            AND COALESCE(delayed_until, create_ts) + interval '60 seconds' < now() 
            AND required_task_id IS NULL
            AND worker_id IS NULL
            """
        )
        return [x[0] for x in cursor.fetchall()]


def _main():
    failed = get_acquire_timeout()
    if failed:
        die(2, '{num} task(s) spent more then 60s in queue: {failed}'.format(num=len(failed), failed=', '.join(failed)))
    
    die()


if __name__ == '__main__':
    _main()
