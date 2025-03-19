# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop cluster start
"""

from typing import Any
from ...core.types import Operation
from ...utils.metadata import StartClusterMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo
from ...utils import metadb
from .constants import MY_CLUSTER_TYPE
from .traits import HadoopOperations, HadoopTasks
from .pillar import get_cluster_pillar
from .compute_quota import check_subclusters_compute_quota


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.START)
def hadoop_start_cluster(cluster: ClusterInfo) -> Operation:
    """
    Start cluster
    """
    task_args: dict[str, Any] = {}
    pillar = get_cluster_pillar(cluster)
    instance_groups = metadb.get_instance_groups(cluster.cid)
    task_args['instance_group_ids'] = [instance_group['instance_group_id'] for instance_group in instance_groups]
    task_args['service_account_id'] = pillar.service_account_id

    check_subclusters_compute_quota(pillar._subclusters.values(), on_cluster_start=True)

    return create_operation(
        task_type=HadoopTasks.start,
        operation_type=HadoopOperations.start,
        metadata=StartClusterMetadata(),
        cid=cluster.cid,
        task_args=task_args,
    )
