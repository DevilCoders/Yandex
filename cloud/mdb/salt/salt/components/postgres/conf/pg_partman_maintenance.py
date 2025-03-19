#!/usr/bin/env python
# -*- coding: utf-8 -*-

import psycopg2, threading,time

lock = threading.RLock()

def maintenance(conn,dbname):
    name = threading.currentThread().getName()
    cur = conn.cursor()
    lock.acquire(blocking=0)
    try:
        cur.execute("SELECT nspname FROM pg_catalog.pg_namespace WHERE oid IN "
                    "(SELECT extnamespace FROM pg_catalog.pg_extension "
                           "WHERE extname = 'pg_partman');")
        extnamespace = cur.fetchall()[0][0]
        cur.execute("UPDATE {}.part_config SET infinite_time_partitions=True;".format(extnamespace))
        conn.commit()
        cur.execute("SET lock_timeout to 0;")
        cur.execute("SELECT {}.run_maintenance(NULL, FALSE, FALSE);".format(extnamespace))
        print("Completed maintenance in %s." % dbname)
        conn.commit()
        cur.execute("SELECT parent_table FROM {}.part_config;".format(extnamespace))
        tables = [i[0] for i in cur.fetchall()]
        for table in tables:
            cur.execute("ANALYZE %s;" % table)
            print("Completed ANALYZE of %s in %s." % (table, dbname))
        conn.commit()
        cur.close()
    except psycopg2.Warning as e:
        print("WARNING: %s" % str(e))
    except psycopg2.Error as e:
        print("ERROR: %s" % str(e))
    finally:
        lock.release()
        conn.close()

def kill_func(conn,pid):
    kill_query = u"""SELECT
                       waiting.locktype           AS waiting_locktype,
                       waiting.relation::regclass AS waiting_table,
                       waiting_stm.query          AS waiting_query,
                       waiting.mode               AS waiting_mode,
                       waiting.pid                AS waiting_pid,
                       other.locktype             AS other_locktype,
                       other.relation::regclass   AS other_table,
                       other_stm.query            AS other_query,
                       other.mode                 AS other_mode,
                       other.pid                  AS other_pid,
                       other.granted              AS other_granted,
                       pg_terminate_backend(other.pid)
                   FROM
                       pg_catalog.pg_locks AS waiting
                   JOIN
                       pg_catalog.pg_stat_activity AS waiting_stm
                       ON (
                           waiting_stm.pid = waiting.pid
                       )
                   JOIN
                       pg_catalog.pg_locks AS other
                       ON (
                           (
                               waiting."database" = other."database"
                           AND waiting.relation  = other.relation
                           )
                           OR waiting.transactionid = other.transactionid
                       )
                   JOIN
                       pg_catalog.pg_stat_activity AS other_stm
                       ON (
                           other_stm.pid = other.pid
                       )
                   WHERE
                       NOT waiting.granted
                   AND
                       waiting.pid <> other.pid and waiting.pid=%(pid)s""";
    name = threading.currentThread().getName()
    cur = conn.cursor()
    cur.execute("SET temp_file_limit = '10MB'")
    i = 0
    while not lock.acquire(blocking=0) and (i<=60):
        time.sleep(1)
        i+=1
        cur.execute(kill_query, {'pid': pid})
        kill_session = cur.fetchall()
        for row in kill_session:
           print row
    lock.release()
    cur.close()

def main():
    conn = psycopg2.connect('dbname=postgres connect_timeout=1')
    conn.autocommit = True
    cur = conn.cursor()

    cur.execute("SELECT pg_is_in_recovery()")

    if cur.fetchone()[0] is True:
        return 0

    cur.execute("SELECT datname FROM pg_database WHERE "
                "datistemplate = false AND datname != 'postgres';")
    dbnames = cur.fetchall()
    dbnames = [i[0] for i in dbnames]
    for dbname in dbnames:
        conn_maintenance = psycopg2.connect('dbname=%s connect_timeout=1' % dbname)
        cur = conn_maintenance.cursor()
        cur.execute("SELECT extversion FROM pg_catalog.pg_extension "
                               "WHERE extname = 'pg_partman';")
        __ = cur.fetchall()
        if len(__) == 0:
            continue 
        cur.execute('select pg_backend_pid()');
        pid=cur.fetchone()[0]

        tm = threading.Thread(None, maintenance, 'Thread-select', (conn_maintenance,dbname))

        tk = threading.Thread(None, kill_func, 'Thread-kill', (conn, pid))

        tm.start()
        tk.start()
        tm.join()
        tk.join()

if __name__ == '__main__':
    main()
