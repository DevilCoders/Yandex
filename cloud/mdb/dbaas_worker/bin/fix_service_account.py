"""
Simple script for fixing service accounts on ClickHouse hosts
"""

import logging
import grpc
import json
from queue import Queue
from typing import Any, Dict, Tuple

from cloud.mdb.dbaas_worker.internal.config import get_config, worker_args_parser
from cloud.mdb.dbaas_worker.internal.providers.deploy import DeployAPI
from cloud.mdb.dbaas_worker.internal.providers.base_metadb import BaseMetaDBProvider
from cloud.mdb.dbaas_worker.internal.providers.compute import ComputeApi
from cloud.mdb.dbaas_worker.internal.providers.mlock import Mlock
from cloud.mdb.dbaas_worker.internal.query import execute
from cloud.mdb.dbaas_worker.internal.providers.iam import Iam


def fix_service_account():
    """
    Console entry-point
    """
    parser = worker_args_parser()
    parser.add_argument('--dry-run', action='store_true', help='Only check, do not actually modify cluster')
    parser.add_argument('--timeout', type=int, default=3600, help='deploy timeout')
    parser.add_argument('cid', type=str, help='cid of cluster to fix')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')

    task = {
        'cid': args.cid,
        'task_id': 'sa-fix',
        'changes': [],
        'context': {},
        'feature_flags': [],
        'folder_id': 'test',
        'timeout': args.timeout,
    }
    config = get_config(args.config)
    compute_api = ComputeApi(config, task, Queue())
    deploy = DeployAPI(config, task, Queue())
    mlock = Mlock(config, task, Queue())
    metadb = BaseMetaDBProvider(config, task, Queue())
    iam = Iam(config, task, Queue())
    iam.reconnect()

    subcid, sa_id, hosts = _get_cluster_info(metadb, args.cid)

    if not sa_id:
        logging.info("Service account not used, nothing to do.")
        return

    try:
        mlock.lock_cluster(hosts=list(hosts.keys()))
        try:
            iam.get_service_account(sa_id)
        except grpc.RpcError as e:
            if e.code() == grpc.StatusCode.NOT_FOUND:
                logging.info('Service account not found, going to remove service account id from pillar')
                if not args.dry_run:
                    _delete_service_account_from_pillar(metadb, args.cid, subcid)
                    _run_service_operation(deploy, hosts, args.timeout)
            else:
                raise RuntimeError('Failed to get service account from iam') from e

        operations = []
        for host in hosts.keys():
            if compute_api.get_instance(host).service_account_id != sa_id:
                logging.info(f'Going to modify {host} metadata to link service account')
                if not args.dry_run:
                    operations.append(compute_api.update_instance_attributes(host, service_account_id=sa_id))
            else:
                logging.info(f'Nothing to do with {host}')

        compute_api.operations_wait(operations)
        if not args.dry_run:
            _run_service_operation(deploy, hosts, args.timeout)
    finally:
        mlock.unlock_cluster()


def _get_cluster_info(metadb: BaseMetaDBProvider, cid: str) -> Tuple[str, str, Dict[str, Any]]:
    with metadb.get_master_conn() as conn:
        with conn:
            cur = conn.cursor()
            mogrified = cur.mogrify(
                """
            SELECT * FROM code.get_hosts_by_cid(i_cid => %(cid)s) WHERE 'clickhouse_cluster'=ANY(roles::text[]);
            """,
                dict(cid=cid),
            )
            cur.execute(mogrified)
            hosts = list(cur.fetchall())
            subcid = next(iter(hosts))['subcid']
            hosts = {row['fqdn']: row for row in hosts}

            res = execute(
                cur, 'get_pillar', subcid=subcid, path='{data,service_account_id}', cid=None, shard_id=None, fqdn=None
            )
            if len(res) > 0:
                return subcid, res[0]['value'], hosts
            raise RuntimeError('Failed to get sa from subcluster pillar')


def _delete_service_account_from_pillar(metadb: BaseMetaDBProvider, cid: str, subcid: str):
    with metadb.get_master_conn() as conn:
        with conn:
            cur = conn.cursor()
            rev = metadb.lock_cluster(conn, cid)
            res = execute(cur, 'get_pillar', subcid=subcid, path='{}', cid=None, shard_id=None, fqdn=None)
            if len(res) == 0:
                raise RuntimeError('Failed to get pillar from subcluster pillar')
            pillar = res[0]['value']
            del pillar['data']['service_account_id']
            mogrified = cur.mogrify(
                """
             SELECT code.update_pillar(
                 i_cid   => %(cluster_id)s,
                 i_rev   => %(cluster_rev)s,
                 i_key   => code.make_pillar_key(
                     i_subcid   => %(subcid)s),
                 i_value => %(value)s);
             """,
                dict(cluster_id=cid, cluster_rev=rev, subcid=subcid, value=json.dumps(pillar)),
            )
            cur.execute(mogrified)
            metadb.complete_cluster_change(conn, cid, rev)


def _run_service_operation(deploy: DeployAPI, hosts: Dict[str, Any], timeout: int):
    deploy.wait(
        [
            deploy.run(
                host,
                deploy_version=deploy.get_deploy_version_from_minion(host),
                deploy_title='service',
                method={
                    'commands': [
                        {
                            'type': 'state.sls',
                            'arguments': [
                                'components.dbaas-operations.service',
                                f'saltenv={opts["environment"]}',
                                f'timeout={timeout}',
                            ],
                            'timeout': timeout,
                        }
                    ],
                    'fqdns': [host],
                    'parallel': 1,
                    'stopOnErrorCount': 1,
                    'timeout': timeout,
                },
            )
            for host, opts in hosts.items()
        ]
    )
