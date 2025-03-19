#!/usr/bin/env python2
"""
Sequently set task to worker for reissue certs for specified cids in metadb
"""

import argparse
import pwd
import random
import os
import string
import sys
import time

from copy import deepcopy

from psycopg2 import connect
from psycopg2.extras import RealDictCursor


CLUSTER_TYPE_MAP = {
    'pg': 'postgresql_cluster',
    'mg': 'mongodb_cluster',
    'ch': 'clickhouse_cluster',
    'my': 'mysql_cluster',
}

TOTAL_WAIT_TASK_SEC = 60 * 60


def _wait_task(cursor, op_id):
    sleep_time_sec = 5.
    sleep_time_max_sec = 15 * 60
    start_time = time.time()
    while time.time() - start_time < TOTAL_WAIT_TASK_SEC:
        print('waiting operation: %s for %d sec' % (op_id, sleep_time_sec))
        time.sleep(sleep_time_sec)
        sleep_time_sec = min(sleep_time_sec * 1.5, sleep_time_max_sec)
        cursor.execute(
            """
            SELECT result FROM dbaas.worker_queue WHERE
                task_id=%(operation_id)s::text
            """, {'operation_id': op_id})
        res = cursor.fetchone()
        if not res:
            raise RuntimeError('failed to get requested operation: %s' % op_id)
        task_res = res['result']
        if task_res is None:
            print('there are no result for operation: %s, res: %s' % (op_id, repr(res)))
            continue
        else:
            print('got result: %s for operation: %s' % (task_res, op_id))
            break


def _reissue(metadb_conn_str, type_cluster, task_suffix, allow_clusters):
    print("connection: '%s'" % metadb_conn_str)
    with connect(metadb_conn_str, cursor_factory=RealDictCursor) as conn:
        cursor = conn.cursor()
        cursor.execute("""
            SELECT cid, folder_id, actual_rev
              FROM dbaas.clusters
             WHERE status not in ('STOPPED', 'PURGED', 'DELETED', 'MODIFY-ERROR', 'CREATE-ERROR') and type=%(type_cluster)s
            """, {'type_cluster': type_cluster})
        db_clusters = deepcopy(cursor.fetchall())
        print("get clusters from MetaDB, total len: %d" % len(db_clusters))

        counter = 0
        total_count = len(allow_clusters)
        if not total_count:
            return
        for db_ci in db_clusters:
            cid = db_ci['cid']
            if cid not in allow_clusters:
                continue

            cursor = conn.cursor()
            # clean certificate from pillar
            cursor.execute("""
                UPDATE dbaas.pillar
                SET value = value - 'cert.crt' - 'cert.key'
                WHERE fqdn IN (SELECT fqdn FROM dbaas.hosts JOIN dbaas.subclusters USING(subcid) JOIN dbaas.clusters USING(cid) WHERE cid=%(cid)s)
                """, {'cid': cid})

            counter += 1
            task_type = type_cluster + '_update_tls_certs'
            op_id = '_'.join(('mdb_crtup', type_cluster, task_suffix, str(counter)))
            print(" * [%d/%d] run task: %s, type: %s, cid: %s" %
                  (counter, total_count, op_id, task_type, cid))
            cursor.execute(
                """
                SELECT * FROM code.add_operation(
                    i_operation_id   => %(operation_id)s::text,
                    i_cid            => %(cid)s::text,
                    i_folder_id      => %(folder_id)s::bigint,
                    i_operation_type => %(task_type)s,
                    i_task_type      => %(task_type)s,
                    i_task_args      => '{ }'::jsonb,
                    i_metadata       => '{ }'::jsonb,
                    i_user_id        => 'crt_reissue_script',
                    i_version        => 2,
                    i_hidden         => true,
                    i_rev            => %(rev)s
                )
                """, {
                    'operation_id': op_id,
                    'cid': cid,
                    'folder_id': db_ci['folder_id'],
                    'task_type': task_type,
                    'rev': db_ci['actual_rev'],
                })
            res = cursor.fetchall()
            print("run task result: %s" % repr(res))
            conn.commit()
            _wait_task(cursor, op_id)
        if counter != total_count:
            print("WARNING!! Not all cids updated, either wrong cid, either cluster in invalid state.\n"
                  "updated %d of %d" % (counter, total_count))


def validate_args(args):
    if args.db not in CLUSTER_TYPE_MAP.keys():
        print('db type should be one of: %s, your argument: "%s"' % (
            ', '.join(CLUSTER_TYPE_MAP.keys()), args.db))
        sys.exit(1)
    if not args.task_suffix:
        random.seed(time.time())
        args.task_suffix = ''.join([random.choice(string.ascii_lowercase) for _ in range(8)])
    return args


def prepare_args():
    parser = argparse.ArgumentParser(
        description="Sequently set task to worker for reissue certs for specified cids in metadb,\n" +
                    "should run it on metadb master host")
    parser.add_argument('-c', '--connection', type=str,
                        help='connection to metadb',
                        default="dbname=dbaas_metadb user=postgres")
    parser.add_argument('-t', '--task-suffix', type=str,
                        help='task suffix, may be ticket number, if empty use random')
    parser.add_argument('-d', '--db', type=str, required=True,
                        help='database type for update certificate, use one of: ' + ', '.join(CLUSTER_TYPE_MAP.keys()))
    parser.add_argument('cids', metavar='CID', type=str, nargs='+',
                        help='cid for update certificate')
    args = parser.parse_args()
    args = validate_args(args)
    return args


if __name__ == '__main__':
    if sys.version_info.major != 2:
        print("should run as python2 version for user root or postgres")
        exit(2)

    args = prepare_args()

    type_cluster = CLUSTER_TYPE_MAP[args.db]
    print("Is going to sequently update certificates: %s, with task suffix: %s, connection to meta db: '%s', number of cids for update: %d\n" %
          (type_cluster, args.task_suffix, args.connection, len(args.cids)))

    if pwd.getpwuid(os.getuid()).pw_name != 'postgres':
        os.seteuid(pwd.getpwnam('postgres').pw_uid)

    print("wait 16 seconds before run...")
    time.sleep(16)

    _reissue(args.connection, type_cluster, args.task_suffix, args.cids)
