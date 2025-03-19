"""
Apache Kafka Delete cluster executor
"""
from ...common.cluster.delete import ClusterDeleteExecutor
from ..utils import build_kafka_host_groups, delete_cluster_dns_record
from ...utils import register_executor


@register_executor('kafka_cluster_delete')
class KafkaClusterDelete(ClusterDeleteExecutor):
    """
    Delete kafka cluster
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.properties = config.kafka

    def run(self):
        kafka_group, _ = build_kafka_host_groups(self.args['hosts'], self.config.kafka, self.config.zookeeper)
        delete_cluster_dns_record(self, kafka_group)
        super().run()
