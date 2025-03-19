# -*- coding: utf-8 -*-
"""
API for MySQL databases management.
"""

from ..mysql.utils import get_idm_system_user_permissions
from ...utils import metadb
from ...utils.infra import get_flavor_by_cluster_id
from ...utils.metadata import CreateDatabaseMetadata, DeleteDatabaseMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .info import get_cluster_config_set
from .pillar import SYSTEM_IDM_USERS, get_cluster_pillar
from .traits import MySQLOperations, MySQLTasks


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.CREATE)
def add_mysql_database(cluster, database_spec, **_):
    """
    Creates MySQL database
    """
    pillar = get_cluster_pillar(cluster)

    db_name = pillar.add_database(database_spec, _get_lower_case_table_names(cluster, pillar))

    # assign default permissions to IDM system users if they exist
    for user_name, db_roles in SYSTEM_IDM_USERS.items():
        if pillar.user_exists(user_name):
            user = pillar.user(cluster['cid'], user_name)
            permissions = user['permissions'] + get_idm_system_user_permissions(db_roles, [db_name])
            pillar.update_user(user_name, None, permissions, None, None, None, None)

    metadb.update_cluster_pillar(cluster['cid'], pillar)

    return create_operation(
        task_type=MySQLTasks.database_create,
        operation_type=MySQLOperations.database_add,
        metadata=CreateDatabaseMetadata(database_name=db_name),
        cid=cluster['cid'],
        task_args={'target-database': db_name, 'zk_hosts': pillar.zk_hosts},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.DELETE)
def delete_mysql_database(cluster, database_name, **_):
    """
    Deletes MySQL database.
    """
    pillar = get_cluster_pillar(cluster)

    db_name = pillar.delete_database(database_name, _get_lower_case_table_names(cluster, pillar))

    metadb.update_cluster_pillar(cluster['cid'], pillar)

    return create_operation(
        task_type=MySQLTasks.database_delete,
        operation_type=MySQLOperations.database_delete,
        metadata=DeleteDatabaseMetadata(database_name=db_name),
        cid=cluster['cid'],
        task_args={'target-database': db_name, 'zk_hosts': pillar.zk_hosts},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.INFO)
def get_mysql_database(cluster, database_name, **_):
    """
    Returns MySQL database.
    """
    pillar = get_cluster_pillar(cluster)

    return pillar.database(cluster['cid'], database_name)


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.LIST)
def list_mysql_databases(cluster, **_):
    """
    Returns list of MySQL databases.
    """
    pillar = get_cluster_pillar(cluster)

    return {'databases': pillar.databases(cluster['cid'])}


def _get_lower_case_table_names(cluster, pillar) -> int:
    flavor = get_flavor_by_cluster_id(cluster['cid'])
    config_set = get_cluster_config_set(cluster=cluster, pillar=pillar, flavor=flavor, version=None)

    return config_set.effective.get('lower_case_table_names')  # type: ignore
