"""
Script for resource preset change (e.g. deprecation)
"""

import json
import logging
from copy import deepcopy
from queue import Queue
from traceback import format_exc

from cloud.mdb.dbaas_worker.internal.config import get_config, worker_args_parser
from cloud.mdb.dbaas_worker.internal.providers.base_metadb import BaseMetaDBProvider
from cloud.mdb.dbaas_worker.internal.providers.compute import ComputeApi
from cloud.mdb.dbaas_worker.internal.query import execute
from cloud.mdb.dbaas_worker.internal.runners import get_host_opts
from cloud.mdb.dbaas_worker.internal.tasks import get_executor
from cloud.mdb.dbaas_worker.internal.tools.resetup import START_TASK_MAP, STOP_TASK_MAP

TASK_MAP = {
    'postgresql_cluster': 'postgresql_cluster_modify',
    'clickhouse_cluster': 'clickhouse_cluster_modify',
    'mongodb_cluster': 'mongodb_cluster_modify',
    'mysql_cluster': 'mysql_cluster_modify',
    'redis_cluster': 'redis_cluster_modify',
    'hadoop_cluster': 'hadoop_cluster_modify',
    'elasticsearch_cluster': 'elasticsearch_cluster_modify',
    'kafka_cluster': 'kafka_cluster_modify',
    'sqlserver_cluster': 'sqlserver_cluster_modify',
}


class ResourcePresetChanger:
    """
    Provider for force resource preset change
    """

    def __init__(self, config, task, queue):
        self.metadb = BaseMetaDBProvider(config, task, queue)
        self.compute = ComputeApi(config, task, queue)
        self.config = config
        self.task = task
        self.queue = queue

    def get_task_args(self, cluster_id):
        """
        Get common task args
        """
        args = {'hosts': {}, 'cid': cluster_id}
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                res = execute(cur, 'generic_resolve', cid=cluster_id)
        for row in res:
            args['hosts'][row['fqdn']] = get_host_opts(row)

        return args

    def update_preset(self, cluster_id, hosts, target_preset):
        """
        Modify preset on hosts and update quota usage
        """
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                cur.execute('SELECT id FROM dbaas.flavors WHERE name = %(name)s', {'name': target_preset})
                target_id = cur.fetchone()['id']
                cur.execute(
                    """
                    SELECT cl.cloud_ext_id
                    FROM dbaas.clouds cl
                    JOIN dbaas.folders f ON (f.cloud_id = cl.cloud_id)
                    JOIN dbaas.clusters c ON (c.folder_id = f.folder_id)
                    WHERE c.cid = %(cluster_id)s
                    """,
                    {'cluster_id': cluster_id},
                )
                cloud_id = cur.fetchone()['cloud_ext_id']
                rev = self.metadb.lock_cluster(conn, cluster_id)
                cur.execute(
                    'UPDATE dbaas.hosts SET flavor = %(flavor)s WHERE fqdn IN %(hosts)s',
                    {'flavor': target_id, 'hosts': tuple(hosts)},
                )
                self.metadb.complete_cluster_change(conn, cluster_id, rev)
                cur.execute('SELECT code.fix_cloud_usage(%(cloud_id)s)', {'cloud_id': cloud_id})

    def get_cluster_status(self, cluster_id):
        """
        Get current cluster status
        """
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                cur.execute('SELECT status FROM dbaas.clusters WHERE cid = %(cid)s', {'cid': cluster_id})
                return cur.fetchone()['status']

    def get_cluster_type(self, cluster_id):
        """
        Get cluster type
        """
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                cur.execute('SELECT type FROM dbaas.clusters WHERE cid = %(cid)s', {'cid': cluster_id})
                return cur.fetchone()['type']

    def run_task(self, task_type, args):
        """
        Run task with fake executor
        """
        task = deepcopy(self.task)
        task['task_type'] = task_type
        task['cid'] = args['cid']
        executor = get_executor(task, self.config, self.queue, args)
        try:
            executor.run()
        except Exception as exc:
            executor.rollback(exc, format_exc(), initially_failed=True)
            raise

    def prepare_cluster(self, cluster_type, cluster_id, vtype):
        """
        Prepare cluster for resource preset change (e.g. disable billing)
        """
        if vtype != 'compute' or self.get_cluster_status(cluster_id) != 'STOPPED':
            return

        args = self.get_task_args(cluster_id)
        if cluster_type != 'hadoop_cluster':
            disable_operations = []
            for host in args['hosts']:
                operation_id = self.compute.disable_billing(host)
                disable_operations.append(operation_id)

            for i in disable_operations:
                self.compute.operation_wait(i)

        start_args = deepcopy(args)
        self.run_task(START_TASK_MAP[cluster_type], start_args)

    def modify_cluster(self, cluster_type, cluster_id, pre_modify_task, modify_args):
        """
        Run modify cluster task
        """
        args = self.get_task_args(cluster_id)
        args.update(modify_args)
        if pre_modify_task:
            self.run_task(pre_modify_task, deepcopy(args))
        self.run_task(TASK_MAP[cluster_type], args)

    def post_modify_cluster(self, cluster_type, cluster_id, vtype):
        """
        Run specific post-modify changes (e.g. enable billing back)
        """
        if vtype != 'compute' or self.get_cluster_status(cluster_id) != 'STOPPED':
            return

        args = self.get_task_args(cluster_id)
        stop_args = deepcopy(args)
        self.run_task(STOP_TASK_MAP[cluster_type], stop_args)

        if cluster_type != 'hadoop_cluster':
            enable_operations = []
            for host in args['hosts']:
                operation_id = self.compute.enable_billing(host)
                if operation_id:
                    enable_operations.append(operation_id)

            for i in enable_operations:
                self.compute.operation_wait(i)

    def change_preset(self, cluster_id, source_preset, target_preset, pre_modify_task, modify_args):
        """
        Change resource preset for cluster (only for hosts matching source preset)
        """
        cluster_type = self.get_cluster_type(cluster_id)
        if cluster_type not in TASK_MAP:
            raise RuntimeError(f'Unsupported cluster_type: {cluster_type}')
        original_cluster = self.get_task_args(cluster_id)
        should_change = []
        vtype = None
        for host, opts in original_cluster['hosts'].items():
            # Right now mixed vtypes do not exist
            if vtype is None:
                vtype = opts['vtype']
            if opts['flavor'] == source_preset:
                should_change.append(host)
        if not should_change:
            raise RuntimeError(f'Unable to find hosts with resource preset id {source_preset} in {cluster_id}')

        self.update_preset(cluster_id, should_change, target_preset)

        self.prepare_cluster(cluster_type, cluster_id, vtype)
        self.modify_cluster(cluster_type, cluster_id, pre_modify_task, modify_args)
        self.post_modify_cluster(cluster_type, cluster_id, vtype)


