# -*- coding: utf-8 -*-
"""
DBaaS Internal API MongoDB cluster stop
"""

from ...core.types import Operation
from ...utils.metadata import StopClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo
from .constants import MY_CLUSTER_TYPE
from .traits import MongoDBOperations, MongoDBTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.STOP)
def mongodb_stop_cluster(cluster: ClusterInfo, **_) -> Operation:
    """
    Stop cluster
    """
    return create_operation(
        task_type=MongoDBTasks.stop,
        operation_type=MongoDBOperations.stop,
        metadata=StopClusterMetadata(),
        cid=cluster.cid,
    )
