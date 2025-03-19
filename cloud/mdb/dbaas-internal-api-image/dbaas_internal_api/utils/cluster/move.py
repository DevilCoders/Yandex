# -*- coding: utf-8 -*-
"""
DBaaS Internal API utils for cluster-related operations
Contains some wrappers on metadb operations
"""

from flask import current_app, g

from dbaas_internal_api.utils.quota import db_quota_limit_field, db_quota_usage_field

from .. import metadb
from ...core.auth import check_action
from ...core.exceptions import DbaasClientError
from ...core.types import Operation
from ..identity import get_folder_by_ext_id
from ..metadata import Metadata
from ..operation_creator import OperationChecks, OperationCreator, compose_task_args, get_idempotence_from_request
from ..search_renders import render_deleted_cluster
from ..types import ClusterInfo, ComparableEnum, FolderIds
from ..validation import check_resource_diffs


def move_cluster(
    cluster: ClusterInfo,
    destination_folder_id: str,
    src_task_type: ComparableEnum,
    dest_task_type: ComparableEnum,
    operation_type: ComparableEnum,
    metadata: Metadata,
) -> Operation:
    """
    Move cluster to folder
    """
    check_action(action='mdb.all.create', folder_ext_id=destination_folder_id)
    allow_missing = current_app.config['IDENTITY']['create_missing']
    dest_cloud, dest_folder = get_folder_by_ext_id(
        allow_missing=allow_missing, create_missing=allow_missing, folder_id=destination_folder_id
    )
    cloud_changed = dest_cloud['cloud_ext_id'] != g.cloud['cloud_ext_id']
    if cloud_changed:
        if not current_app.config['IDENTITY']['allow_move_between_clouds']:
            raise DbaasClientError('Unable to move cluster between clouds')
        metadb.lock_cloud(update_context=False)
        quota_change = metadb.get_cluster_quota_usage(cluster.cid)
        diffs = {
            key: {
                'quota': dest_cloud[db_quota_limit_field(key)],
                'used': dest_cloud[db_quota_usage_field(key)],
                'change': value,
            }
            for key, value in quota_change.items()
        }
        check_resource_diffs(dest_cloud['cloud_ext_id'], diffs)
        dest_resources = {'add_{key}'.format(key=key): value for key, value in quota_change.items()}
        src_resources = {'add_{key}'.format(key=key): -value for key, value in quota_change.items()}
        metadb.cloud_update_used_resources(**src_resources)
        metadb.cloud_update_used_resources(**dest_resources, cloud_id=dest_cloud['cloud_id'])

    metadb.update_cluster_folder(cluster.cid, dest_folder['folder_id'])

    # if cluster change cloud we should:
    #   1. send delete document with old cloud
    #   2. send new document with new cloud, folder ...
    # Search use sharding by cloud_id - https://st.yandex-team.ru/PS-3211#5d56e673a2b79e001eca38f6
    def as_deleted_search_queue_render(operation: Operation) -> dict:
        return render_deleted_cluster(
            cluster=cluster,
            timestamp=operation.created_at,
            deleted_at=operation.created_at,
            folder_ext_id=g.folder['folder_ext_id'],
            cloud_ext_id=g.cloud['cloud_ext_id'],
        )

    src_op = OperationCreator(
        cid=cluster.cid,
        folder=FolderIds(
            folder_id=g.folder['folder_id'],
            folder_ext_id=g.folder['folder_ext_id'],
        ),
        cloud_ext_id=g.cloud['cloud_ext_id'],
        search_queue_render=as_deleted_search_queue_render if cloud_changed else OperationCreator.skip_search_queue,
    ).add_operation(
        task_type=src_task_type,
        operation_type=operation_type,
        metadata=metadata,
        task_args=compose_task_args(),
        idempotence_data=get_idempotence_from_request(),
    )

    OperationCreator(
        cid=cluster.cid,
        folder=FolderIds(folder_id=dest_folder['folder_id'], folder_ext_id=dest_folder['folder_ext_id']),
        cloud_ext_id=dest_cloud['cloud_ext_id'],
        skip_checks=OperationChecks.all,
        event_render=OperationCreator.skip_event,
    ).add_operation(
        task_type=dest_task_type,
        operation_type=operation_type,
        metadata=metadata,
        task_args=compose_task_args(),
        idempotence_data=None,
        required_operation_id=src_op.id,
    )

    return src_op
