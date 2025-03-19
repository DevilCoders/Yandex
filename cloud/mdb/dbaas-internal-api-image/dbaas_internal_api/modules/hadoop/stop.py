# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop cluster stop
"""
import semver

from ...core.exceptions import DbaasClientError
from ...core.types import Operation
from ...utils.metadata import StopClusterMetadata
from ...utils import metadb
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from ...utils.types import ClusterInfo
from .constants import MY_CLUSTER_TYPE
from .traits import HadoopOperations, HadoopTasks
from .pillar import get_cluster_pillar


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.STOP)
def hadoop_stop_cluster(cluster: ClusterInfo, decommission_timeout=None, **_) -> Operation:
    """
    Stop cluster
    """
    # db_specific_params.get('decommission_timeout')
    cid = cluster.cid
    pillar = get_cluster_pillar(cluster)
    image_version = semver.VersionInfo.parse(pillar.semantic_version)
    if decommission_timeout and image_version < semver.VersionInfo(1, 2):
        raise DbaasClientError('Decommission is supported for clusters with version >= 1.2')
    task_args = {}
    if not decommission_timeout:
        metadb.terminate_hadoop_jobs(cid)
    else:
        task_args['decommission_timeout'] = decommission_timeout

    instance_groups = metadb.get_instance_groups(cid)
    task_args['instance_group_ids'] = [instance_group['instance_group_id'] for instance_group in instance_groups]
    pillar = get_cluster_pillar(cluster)
    task_args['service_account_id'] = pillar.service_account_id

    return create_operation(
        task_type=HadoopTasks.stop,
        operation_type=HadoopOperations.stop,
        metadata=StopClusterMetadata(),
        cid=cid,
        task_args=task_args,
    )
