# -*- coding: utf-8 -*-
"""
DBaaS Internal API Redis cluster move
"""

from flask import g

from ...core.types import Operation
from ...utils.cluster.move import move_cluster
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.metadata import MoveClusterMetadata
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo
from .constants import MY_CLUSTER_TYPE
from .traits import RedisOperations, RedisTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.MOVE)
def redis_move_cluster(cluster: ClusterInfo, destination_folder_id: str) -> Operation:
    """
    Move cluster
    """
    ensure_no_feature_flag('MDB_REDIS_DISABLE_LEGACY_CLUSTER_OPERATE_API')
    return move_cluster(
        cluster,
        destination_folder_id,
        RedisTasks.move,
        RedisTasks.move_noop,
        RedisOperations.move,
        MoveClusterMetadata(g.folder['folder_ext_id'], destination_folder_id),
    )
