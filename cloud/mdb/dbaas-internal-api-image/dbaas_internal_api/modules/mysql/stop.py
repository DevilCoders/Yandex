# -*- coding: utf-8 -*-
"""
DBaaS Internal API MySQL cluster stop
"""

from ...core.types import Operation
from ...utils.metadata import StopClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo
from .constants import MY_CLUSTER_TYPE
from .traits import MySQLOperations, MySQLTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.STOP)
def mysql_stop_cluster(cluster: ClusterInfo, **_) -> Operation:
    """
    Stop cluster
    """
    return create_operation(
        task_type=MySQLTasks.stop,
        operation_type=MySQLOperations.stop,
        metadata=StopClusterMetadata(),
        cid=cluster.cid,
    )
