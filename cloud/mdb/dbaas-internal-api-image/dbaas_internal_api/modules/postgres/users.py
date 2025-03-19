# -*- coding: utf-8 -*-
"""
DBaaS Internal API PostgreSQL user methods
"""

from . import utils, validation
from ...core.crypto import encrypt
from ...utils import infra, metadb
from ...utils.metadata import (
    CreateUserMetadata,
    DeleteUserMetadata,
    GrantUserPermissionMetadata,
    ModifyUserMetadata,
    RevokeUserPermissionMetadata,
)
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .api import convertors
from .constants import MY_CLUSTER_TYPE, USER_OBJ
from .pillar import get_cluster_pillar
from .traits import PostgresqlOperations, PostgresqlTasks
from .types import UserWithPassword

MY_OBJ = USER_OBJ


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.CREATE)
def create_postgresql_user(cluster, user_spec, **_):
    """
    Creates postgresql user. Returns task for worker
    """
    cluster_pillar = get_cluster_pillar(cluster)
    databases = cluster_pillar.databases.get_databases()
    user = convertors.user_from_spec(user_spec, databases)

    resources = infra.get_resources(cluster['cid'])
    flavor = metadb.get_flavor_by_name(resources.resource_preset_id)

    utils.validate_user_name(user.name)
    validation.validate_user_conns(user, cluster_pillar, flavor)
    utils.validate_grants(cluster_pillar.pgusers.get_users(), cluster_pillar.sox_audit)

    cluster_pillar.pgusers.add_user(user)
    metadb.update_cluster_pillar(cluster['cid'], cluster_pillar)

    return create_operation(
        task_type=PostgresqlTasks.user_create,
        operation_type=PostgresqlOperations.user_create,
        metadata=CreateUserMetadata(user_name=user.name),
        cid=cluster['cid'],
        task_args={'target-user': user.name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.MODIFY)
def modify_postgresql_user(cluster, name, **user_spec):
    """
    Modifies postgresql user. Returns task for worker
    """
    cluster_pillar = get_cluster_pillar(cluster)
    utils.validate_user_name(name)

    user = cluster_pillar.pgusers.user(name)
    user = UserWithPassword(
        name=name,
        encrypted_password=(encrypt(user_spec['password']) if 'password' in user_spec else user.encrypted_password),
        connect_dbs=(
            utils.permissions_to_databases(user_spec['permissions']) if 'permissions' in user_spec else user.connect_dbs
        ),
        conn_limit=(user_spec['conn_limit'] if 'conn_limit' in user_spec else user.conn_limit),
        settings=(user_spec['settings'] if 'settings' in user_spec else user.settings),
        login=(user_spec['login'] if 'login' in user_spec else user.login),
        grants=(user_spec['grants'] if 'grants' in user_spec else user.grants),
    )

    resources = infra.get_resources(cluster['cid'])
    flavor = metadb.get_flavor_by_name(resources.resource_preset_id)
    validation.validate_user_conns(user, cluster_pillar, flavor)
    cluster_pillar.pgusers.update_user(user)
    utils.validate_grants(cluster_pillar.pgusers.get_users(), cluster_pillar.sox_audit)

    metadb.update_cluster_pillar(cluster['cid'], cluster_pillar)

    return create_operation(
        task_type=PostgresqlTasks.user_modify,
        operation_type=PostgresqlOperations.user_modify,
        metadata=ModifyUserMetadata(user_name=name),
        cid=cluster['cid'],
        task_args={'target-user': name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.GRANT_PERMISSION)
def grant_postgresql_user_permission(cluster, name, permission, **_):
    """
    Grants permission to PostgreSQL user.
    """
    cluster_pillar = get_cluster_pillar(cluster)
    cluster_pillar.pgusers.add_user_permission(name, permission)
    metadb.update_cluster_pillar(cluster['cid'], cluster_pillar)

    return create_operation(
        task_type=PostgresqlTasks.user_modify,
        operation_type=PostgresqlOperations.grant_permission,
        metadata=GrantUserPermissionMetadata(user_name=name),
        cid=cluster['cid'],
        task_args={'target-user': name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.REVOKE_PERMISSION)
def revoke_postgresql_user_permission(cluster, name, database_name, **_):
    """
    Adds permission to PostgreSQL user.
    """
    cluster_pillar = get_cluster_pillar(cluster)
    cluster_pillar.pgusers.delete_user_permission(name, database_name)
    metadb.update_cluster_pillar(cluster['cid'], cluster_pillar)

    return create_operation(
        task_type=PostgresqlTasks.user_modify,
        operation_type=PostgresqlOperations.revoke_permission,
        metadata=RevokeUserPermissionMetadata(user_name=name),
        cid=cluster['cid'],
        task_args={'target-user': name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.DELETE)
def delete_postgresql_user(cluster, name, **_):
    """
    Deletes postgresql user. Returns task for worker
    """
    cluster_pillar = get_cluster_pillar(cluster)

    utils.validate_user_name(name)

    cluster_pillar.pgusers.delete_user(name)
    metadb.update_cluster_pillar(cluster['cid'], cluster_pillar)

    return create_operation(
        task_type=PostgresqlTasks.user_delete,
        operation_type=PostgresqlOperations.user_delete,
        metadata=DeleteUserMetadata(user_name=name),
        cid=cluster['cid'],
        task_args={'target-user': name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.INFO)
def info_postgresql_user(cluster, name, **_):
    """
    Returns info about postgresql user
    """
    cluster_pillar = get_cluster_pillar(cluster)
    user = cluster_pillar.pgusers.public_user(name)

    return convertors.user_to_spec(user, cluster_id=cluster['cid'])


@register_request_handler(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.LIST)
def list_postgresql_user(cluster, **_):
    """
    Returns list of postgresql users
    """
    cluster_pillar = get_cluster_pillar(cluster)

    return {
        MY_OBJ: [
            convertors.user_to_spec(u, cluster_id=cluster['cid']) for u in cluster_pillar.pgusers.get_public_users()
        ],
    }
