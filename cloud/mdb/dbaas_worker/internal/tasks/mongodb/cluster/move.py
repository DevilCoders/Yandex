"""
MongoDB Move cluster executor
"""
from ...common.cluster.move import ClusterMoveExecutor
from ...utils import register_executor


@register_executor('mongodb_cluster_move')
class MongoDBClusterMove(ClusterMoveExecutor):
    """
    Move mongodb cluster
    """
