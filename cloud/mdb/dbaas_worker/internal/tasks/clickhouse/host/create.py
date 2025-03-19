"""
ClickHouse host create executor.
"""
from copy import deepcopy

from ...common.host.create import HostCreateExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map, create_schema_backup, delete_ch_backup


@register_executor('clickhouse_host_create')
class ClickHouseHostCreate(HostCreateExecutor):
    """
    Create ClickHouse host in dbm or compute.
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = deepcopy(self.config.clickhouse)
        ch_hosts, _ = classify_host_map(self.args['hosts'], ignored_hosts=self.args.get('ignore_hosts', []))
        self.properties.conductor_group_id = next(iter(ch_hosts.values()))['subcid']

    def run(self):
        self.acquire_lock()

        host = self.args['host']
        target_shard = self.args['hosts'][host].get('shard_name')
        ch_hosts, _ = classify_host_map(self.args['hosts'], ignored_hosts=self.args.get('ignore_hosts', []))
        replica_host = None
        self.args['hosts'][host]['service_account_id'] = self.args.get('service_account_id')
        self.properties.create_host_pillar.update({'restart-check': False})

        backup_id = None
        if self.args.get('resetup_from_replica', False):
            for ch_host in ch_hosts:
                if ch_host != host and ch_hosts[ch_host].get('shard_name') == target_shard:
                    replica_host = ch_host
                    break
            backup_shard = ch_hosts[replica_host].get('shard_name')
            backup_id = create_schema_backup(self.deploy_api, replica_host)
            self.properties.create_host_pillar.update(
                {
                    'replica_schema_backup': backup_id,
                    'schema_backup_shard': backup_shard,
                }
            )

        self._create_host()

        if self.args.get('resetup_from_replica', False):
            delete_ch_backup(self.deploy_api, replica_host, backup_id)

        self.release_lock()


@register_executor('clickhouse_zookeeper_host_create')
class ClickHouseZookeeperHostCreate(HostCreateExecutor):
    """
    Create Zookeeper host in dbm or compute.
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = deepcopy(self.config.zookeeper)
        _, zk_hosts = classify_host_map(self.args['hosts'], ignored_hosts=self.args.get('ignore_hosts', []))
        self.properties.conductor_group_id = next(iter(zk_hosts.values()))['subcid']

    def run(self):
        self.acquire_lock()

        self._create_host()

        _, zk_hosts = classify_host_map(self.args['hosts'], ignored_hosts=self.args.get('ignore_hosts', []))
        old_zk_hosts = {host: opts for host, opts in zk_hosts.items() if host != self.args['host']}

        zk_group = build_host_group(self.config.zookeeper, zk_hosts)
        old_zk_group = build_host_group(self.config.zookeeper, old_zk_hosts)

        # validate that the new node has joined the cluster
        self._health_host_group(zk_group)

        self._run_operation_host_group(
            old_zk_group,
            'cluster-reconfig',
            title='zookeeper-reconfig-join',
            pillar={
                'zookeeper-join': {
                    'fqdn': self.args['host'],
                    'zid': self.args['zid_added'],
                },
            },
        )

        self.release_lock()
