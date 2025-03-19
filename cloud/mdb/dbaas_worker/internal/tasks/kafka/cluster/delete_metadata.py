"""
Kafka Delete Metadata cluster executor
"""

from ....utils import get_first_value
from ...common.cluster.delete_metadata import ClusterDeleteMetadataExecutor
from ...utils import register_executor, get_managed_hostname
from ..utils import build_kafka_host_groups


@register_executor('kafka_cluster_delete_metadata')
class KafkaClusterDeleteMetadata(ClusterDeleteMetadataExecutor):
    """
    Delete Kafka cluster metadata
    """

    def run(self):
        self._delete_service_account()
        groups = build_kafka_host_groups(self.args['hosts'], self.config.kafka, self.config.zookeeper)
        groups = [group for group in groups if group.hosts]
        subcids = set()
        for group in groups:
            if group.hosts:
                self._delete_host_group(group)
                for host, opts in group.hosts.items():
                    managed_hostname = get_managed_hostname(host, opts)
                    self.eds_api.host_unregister(managed_hostname)
                subcids.add(get_first_value(group.hosts)['subcid'])
                self._delete_encryption_stuff(group)
                self._delete_cluster_service_account(group)
        for subcid in subcids:
            self.conductor.group_absent(subcid)
        self.solomon.cluster_absent(self.task['cid'])
