"""
ClickHouse shard delete executor.
"""

from ....providers.zookeeper import Zookeeper
from ...common.shard.delete import ShardDeleteExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map


@register_executor('clickhouse_shard_delete')
class ClickhouseShardDelete(ShardDeleteExecutor):
    """
    Delete ClickHouse shard.
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.zookeeper = Zookeeper(config, task, queue)
        self.properties = self.config.clickhouse
        self.zk_properties = self.config.zookeeper

    def run(self):
        lock_hosts = list(self.args['hosts'])
        lock_hosts.extend([host['fqdn'] for host in self.args['shard_hosts']])
        self.mlock.lock_cluster(sorted(lock_hosts))
        ch_hosts, zk_hosts = classify_host_map(self.args['hosts'], ignored_hosts=self.args.get('ignore_hosts', []))
        ch_host_group = build_host_group(self.properties, ch_hosts)
        zk_host_group = build_host_group(self.zk_properties, zk_hosts)

        self._health_host_group(zk_host_group)
        self._health_host_group(ch_host_group)

        self._run_operation_host_group(zk_host_group, 'metadata')
        self._run_operation_host_group(ch_host_group, 'metadata')

        self._delete_hosts()

        deleted_shard_hosts = self.args['shard_hosts']

        fqdns = []
        for host in deleted_shard_hosts:
            fqdns.append(host['fqdn'])

        self._run_operation_host_group(
            zk_host_group,
            'cleanup',
            title='post-clickhouse-delete-cleanup',
            pillar={
                'clickhouse-hosts': {
                    'cid': self.task['cid'],
                    'removed-hosts': fqdns,
                },
            },
            timeout=6 * 3600,
        )

        self.mlock.unlock_cluster()
