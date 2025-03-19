"""
DBaaS Internal API cluster delete helpers
"""

from datetime import timedelta
from typing import Type

from .. import metadb
from ...core.types import Operation
from ..config import get_backups_purge_delay
from ..metadata import DeleteClusterMetadata, Metadata
from ..operation_creator import (
    OperationChecks,
    OperationCreator,
    compose_task_args,
    create_operation,
    get_idempotence_from_request,
)
from ..types import ComparableEnum


def delete_shard_with_task(
    cluster: dict,
    task_type: ComparableEnum,
    operation_type: ComparableEnum,
    metadata: Metadata,
    shard_id: str,
    time_limit: timedelta = None,
    args=None,
) -> Operation:
    """
    Delete shard and create corresponding task for worker in metadb.
    """
    hosts = metadb.get_shard_hosts(shard_id)
    metadb.delete_hosts_batch(cid=cluster['cid'], fqdns=[h['fqdn'] for h in hosts])
    metadb.delete_shard(shard_id, cid=cluster['cid'])

    flavor = metadb.get_flavor_by_id(hosts[0]['flavor'])
    host_count = len(hosts)
    metadb.cloud_update_used_resources(
        add_cpu=-host_count * flavor['cpu_guarantee'],
        add_gpu=-host_count * flavor['gpu_limit'],
        add_memory=-host_count * flavor['memory_guarantee'],
        add_space=-host_count * hosts[0]['space_limit'],
        disk_type_id=hosts[0]['disk_type_id'],
    )

    task_args = args if args else {}
    task_args['shard_hosts'] = [
        {
            'fqdn': host['fqdn'],
            'vtype': host['vtype'],
            'vtype_id': host['vtype_id'],
        }
        for host in hosts
    ]

    return create_operation(
        task_type=task_type,
        operation_type=operation_type,
        metadata=metadata,
        cid=cluster['cid'],
        time_limit=time_limit,
        task_args=task_args,
    )


def delete_cluster_tasks(
    cid: str, task_args: dict, tasks_enum: Type[ComparableEnum], operations_enum: Type[ComparableEnum]
) -> Operation:
    """
    Create delete cluster task
    """

    task_args = compose_task_args(task_args)
    del_operation = OperationCreator.make_from_request(
        cid,
        # don't check failed, cause
        # user should be able to delete `broken` cluster
        skip_checks=OperationChecks.is_consistent,
    ).add_operation(
        task_type=tasks_enum['delete'],
        operation_type=operations_enum['delete'],
        metadata=DeleteClusterMetadata(),
        task_args=task_args,
        idempotence_data=get_idempotence_from_request(),
    )

    del_metadata_operation = OperationCreator.make_from_request(
        cid,
        skip_checks=OperationChecks.all,
        search_queue_render=OperationCreator.skip_search_queue,
        event_render=OperationCreator.skip_event,
    ).add_operation(
        task_type=tasks_enum['delete_metadata'],
        operation_type=operations_enum['delete'],
        metadata=DeleteClusterMetadata(),
        task_args=task_args,
        idempotence_data=None,
        hidden=True,
        required_operation_id=del_operation.id,
    )

    OperationCreator.make_from_request(
        cid,
        skip_checks=OperationChecks.all,
        search_queue_render=OperationCreator.skip_search_queue,
        event_render=OperationCreator.skip_event,
    ).add_operation(
        task_type=tasks_enum['purge'],
        operation_type=operations_enum['delete'],
        metadata=DeleteClusterMetadata(),
        task_args=task_args,
        idempotence_data=None,
        hidden=True,
        required_operation_id=del_metadata_operation.id,
        delay_by=timedelta(days=get_backups_purge_delay()),
    )

    return del_operation
