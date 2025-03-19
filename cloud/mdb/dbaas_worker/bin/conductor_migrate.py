"""
Script for conductor group move (per cluster type)
"""

import logging
from queue import Queue

from cloud.mdb.dbaas_worker.internal.config import get_config, worker_args_parser
from cloud.mdb.dbaas_worker.internal.providers.base_metadb import BaseMetaDBProvider
from cloud.mdb.dbaas_worker.internal.providers.conductor import ConductorApi
from cloud.mdb.dbaas_worker.internal.query import execute
from cloud.mdb.dbaas_worker.internal.runners import get_host_opts
from cloud.mdb.dbaas_worker.internal.tasks.clickhouse.utils import classify_host_map as ch_classify_host_map
from cloud.mdb.dbaas_worker.internal.tasks.elasticsearch.utils import classify_host_map as es_classify_host_map
from cloud.mdb.dbaas_worker.internal.tasks.kafka.utils import classify_host_map as kafka_classify_host_map
from cloud.mdb.dbaas_worker.internal.tasks.mongodb.utils import classify_host_map as mongodb_classify_host_map
from cloud.mdb.dbaas_worker.internal.utils import get_first_value
from cloud.mdb.dbaas_worker.internal.utils import get_conductor_root_group


def clickhouse_group_resolve(config, hosts):
    """
    Return group/properties map for clickhouse cluster
    """
    ch_hosts, zk_hosts = ch_classify_host_map(hosts)

    first_ch_host = get_first_value(ch_hosts)
    ret = {first_ch_host['subcid']: {'props': config.clickhouse, 'opts': first_ch_host}}

    if zk_hosts:
        first_zk_host = get_first_value(zk_hosts)
        ret[first_zk_host['subcid']] = {'props': config.zookeeper, 'opts': first_zk_host}

    return ret


def elasticsearch_group_resolve(config, hosts):
    """
    Return group/properties map for elasticsearch cluster
    """
    master_hosts, data_hosts = es_classify_host_map(hosts)

    first_data_host = get_first_value(data_hosts)
    ret = {first_data_host['subcid']: {'props': config.elasticsearch_data, 'opts': first_data_host}}

    if master_hosts:
        first_master_host = get_first_value(master_hosts)
        ret[first_master_host['subcid']] = {'props': config.elasticsearch_master, 'opts': first_master_host}

    return ret


def kafka_group_resolve(config, hosts):
    """
    Return group/properties map for kafka cluster
    """
    kafka_hosts, zk_hosts = kafka_classify_host_map(hosts)

    first_kafka_host = get_first_value(kafka_hosts)
    ret = {first_kafka_host['subcid']: {'props': config.kafka, 'opts': first_kafka_host}}

    if zk_hosts:
        first_zk_host = get_first_value(zk_hosts)
        ret[first_zk_host['subcid']] = {'props': config.zookeeper, 'opts': first_zk_host}

    return ret


def mongodb_group_resolve(config, hosts):
    """
    Return group/properties map for mongodb cluster
    """
    chosts = mongodb_classify_host_map(hosts)

    ret = {}

    for host_type, typed_hosts in chosts.items():
        first_host = get_first_value(typed_hosts)
        ret[first_host['subcid']] = {'props': getattr(config, host_type), 'opts': first_host}

    return ret


GROUP_RESOLVE_MAP = {
    'clickhouse_cluster': clickhouse_group_resolve,
    'elasticsearch_cluster': elasticsearch_group_resolve,
    'kafka_cluster': kafka_group_resolve,
    'mongodb_cluster': mongodb_group_resolve,
}


class ConductorMigrator:
    """
    Conductor group migrator (one subcluster at a time)
    """

    def __init__(self, config, cluster_type):
        task = {
            'task_id': f'conductor-migrate-{cluster_type}',
            'timeout': 24 * 3600,
            'changes': [],
            'context': {},
            'feature_flags': [],
            'folder_id': 'nonexistent',
        }
        queue = Queue(maxsize=10**6)
        self.metadb = BaseMetaDBProvider(config, task, queue)
        self.conductor = ConductorApi(config, task, queue)
        self.config = config
        self.cluster_type = cluster_type

    def get_task_hosts(self, cluster_id):
        """
        Get hosts in task args format
        """
        ret = {}
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                res = execute(cur, 'generic_resolve', cid=cluster_id)
        for row in res:
            ret[row['fqdn']] = get_host_opts(row)
            ret[row['fqdn']]['cid'] = cluster_id

        return ret

    def get_clusters(self):
        """
        Get all visible clusters with cluster_type
        """
        with self.metadb.get_master_conn() as conn:
            with conn:
                cur = conn.cursor()
                cur.execute(
                    'SELECT c.cid FROM dbaas.clusters c WHERE type = %(type)s AND code.visible(c)',
                    {'type': self.cluster_type},
                )
                return [x['cid'] for x in cur.fetchall()]

    def default_resolve(self, config, hosts):
        """
        Resolver for clusters with single group by cid
        """
        properties = getattr(config, self.cluster_type.split('_')[0])
        first_host = get_first_value(hosts)
        return {first_host['cid']: {'props': properties, 'opts': first_host}}

    def migrate(self):
        """
        Migrate conductor groups into new if required
        """
        clusters = self.get_clusters()
        if self.cluster_type in GROUP_RESOLVE_MAP:
            resolver = GROUP_RESOLVE_MAP[self.cluster_type]
        else:
            resolver = self.default_resolve

        for cid in clusters:
            hosts = self.get_task_hosts(cid)
            group_map = resolver(self.config, hosts)
            for group, data in group_map.items():
                self.conductor.group_has_root(group, get_conductor_root_group(data['props'], data['opts']))


def conductor_migrate():
    """
    Console entry-point
    """
    parser = worker_args_parser()
    parser.add_argument('ctype', type=str, help='target cluster type')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s:\t%(message)s')
    config = get_config(args.config)
    migrator = ConductorMigrator(config, args.ctype)

    migrator.migrate()
