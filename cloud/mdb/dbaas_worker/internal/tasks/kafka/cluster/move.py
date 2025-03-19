"""
Apache Kafka Move cluster executor
"""
from ...common.cluster.move import ClusterMoveExecutor
from ...utils import register_executor


@register_executor('kafka_cluster_move')
class KafkaClusterMove(ClusterMoveExecutor):
    """
    Move kafka cluster
    """
