"""
Elasticsearch Start cluster executor
"""
from ...common.cluster.start import ClusterStartExecutor
from ...utils import register_executor
from ..utils import host_groups


@register_executor('elasticsearch_cluster_start')
class ElasticsearchClusterStart(ClusterStartExecutor):
    """
    Start elasticsearch cluster in compute
    """

    def run(self):
        self.acquire_lock()

        master_group, data_group = host_groups(
            self.args['hosts'], self.config.elasticsearch_master, self.config.elasticsearch_data
        )

        if master_group.hosts:
            self._start_host_group(master_group)
        self._start_host_group(data_group)

        self.release_lock()
