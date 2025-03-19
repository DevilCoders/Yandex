"""
Rebalance slot distribution in Redis Cluster.
"""

from datetime import timedelta

from ...core.exceptions import PreconditionFailedError
from ...core.types import Operation
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.metadata import RebalanceClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .pillar import RedisPillar
from .traits import RedisOperations, RedisTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.REBALANCE)
def rebalance_handler(cluster: dict, **_) -> Operation:
    """
    Redis rebalance slot distribution handler.
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_CLUSTER_OPERATE_API')
    cluster_pillar = RedisPillar(cluster['value'])
    if not cluster_pillar.is_cluster_enabled():
        raise PreconditionFailedError('Sharding must be enabled')

    return create_operation(
        task_type=RedisTasks.rebalance,
        operation_type=RedisOperations.rebalance,
        metadata=RebalanceClusterMetadata(),
        time_limit=timedelta(hours=24),
        cid=cluster['cid'],
        task_args={},
    )
