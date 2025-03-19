# -*- coding: utf-8 -*-
"""
DBaaS Internal API MySQL cluster move
"""

from flask import g

from ...core.types import Operation
from ...utils.cluster.move import move_cluster
from ...utils.metadata import MoveClusterMetadata
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo
from .constants import MY_CLUSTER_TYPE
from .traits import MySQLOperations, MySQLTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.MOVE)
def mysql_move_cluster(cluster: ClusterInfo, destination_folder_id: str) -> Operation:
    """
    Move cluster
    """
    return move_cluster(
        cluster,
        destination_folder_id,
        MySQLTasks.move,
        MySQLTasks.move_noop,
        MySQLOperations.move,
        MoveClusterMetadata(g.folder['folder_ext_id'], destination_folder_id),
    )
