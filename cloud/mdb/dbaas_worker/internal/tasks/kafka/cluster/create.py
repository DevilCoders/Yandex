"""
Apache Kafka Create cluster executor
"""
from ...common.cluster.create import ClusterCreateExecutor
from ...utils import register_executor
from ..utils import build_kafka_host_groups, classify_host_map, make_cluster_dns_record

CREATE = 'kafka_cluster_create'


@register_executor(CREATE)
class KafkaClusterCreate(ClusterCreateExecutor):
    """
    Create kafka cluster in dbm and/or compute
    """

    def _before_highstate(self):
        """
        Actions that can be executed before highstate (e.g. update pillar)
        """
        kafka_hosts, _ = classify_host_map(self.args['hosts'])
        self._create_service_account(cluster_resource_name='managed-kafka.cluster', hosts=kafka_hosts)

    def run(self):
        self.acquire_lock()
        groups = build_kafka_host_groups(self.args['hosts'], self.config.kafka, self.config.zookeeper)
        groups = [group for group in groups if group.hosts]
        self._create(*groups)
        make_cluster_dns_record(self, groups[0])
        for group in groups:
            self._health_host_group(group)
        self.release_lock()
