"""
ClickHouse host delete executor.
"""

from ...common.host.delete import HostDeleteExecutor
from ...utils import (
    build_host_group,
    build_host_group_from_list,
    register_executor,
    split_hosts_for_modify_by_shard_batches,
)
from ..utils import classify_host_map


@register_executor('clickhouse_host_delete')
class ClickHouseHostDelete(HostDeleteExecutor):
    """
    Delete ClickHouse host in dbm or compute.

    It involves configuration update (invoking high-state) on all
    hosts except the deleting one. The update is performed before destroying
    the host.
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)

    def run(self):
        lock_hosts = list(self.args['hosts'])
        lock_hosts.append(self.args['host']['fqdn'])
        self.mlock.lock_cluster(sorted(lock_hosts))
        hosts = self.args['hosts']
        ch_hosts, zk_hosts = classify_host_map(hosts, ignored_hosts=self.args.get('ignore_hosts', []))

        zk_group = build_host_group(self.config.zookeeper, zk_hosts)
        ch_group = build_host_group(self.config.clickhouse, ch_hosts)

        self._run_operation_host_group(zk_group, 'metadata')
        self._run_operation_host_group(ch_group, 'metadata')

        delete_host_group = build_host_group_from_list(self.config.clickhouse, [self.args['host']])
        self._delete_host_group_full(delete_host_group)

        self._run_operation_host_group(
            zk_group,
            'cleanup',
            title='post-clickhouse-delete-cleanup',
            pillar={
                'clickhouse-hosts': {
                    'cid': self.task['cid'],
                    'removed-hosts': [
                        self.args['host']['fqdn'],
                    ],
                },
            },
            timeout=6 * 3600,
        )

        self.mlock.unlock_cluster()


@register_executor('clickhouse_zookeeper_host_delete')
class ClickHouseZookeeperHostDelete(HostDeleteExecutor):
    """
    Delete Zookeeper host in dbm or compute.
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)

    def run(self):
        lock_hosts = list(self.args['hosts'])
        lock_hosts.append(self.args['host']['fqdn'])
        self.mlock.lock_cluster(sorted(lock_hosts))
        ch_hosts, zk_hosts = classify_host_map(self.args['hosts'], ignored_hosts=self.args.get('ignore_hosts', []))

        zk_group = build_host_group(self.config.zookeeper, zk_hosts)
        ch_group = build_host_group(self.config.clickhouse, ch_hosts)
        for opts in ch_group.hosts.values():
            opts['deploy'] = {'pillar': {'include-metadata': True, 'service-restart': True}}

        self._health_host_group(zk_group)

        self._run_operation_host_group(
            zk_group,
            'cluster-reconfig',
            title='zookeeper-reconfig-leave',
            pillar={
                'zookeeper-leave': {
                    'fqdn': self.args['host']['fqdn'],
                    'zid': self.args['zid_deleted'],
                },
            },
        )

        self._run_operation_host_group(zk_group, 'metadata')

        for i_hosts in split_hosts_for_modify_by_shard_batches(
            ch_group.hosts, fast_mode=self.args.get('fast_mode', False)
        ):
            i_group = build_host_group(self.config.clickhouse, i_hosts)
            self._run_operation_host_group(i_group, 'service')

        delete_host_group = build_host_group_from_list(self.config.zookeeper, [self.args['host']])
        self._delete_host_group_full(delete_host_group)
        self.mlock.unlock_cluster()
