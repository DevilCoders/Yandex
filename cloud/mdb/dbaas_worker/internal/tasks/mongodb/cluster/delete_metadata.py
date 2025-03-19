"""
MongoDB Delete Metadata cluster executor
"""

from ....providers.zookeeper import Zookeeper
from ...common.cluster.delete_metadata import ClusterDeleteMetadataExecutor
from ...utils import build_host_group, register_executor, get_managed_hostname
from ..utils import classify_host_map


@register_executor('mongodb_cluster_delete_metadata')
class MongoDBClusterDeleteMetadata(ClusterDeleteMetadataExecutor):
    """
    Delete mongodb cluster metadata
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.zookeeper = Zookeeper(config, task, queue)

    def run(self):
        hosts = classify_host_map(self.args['hosts'])
        host_groups = {
            host_type: build_host_group(getattr(self.config, host_type), hosts[host_type]) for host_type in hosts
        }

        for host_type, group in host_groups.items():
            self._delete_host_group(group)
            subcid = None

            for opts in hosts[host_type].values():
                if 'subcid' in opts:
                    subcid = opts['subcid']
                    break
            if subcid is None:
                continue
            self.conductor.group_absent(subcid)
            for host, opts in group.hosts.items():
                managed_hostname = get_managed_hostname(host, opts)
                self.eds_api.host_unregister(managed_hostname)

        self.solomon.cluster_absent(self.task['cid'])
