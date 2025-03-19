"""
Apache Kafka Stop cluster executor
"""
from ...common.cluster.stop import ClusterStopExecutor
from ...utils import register_executor
from ..utils import build_kafka_host_groups, delete_cluster_dns_record


@register_executor('kafka_cluster_stop')
class KafkaClusterStop(ClusterStopExecutor):
    """
    Stop kafka cluster in compute
    """

    def run(self):
        self.acquire_lock()
        kafka_group, zk_group = build_kafka_host_groups(self.args['hosts'], self.config.kafka, self.config.zookeeper)
        delete_cluster_dns_record(self, kafka_group)
        self._stop_host_group(kafka_group)
        if zk_group.hosts:
            self._stop_host_group(zk_group)
        self.release_lock()
