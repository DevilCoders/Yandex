#!/usr/bin/env python

import json
import socket
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


def get_failed_tasks(cluster_type):
    """
    Parse worker config and get number of failed tasks for local host
    """
    with open('/etc/dbaas-worker.conf') as inp:
        config = json.load(inp)

    for host in config['main']['metadb_hosts']:
        conn = get_conn(host, config['main']['metadb_dsn'])
        if conn:
            break

    if not conn:
        die(1, 'Unable to get conn with metadb')

    workers = {{ salt.pillar.get('data:k8s_workers', []) }}
    workers.append(socket.getfqdn())

    with conn as txn:
        cursor = txn.cursor()
        cursor.execute(
            """
            SELECT q.task_id FROM dbaas.worker_queue q
            JOIN dbaas.clusters c ON (q.cid = c.cid AND q.finish_rev = c.actual_rev)
            WHERE q.result = false
            AND q.worker_id in %(id)s
            AND c.type = %(type)s
            AND dbaas.error_cluster_status(c.status)
            AND (code.managed(c) OR q.errors->0->'exposable' = 'false'::jsonb OR task_type = 'hadoop_cluster_delete')
            AND NOT (c.name ~ 'dbaas_e2e' AND task_type ~ 'create')
            """,
            {'id': tuple(workers), 'type': cluster_type},
        )
        return [x[0] for x in cursor.fetchall()]


def _main():
    failed = get_failed_tasks(sys.argv[1])
    if failed:
        mdbui_hostname = "{{ salt.pillar.get('data:ui:address', '') }}"
        if mdbui_hostname and len(sys.argv) > 2 and sys.argv[2] == 'true':
            failed = ['https://{mdbui_hostname}/meta/workerqueue/{task_id}/change/'
                          .format(mdbui_hostname=mdbui_hostname, task_id=task_id) for task_id in failed]
        die(2, '{num} failed task(s): {failed}'.format(num=len(failed), failed=', '.join(failed)))
    else:
        die()


if __name__ == '__main__':
    _main()
