"""
ClickHouse Remove ZooKeeper executor.
"""
from ..utils import ch_build_host_groups
from ...common.delete import BaseDeleteExecutor
from ...utils import register_executor, build_host_group_from_list


@register_executor('clickhouse_remove_zookeeper')
class ClickhouseRemoveZookeeperExecuter(BaseDeleteExecutor):
    """
    Remove ZooKeeper hosts. Should be used only to cleanup after enabling Keeper.
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))

        ch_host_group, _ = ch_build_host_groups(
            self.args['hosts'],
            self.config,
            ignored_hosts=self.args.get('ignore_hosts', []),
        )

        delete_host_group = build_host_group_from_list(self.config.zookeeper, self.args.get('delete_hosts'))
        self._delete_host_group_full(delete_host_group)

        self._run_operation_host_group(
            ch_host_group, 'service', pillar={'service-restart': True}, order=ch_host_group.hosts.keys()
        )

        self.mlock.unlock_cluster()
