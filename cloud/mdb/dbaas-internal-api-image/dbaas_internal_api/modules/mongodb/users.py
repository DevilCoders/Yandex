# -*- coding: utf-8 -*-
"""
DBaaS Internal API MongoDB user methods
"""
from ...utils import metadb
from ...utils.metadata import (
    CreateUserMetadata,
    DeleteUserMetadata,
    GrantUserPermissionMetadata,
    ModifyUserMetadata,
    RevokeUserPermissionMetadata,
)
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .pillar import get_cluster_pillar
from .traits import MongoDBOperations, MongoDBTasks
from .utils import get_user_database_task_timeout


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.CREATE)
def create_mongodb_user(cluster, user_spec, **_):
    """
    Creates MongoDB user. Returns task for worker
    """
    pillar = get_cluster_pillar(cluster)
    pillar.add_user(user_spec)

    metadb.update_cluster_pillar(cluster['cid'], pillar)

    return create_operation(
        task_type=MongoDBTasks.user_create,
        operation_type=MongoDBOperations.user_create,
        metadata=CreateUserMetadata(user_name=user_spec['name']),
        time_limit=get_user_database_task_timeout(pillar),
        cid=cluster['cid'],
        task_args={'target-user': user_spec['name']},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.MODIFY)
def modify_mongodb_user(cluster, user_name, password=None, permissions=None, **_):
    """
    Modifies MongoDB user. Returns task for worker
    """
    pillar = get_cluster_pillar(cluster)

    pillar.update_user(user_name, password, permissions)

    metadb.update_cluster_pillar(cluster['cid'], pillar)
    return create_operation(
        task_type=MongoDBTasks.user_modify,
        operation_type=MongoDBOperations.user_modify,
        metadata=ModifyUserMetadata(user_name=user_name),
        time_limit=get_user_database_task_timeout(pillar),
        cid=cluster['cid'],
        task_args={'target-user': user_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.DELETE)
def delete_mongodb_user(cluster, user_name, **_):
    # pylint: disable=unused-argument
    """
    Deletes MongoDB user. Returns task for worker
    """
    pillar = get_cluster_pillar(cluster)

    pillar.delete_user(user_name)

    metadb.update_cluster_pillar(cluster['cid'], pillar)
    return create_operation(
        task_type=MongoDBTasks.user_delete,
        operation_type=MongoDBOperations.user_delete,
        metadata=DeleteUserMetadata(user_name=user_name),
        time_limit=get_user_database_task_timeout(pillar),
        cid=cluster['cid'],
        task_args={'target-user': user_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.INFO)
def get_mongodb_user(cluster, user_name, **_):
    """
    Returns info about MongoDB user
    """
    pillar = get_cluster_pillar(cluster)

    return pillar.user(cluster['cid'], user_name)


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.LIST)
def list_mongodb_users(cluster, **_):
    """
    Returns list of public MongoDB users
    """
    pillar = get_cluster_pillar(cluster)
    return {'users': pillar.users(cluster['cid'])}


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.GRANT_PERMISSION)
def grant_mongodb_user_permission(cluster, user_name, permission, **_):
    """
    Grants permission to MongoDB user.
    """
    pillar = get_cluster_pillar(cluster)

    pillar.add_user_permission(user_name, permission)

    metadb.update_cluster_pillar(cluster['cid'], pillar)

    return create_operation(
        task_type=MongoDBTasks.user_modify,
        operation_type=MongoDBOperations.grant_permission,
        metadata=GrantUserPermissionMetadata(user_name=user_name),
        time_limit=get_user_database_task_timeout(pillar),
        cid=cluster['cid'],
        task_args={'target-user': user_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.REVOKE_PERMISSION)
def revoke_mongodb_user_permission(cluster, user_name, database_name, **_):
    """
    Revokes permission from MongoDB user.
    """
    pillar = get_cluster_pillar(cluster)

    pillar.delete_user_permission(user_name, database_name)

    metadb.update_cluster_pillar(cluster['cid'], pillar)

    return create_operation(
        task_type=MongoDBTasks.user_modify,
        operation_type=MongoDBOperations.revoke_permission,
        metadata=RevokeUserPermissionMetadata(user_name=user_name),
        time_limit=get_user_database_task_timeout(pillar),
        cid=cluster['cid'],
        task_args={'target-user': user_name},
    )
