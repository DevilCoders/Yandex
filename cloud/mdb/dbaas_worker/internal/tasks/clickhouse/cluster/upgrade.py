"""
ClickHouse Upgrade cluster executor
"""
from ....providers.clickhouse.cloud_storage import CloudStorage
from ...utils import build_host_group, register_executor, split_hosts_for_modify_by_shard_batches
from ..utils import ch_build_host_groups, update_zero_copy_required
from .modify import ClickHouseClusterModify


@register_executor('clickhouse_cluster_upgrade')
class ClickHouseClusterUpgrade(ClickHouseClusterModify):
    """
    Upgrade ClickHouse cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.cloud_storage = CloudStorage(self.config, self.task, self.queue, self.args)

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))

        ch_group, zk_group = ch_build_host_groups(
            self.args['hosts'],
            self.config,
            ignored_hosts=self.args.get('ignore_hosts', []),
        )

        self._health_host_group(ch_group)
        self._health_host_group(zk_group)

        self._enable_cloud_storage(ch_group)
        self._update_geobase(ch_group)
        self._upgrade(ch_group, zk_group)

        self.mlock.unlock_cluster()

    def _upgrade(self, ch_group, zk_group):
        if not update_zero_copy_required(self.args, ch_group, zk_group, self.cloud_storage):
            for opts in ch_group.hosts.values():
                opts['deploy'] = {'pillar': {'service-restart': True}}
        else:
            for opts in ch_group.hosts.values():
                opts['deploy'] = {'pillar': {'service-restart': True, 'convert_zero_copy_schema': 'convert'}}

            for i_hosts in split_hosts_for_modify_by_shard_batches(
                ch_group.hosts, fast_mode=self.args.get('fast_mode', False)
            ):
                i_group = build_host_group(self.config.clickhouse, i_hosts)
                self._highstate_host_group(i_group, title='zero-copy-compatibility')
                self._health_host_group(i_group, '-post-convert')

            for opts in ch_group.hosts.values():
                opts['deploy'] = {'pillar': {'service-restart': True, 'convert_zero_copy_schema': 'cleanup'}}

        for i_hosts in split_hosts_for_modify_by_shard_batches(
            ch_group.hosts, fast_mode=self.args.get('fast_mode', False)
        ):
            i_group = build_host_group(self.config.clickhouse, i_hosts)

            self._highstate_host_group(i_group)

            self._health_host_group(i_group, '-post-upgrade')
