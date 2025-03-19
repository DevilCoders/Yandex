# -*- coding: utf-8 -*-
"""
DBaaS Internal API Clickhouse cluster start
"""
from datetime import timedelta

from ...core.types import Operation
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.metadata import StartClusterMetadata
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo
from .constants import MY_CLUSTER_TYPE
from .traits import ClickhouseOperations, ClickhouseTasks
from .utils import create_operation


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.START)
def clickhouse_start_cluster(cluster: ClusterInfo) -> Operation:
    """
    Start cluster
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_START_CLUSTER_API')

    return create_operation(
        task_type=ClickhouseTasks.start,
        operation_type=ClickhouseOperations.start,
        metadata=StartClusterMetadata(),
        cid=cluster.cid,
        time_limit=timedelta(hours=3),
    )
