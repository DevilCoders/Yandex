"""
Apache Kafka Start cluster executor
"""
from ...common.cluster.start import ClusterStartExecutor
from ...utils import register_executor
from ..utils import build_kafka_host_groups, make_cluster_dns_record


@register_executor('kafka_cluster_start')
class KafkaClusterStart(ClusterStartExecutor):
    """
    Start kafka cluster in compute
    """

    def run(self):
        self.acquire_lock()
        kafka_group, zk_group = build_kafka_host_groups(self.args['hosts'], self.config.kafka, self.config.zookeeper)
        if zk_group.hosts:
            self._start_host_group(zk_group)
        self._start_host_group(kafka_group)
        make_cluster_dns_record(self, kafka_group)
        self.release_lock()
