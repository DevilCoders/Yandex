# -*- coding: utf-8 -*-
"""
API for ClickHouse users management.
"""
from ...core.exceptions import UserAPIDisabledError
from ...utils import metadb
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.metadata import (
    CreateUserMetadata,
    DeleteUserMetadata,
    GrantUserPermissionMetadata,
    ModifyUserMetadata,
    RevokeUserPermissionMetadata,
)
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .pillar import get_subcid_and_pillar
from .traits import ClickhouseOperations, ClickhouseTasks
from .utils import create_operation, process_user_spec, validate_user_quotas


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.CREATE)
def create_clickhouse_user(cluster, user_spec, **_):
    """
    Creates ClickHouse user.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_USERS_API')

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    if pillar.sql_user_management:
        raise UserAPIDisabledError()

    process_user_spec(user_spec, pillar.database_names)
    validate_user_quotas(user_spec.get('quotas'))

    pillar.add_user(user_spec)

    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return create_operation(
        task_type=ClickhouseTasks.user_create,
        operation_type=ClickhouseOperations.user_create,
        metadata=CreateUserMetadata(user_name=user_spec['name']),
        cid=cluster['cid'],
        task_args={'target-user': user_spec['name']},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.MODIFY)
def modify_clickhouse_user(cluster, user_name, password=None, permissions=None, settings=None, quotas=None, **_):
    """
    Modifies ClickHouse user.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_USERS_API')

    validate_user_quotas(quotas)

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    if pillar.sql_user_management:
        raise UserAPIDisabledError()

    pillar.update_user(user_name, password, permissions, settings, quotas)

    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return create_operation(
        task_type=ClickhouseTasks.user_modify,
        operation_type=ClickhouseOperations.user_modify,
        metadata=ModifyUserMetadata(user_name=user_name),
        cid=cluster['cid'],
        task_args={'target-user': user_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.GRANT_PERMISSION)
def grant_clickhouse_user_permission(cluster, user_name, permission, **_):
    """
    Grants permission to ClickHouse user.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_USERS_API')

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    if pillar.sql_user_management:
        raise UserAPIDisabledError()

    pillar.add_user_permission(user_name, permission)

    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return create_operation(
        task_type=ClickhouseTasks.user_modify,
        operation_type=ClickhouseOperations.grant_permission,
        metadata=GrantUserPermissionMetadata(user_name=user_name),
        cid=cluster['cid'],
        task_args={'target-user': user_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.REVOKE_PERMISSION)
def revoke_clickhouse_user_permission(cluster, user_name, database_name, **_):
    """
    Revokes permission from ClickHouse user.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_USERS_API')

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    if pillar.sql_user_management:
        raise UserAPIDisabledError()

    pillar.delete_user_permission(user_name, database_name)

    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return create_operation(
        task_type=ClickhouseTasks.user_modify,
        operation_type=ClickhouseOperations.revoke_permission,
        metadata=RevokeUserPermissionMetadata(user_name=user_name),
        cid=cluster['cid'],
        task_args={'target-user': user_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.DELETE)
def delete_clickhouse_user(cluster, user_name, **_):
    """
    Deletes ClickHouse user.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_USERS_API')

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    if pillar.sql_user_management:
        raise UserAPIDisabledError()

    pillar.delete_user(user_name)

    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return create_operation(
        task_type=ClickhouseTasks.user_delete,
        operation_type=ClickhouseOperations.user_delete,
        metadata=DeleteUserMetadata(user_name=user_name),
        cid=cluster['cid'],
        task_args={'target-user': user_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.INFO)
def get_clickhouse_user(cluster, user_name, **_):
    """
    Returns ClickHouse user.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_USERS_API')

    _subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    return pillar.user(cluster['cid'], user_name)


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.LIST)
def list_clickhouse_users(cluster, **_):
    """
    Returns list of ClickHouse users.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_USERS_API')

    _subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    if pillar.sql_user_management:
        return {'users': []}

    return {'users': pillar.users(cluster['cid'])}
