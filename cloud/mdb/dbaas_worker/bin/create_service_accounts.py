"""
Simple script for update security groups
"""

import logging
from queue import Queue

from cloud.mdb.dbaas_worker.internal.config import get_config, worker_args_parser
from cloud.mdb.dbaas_worker.internal.providers.base_metadb import BaseMetaDBProvider
from cloud.mdb.dbaas_worker.internal.providers.mlock import Mlock
from cloud.mdb.dbaas_worker.internal.query import execute
from cloud.mdb.dbaas_worker.internal.runners import get_host_opts
from cloud.mdb.dbaas_worker.internal.tasks.common.create import BaseCreateExecutor
from cloud.mdb.dbaas_worker.internal.tasks.kafka.utils import classify_host_map


def create_service_accounts():
    """
    Create service account for clusters that miss one. Currently only Kafka clusters are supported.
    """
    parser = worker_args_parser()
    parser.add_argument('-n', '--dry-run', action='store_true', help='Only emulate, do not real update')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')

    list_task = {
        'cid': 'list-cids',
        'task_id': 'list-cids',
        'changes': [],
        'context': {},
        'feature_flags': [],
        'folder_id': 'test',
    }
    config = get_config(args.config)
    metadb = BaseMetaDBProvider(config, list_task, Queue())

    clusters_to_process = _get_clusters_to_process(metadb, 'kafka_cluster')

    if not clusters_to_process:
        logging.info("There're no clusters without service account")
        return

    logging.info(f"Total number of clusters that don't have service account: {len(clusters_to_process)}")

    skipped_by_status = {}
    processed = []
    failed = []

    for cid, status in clusters_to_process:
        if status not in ('RUNNING', 'STOPPED'):
            skipped_by_status.setdefault(status, []).append(cid)
            continue

        if args.dry_run:
            continue

        logging.info(f"Creating service account for cluster {cid}")

        create_task = {
            'cid': cid,
            'task_id': f'create-service-accounts-for-kafka-cluster-{cid}',
            'changes': [],
            'context': {},
            'feature_flags': [],
            'folder_id': 'some',
        }
        mlock = Mlock(config, create_task, Queue())

        try:
            hosts = _get_task_hosts(metadb, cid)
            mlock.lock_cluster(hosts=list(hosts.keys()))

            executor = BaseCreateExecutor(config, create_task, Queue(), {})
            kafka_hosts, _ = classify_host_map(hosts)
            executor._create_service_account('managed-kafka.cluster', kafka_hosts)
            processed.append(cid)
        except Exception as ex:
            failed.append(cid)
            logging.error(f'Failed to create service account for cluster: {ex}')
        finally:
            mlock.unlock_cluster()

    if processed:
        logging.info(f"Service accounts were created for {len(processed)} clusters")

    if failed:
        logging.info(f"Some errors happened while creating service accounts for {len(failed)} clusters")

    if skipped_by_status:
        messages = [
            f"{len(cluster_ids)} clusters in status {status}" for status, cluster_ids in skipped_by_status.items()
        ]
        logging.info(f"Some clusters were skipped due to their status: {', '.join(messages)}")


def _get_task_hosts(metadb, cluster_id):
    """
    Get hosts in task args format
    """
    ret = {}
    with metadb.get_master_conn() as conn:
        with conn:
            cur = conn.cursor()
            res = execute(cur, 'generic_resolve', cid=cluster_id)
    for row in res:
        ret[row['fqdn']] = get_host_opts(row)
        ret[row['fqdn']]['cid'] = cluster_id

    return ret


def _get_clusters_to_process(metadb, role):
    """
    Get resources existing in metadb (only hosts for now)
    """

    with metadb.get_master_conn() as conn:
        with conn:
            cur = conn.cursor()
            cur.execute(
                """
                SELECT c.cid, c.status
                FROM dbaas.clusters c
                JOIN dbaas.subclusters s USING (cid)
                LEFT JOIN dbaas.pillar p USING (subcid)
                WHERE code.visible(c) AND %(role)s = ANY(s.roles::text[])
                AND (p.value IS NULL OR NOT p.value -> 'data' ? 'service_account')
                """,
                {
                    'role': role,
                },
            )
            return [(row['cid'], row['status']) for row in cur.fetchall()]
