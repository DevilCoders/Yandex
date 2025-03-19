# -*- coding: utf-8 -*-
"""
DBaaS Internal API MySQL user methods
"""
from dbaas_internal_api.modules.mysql.info import get_cluster_config_set
from dbaas_internal_api.utils.infra import get_flavor_by_cluster_id
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
from .traits import MySQLOperations, MySQLTasks
from .utils import get_cluster_version


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.CREATE)
def create_mysql_user(cluster, user_spec, **_):
    """
    Creates MySQL user. Returns task for worker
    """

    pillar = get_cluster_pillar(cluster)
    version = get_cluster_version(cluster['cid'], pillar)

    pillar.add_user(user_spec, version)
    metadb.update_cluster_pillar(cluster['cid'], pillar)

    return create_operation(
        task_type=MySQLTasks.user_create,
        operation_type=MySQLOperations.user_create,
        metadata=CreateUserMetadata(user_name=user_spec['name']),
        cid=cluster['cid'],
        task_args={'target-user': user_spec['name'], 'zk_hosts': pillar.zk_hosts},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.MODIFY)
def modify_mysql_user(
    cluster,
    user_name,
    password=None,
    permissions=None,
    global_permissions=None,
    connection_limits=None,
    plugin=None,
    **_
):
    """
    Modifies MySQL user. Returns task for worker
    """

    pillar = get_cluster_pillar(cluster)
    version = get_cluster_version(cluster['cid'], pillar)

    pillar.update_user(user_name, password, permissions, global_permissions, connection_limits, plugin, version)
    metadb.update_cluster_pillar(cluster['cid'], pillar)
    return create_operation(
        task_type=MySQLTasks.user_modify,
        operation_type=MySQLOperations.user_modify,
        metadata=ModifyUserMetadata(user_name=user_name),
        cid=cluster['cid'],
        task_args={'target-user': user_name, 'zk_hosts': pillar.zk_hosts},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.DELETE)
def delete_mysql_user(cluster, user_name, **_):
    # pylint: disable=unused-argument
    """
    Deletes MySQL user. Returns task for worker
    """
    pillar = get_cluster_pillar(cluster)

    pillar.delete_user(user_name)

    metadb.update_cluster_pillar(cluster['cid'], pillar)
    return create_operation(
        task_type=MySQLTasks.user_delete,
        operation_type=MySQLOperations.user_delete,
        metadata=DeleteUserMetadata(user_name=user_name),
        cid=cluster['cid'],
        task_args={'target-user': user_name, 'zk_hosts': pillar.zk_hosts},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.INFO)
def get_mysql_user(cluster, user_name, **_):
    """
    Returns info about MySQL user
    """
    pillar = get_cluster_pillar(cluster)

    return pillar.user(cluster['cid'], user_name)


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.LIST)
def list_mysql_users(cluster, **_):
    """
    Returns list of public MySQL users
    """
    pillar = get_cluster_pillar(cluster)
    return {'users': pillar.users(cluster['cid'])}


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.GRANT_PERMISSION)
def add_mysql_user_permission(cluster, user_name, permission, **_):
    """
    Adds permission to MySQL user.
    """
    pillar = get_cluster_pillar(cluster)
    pillar.add_user_permission(user_name, permission, _get_lower_case_table_names(cluster, pillar))

    metadb.update_cluster_pillar(cluster['cid'], pillar)

    return create_operation(
        task_type=MySQLTasks.user_modify,
        operation_type=MySQLOperations.grant_permission,
        metadata=GrantUserPermissionMetadata(user_name=user_name),
        cid=cluster['cid'],
        task_args={'target-user': user_name, 'zk_hosts': pillar.zk_hosts},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.REVOKE_PERMISSION)
def revoke_mysql_user_permission(cluster, user_name, permission, **_):
    """
    Revokes permission from MySQL user.
    """
    pillar = get_cluster_pillar(cluster)
    pillar.delete_user_permission(user_name, permission, _get_lower_case_table_names(cluster, pillar))

    metadb.update_cluster_pillar(cluster['cid'], pillar)

    return create_operation(
        task_type=MySQLTasks.user_modify,
        operation_type=MySQLOperations.revoke_permission,
        metadata=RevokeUserPermissionMetadata(user_name=user_name),
        cid=cluster['cid'],
        task_args={'target-user': user_name, 'zk_hosts': pillar.zk_hosts},
    )


def _get_lower_case_table_names(cluster, pillar) -> int:
    flavor = get_flavor_by_cluster_id(cluster['cid'])
    config_set = get_cluster_config_set(cluster=cluster, pillar=pillar, flavor=flavor, version=None)

    return config_set.effective.get('lower_case_table_names')  # type: ignore
