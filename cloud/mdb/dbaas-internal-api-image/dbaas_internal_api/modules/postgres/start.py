# -*- coding: utf-8 -*-
"""
DBaaS Internal API PostgreSQL cluster start
"""

from ...core.types import Operation
from ...utils.metadata import StartClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo
from .constants import MY_CLUSTER_TYPE
from .traits import PostgresqlOperations, PostgresqlTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.START)
def postgresql_start_cluster(cluster: ClusterInfo) -> Operation:
    """
    Start cluster
    """
    return create_operation(
        task_type=PostgresqlTasks.start,
        operation_type=PostgresqlOperations.start,
        metadata=StartClusterMetadata(),
        cid=cluster.cid,
    )
