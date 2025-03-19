# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop cluster delete
"""

import semver

from ...core.exceptions import DbaasClientError, SubclusterNotExistsError
from ...utils import metadb
from ...utils.metadata import DeleteClusterMetadata
from ...utils.operation_creator import (
    OperationChecks,
    OperationCreator,
    compose_task_args,
    create_operation,
    get_idempotence_from_request,
)
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import COMPUTE_SUBCLUSTER_TYPE, MY_CLUSTER_TYPE
from .metadata import DeleteSubclusterMetadata
from .pillar import get_cluster_pillar
from .traits import HadoopOperations, HadoopTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.DELETE)
def hadoop_cluster_delete(cluster, undeleted_hosts, decommission_timeout=None, **_):
    """
    Delete Hadoop cluster
    """
    pillar = get_cluster_pillar(cluster)
    image_version = semver.VersionInfo.parse(pillar.semantic_version)
    if decommission_timeout and image_version < semver.VersionInfo(1, 2):
        raise DbaasClientError('Decommission is supported for clusters with version >= 1.2')

    task_args = {
        'decommission_timeout': decommission_timeout,
        'service_account_id': pillar.service_account_id,
    }

    cid = cluster['cid']
    instance_groups = metadb.get_instance_groups(cid)
    instance_group_ids = [instance_group['instance_group_id'] for instance_group in instance_groups]
    if instance_group_ids:
        task_args['instance_group_ids'] = instance_group_ids
    if undeleted_hosts:
        task_args['hosts'] = undeleted_hosts
    return OperationCreator.make_from_request(cid=cid, skip_checks=OperationChecks.is_consistent).add_operation(
        task_type=HadoopTasks.delete,
        operation_type=HadoopOperations.delete,
        metadata=DeleteClusterMetadata(),
        task_args=compose_task_args(task_args),
        idempotence_data=get_idempotence_from_request(),
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.SUBCLUSTER, DbaasOperation.DELETE)
def hadoop_subcluster_delete(cluster, subcid: str, decommission_timeout=None, **_):
    """Delete hadoop subcluster"""

    pillar = get_cluster_pillar(cluster)
    image_version = semver.VersionInfo.parse(pillar.semantic_version)
    if decommission_timeout and image_version < semver.VersionInfo(1, 2):
        raise DbaasClientError('Decommission is supported for clusters with version >= 1.2')

    subcluster = pillar.get_subcluster(subcid)
    if not subcluster:
        raise SubclusterNotExistsError(subcid)
    cid = cluster['cid']
    task_args = {
        'subcid': subcid,
        'decommission_timeout': decommission_timeout,
        'service_account_id': pillar.service_account_id,
    }
    subcluster_role = subcluster.get('role')
    if 'instance_group_config' in subcluster:
        instance_group = metadb.get_instance_group(subcid)
        task_args['instance_group_id'] = instance_group['instance_group_id']
    elif subcluster_role == COMPUTE_SUBCLUSTER_TYPE:
        host_list = metadb.get_hosts(cid, subcid=subcid)
        metadb.delete_hosts_batch([h['fqdn'] for h in host_list], cid)
        task_args['host_list'] = host_list
    else:
        raise DbaasClientError('Only deleting computenodes is allowed')

    metadb.delete_subcluster(cid=cid, subcid=subcid)
    pillar.remove_subcluster(subcid)
    metadb.update_cluster_pillar(cid, pillar)

    return create_operation(
        task_type=HadoopTasks['subcluster_delete'],
        operation_type=HadoopOperations['subcluster_delete'],
        cid=cid,
        metadata=DeleteSubclusterMetadata(subcid),
        task_args=compose_task_args(task_args),
    )
