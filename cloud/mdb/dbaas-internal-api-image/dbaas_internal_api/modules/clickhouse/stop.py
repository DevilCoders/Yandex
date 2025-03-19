# -*- coding: utf-8 -*-
"""
DBaaS Internal API Clickhouse cluster stop
"""
from ...core.types import Operation
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.metadata import StopClusterMetadata
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo
from .constants import MY_CLUSTER_TYPE
from .traits import ClickhouseOperations, ClickhouseTasks
from .utils import create_operation


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.STOP)
def clickhouse_stop_cluster(cluster: ClusterInfo, **_) -> Operation:
    """
    Stop cluster
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_STOP_CLUSTER_API')

    return create_operation(
        task_type=ClickhouseTasks.stop,
        operation_type=ClickhouseOperations.stop,
        metadata=StopClusterMetadata(),
        cid=cluster.cid,
    )
