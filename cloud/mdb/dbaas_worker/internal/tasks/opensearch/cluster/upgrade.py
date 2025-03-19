"""
OpenSearch Upgrade cluster executor
"""

from ...utils import register_executor
from ..utils import host_groups, OpensearchExecutorTrait
from .modify import OpensearchClusterModify


@register_executor('opensearch_cluster_upgrade')
class OpensearchClusterUpgrade(OpensearchClusterModify, OpensearchExecutorTrait):
    """
    Upgrade OpenSearch cluster
    """

    def run(self):
        self.mlock.lock_cluster(sorted(self.args['hosts']))

        master_group, data_group = host_groups(
            self.args['hosts'], self.config.opensearch_master, self.config.opensearch_data
        )
        self._os_upgrade_to_major_version(master_group, data_group)

        self.mlock.unlock_cluster()