def force_resource_preset_change():
    """
    Console entry-point
    """
    parser = worker_args_parser(
        description='\n'.join(
            [
                'Examples:',
                './force-resource-preset-change -s s1.micro -t s1.small e4u1pnvur0cc31qeduhk',
                'for hadoop: ./force-resource-preset-change -s s2.medium -t s2.small --args=\'{"image_id": "fdvj964gkvfbpd35bk4q"}\' -f aoescshp3an5oac74m7v e4uk6rkhqm1p5ikiocuc',
            ]
        )
    )
    parser.add_argument('-s', '--source', type=str, help='Source preset id', required=True)
    parser.add_argument('-t', '--target', type=str, help='Target preset id', required=True)
    parser.add_argument('-f', '--folder_id', type=str, help='Task folder ext id', default='test')
    parser.add_argument('-p', '--pre', type=str, help='Run pre-modify task')
    parser.add_argument('-a', '--args', type=str, help='Additional modify task args', default='{}')
    parser.add_argument('cid', type=str, help='target cluster id')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')
    config = get_config(args.config)
    changer = ResourcePresetChanger(
        config,
        {
            'task_id': f'change-resource-preset-{args.cid}',
            'timeout': 24 * 3600,
            'changes': [],
            'context': {},
            'feature_flags': [],
            'folder_id': args.folder_id,
        },
        Queue(maxsize=10**6),
    )

    args_dict = json.loads(args.args)

    changer.change_preset(args.cid, args.source, args.target, args.pre, args_dict)
