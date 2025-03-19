# -*- coding: utf-8 -*-
"""
DBaaS Internal API Redis cluster start
"""

from ...core.types import Operation
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.metadata import StartClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo
from .constants import MY_CLUSTER_TYPE
from .traits import RedisOperations, RedisTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.START)
def redis_start_cluster(cluster: ClusterInfo) -> Operation:
    """
    Start cluster
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_CLUSTER_OPERATE_API')
    return create_operation(
        task_type=RedisTasks.start,
        operation_type=RedisOperations.start,
        metadata=StartClusterMetadata(),
        cid=cluster.cid,
    )
