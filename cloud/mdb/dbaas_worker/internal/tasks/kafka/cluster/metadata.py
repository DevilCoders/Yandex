"""
Kafka metadata state executor
"""
from ...common.cluster.metadata import ClusterMetadataExecutor
from ...utils import register_executor


@register_executor('kafka_metadata_update')
class KafkaClusterMetadata(ClusterMetadataExecutor):
    """
    Execute metadata on Kafka cluster
    """
