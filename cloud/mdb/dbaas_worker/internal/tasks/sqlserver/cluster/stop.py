"""
SQLServer Stop cluster executor
"""
from ...common.cluster.stop import ClusterStopExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map


@register_executor('sqlserver_cluster_stop')
class SQLServerClusterStop(ClusterStopExecutor):
    """
    Stop sqlserver cluster in compute
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))
        de_hosts, witness_hosts = classify_host_map(self.args['hosts'])
        de_host_group = build_host_group(self.config.sqlserver, de_hosts)
        if not de_host_group.properties:
            raise RuntimeError('Configuration error: has de_hosts but not de_host_group.properties')

        witness_host_group = build_host_group(self.config.windows_witness, witness_hosts)
        if witness_hosts:
            if not witness_host_group.properties:
                raise RuntimeError('Configuration error: has witness_hosts but not witness_host_group.properties')
            witness_host_group.properties.conductor_group_id = next(iter(witness_hosts.values()))['subcid']

        if witness_host_group.hosts:
            self._stop_host_group(witness_host_group)
        self._stop_host_group(de_host_group)
        self.mlock.unlock_cluster()
