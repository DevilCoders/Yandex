"""
ElasticSearch Upgrade cluster executor
"""

from ...utils import register_executor
from ..utils import host_groups, ElasticsearchExecutorTrait
from .modify import ElasticsearchClusterModify


@register_executor('elasticsearch_cluster_upgrade')
class ElasticsearchClusterUpgrade(ElasticsearchClusterModify, ElasticsearchExecutorTrait):
    """
    Upgrade ElasticSearch cluster
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))

        master_group, data_group = host_groups(
            self.args['hosts'], self.config.elasticsearch_master, self.config.elasticsearch_data
        )
        self._es_upgrade_to_major_version(master_group, data_group)

        self.mlock.unlock_cluster()
