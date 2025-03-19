"""
MongoDB Start cluster executor
"""
from ...common.cluster.start import ClusterStartExecutor
from ...utils import build_host_group, register_executor
from ..utils import classify_host_map


@register_executor('mongodb_cluster_start')
class MongodbClusterStart(ClusterStartExecutor):
    """
    Start mongodb cluster in compute
    """

    def run(self):
        self.acquire_lock()
        host_groups = [
            build_host_group(getattr(self.config, host_type), hosts)
            for host_type, hosts in classify_host_map(self.args['hosts']).items()
        ]
        for host_group in host_groups:
            subcid = next(iter(host_group.hosts.values()))['subcid']
            host_group.properties.conductor_group_id = subcid
            self._start_host_group(host_group)
        self.release_lock()
