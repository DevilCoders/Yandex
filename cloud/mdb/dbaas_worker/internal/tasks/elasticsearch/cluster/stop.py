"""
Elasticsearch Stop cluster executor
"""
from ...common.cluster.stop import ClusterStopExecutor
from ...utils import register_executor
from ..utils import host_groups


@register_executor('elasticsearch_cluster_stop')
class ElasticsearchClusterStop(ClusterStopExecutor):
    """
    Stop elasticsearch cluster in compute
    """

    def run(self):
        self.acquire_lock()
        master_group, data_group = host_groups(
            self.args['hosts'], self.config.elasticsearch_master, self.config.elasticsearch_data
        )

        self._stop_host_group(data_group)
        if master_group.hosts:
            self._stop_host_group(master_group)

        self.release_lock()
