"""
ClickHouse Add ZooKeeper executor.
"""
from ...common.create import BaseCreateExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map


@register_executor('clickhouse_add_zookeeper')
class ClickhouseAddZookeeperExecuter(BaseCreateExecutor):
    """
    Add ZooKeeper.
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        ch_hosts, zk_hosts = classify_host_map(self.args['hosts'], ignored_hosts=self.args.get('ignore_hosts', []))

        ch_group = build_host_group(self.config.clickhouse, ch_hosts)
        for opts in ch_group.hosts.values():
            opts['deploy'] = {'pillar': {'service-restart': True}}

        zk_group = build_host_group(self.config.zookeeper, zk_hosts)
        zk_subcid = next(iter(zk_hosts.values()))['subcid']
        zk_group.properties.conductor_group_id = zk_subcid

        self._create_conductor_group(zk_group)
        self._create_host_secrets(zk_group)
        self._create_host_group(zk_group)

        self._issue_tls(zk_group)

        self._highstate_host_group(zk_group)
        self._highstate_host_group(ch_group)
        self._create_public_records(zk_group)
        self._enable_monitoring(zk_group)
        self.mlock.unlock_cluster()
