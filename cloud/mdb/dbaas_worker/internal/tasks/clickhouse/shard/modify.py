"""
ClickHouse Modify shard executor.
"""

from ....utils import split_host_map, get_first_value
from ...common.shard.modify import ShardModifyExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map


@register_executor('clickhouse_shard_modify')
class ClickhouseShardModify(ShardModifyExecutor):
    """
    Modify ClickHouse shard.
    """

    def run(self):
        """
        Run modify on clickhouse shard
        """
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        ch_hosts, zk_hosts = classify_host_map(self.args['hosts'], ignored_hosts=self.args.get('ignore_hosts', []))

        shard_hosts, rest_hosts = split_host_map(ch_hosts, self.args['shard_id'])

        for host in shard_hosts:
            shard_hosts[host]['service_account_id'] = self.args.get('service_account_id')
            shard_hosts[host]['deploy'] = {
                'pillar': {
                    'update-geobase': self.args.get('update-geobase', False),
                },
            }

        shard_host_group = build_host_group(self.config.clickhouse, shard_hosts)
        shard_host_group.properties.conductor_group_id = get_first_value(shard_hosts)['subcid']
        rest_host_group = build_host_group(self.config.clickhouse, rest_hosts)
        zk_host_group = build_host_group(self.config.zookeeper, zk_hosts)

        self._modify_shard(shard_host_group, None, [zk_host_group, rest_host_group])
        self.mlock.unlock_cluster()
