# -*- coding: utf-8 -*-
"""
API for MongoDB databases management.
"""

from ...utils import metadb
from ...utils.metadata import CreateDatabaseMetadata, DeleteDatabaseMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .pillar import get_cluster_pillar
from .traits import MongoDBOperations, MongoDBTasks
from .utils import get_user_database_task_timeout


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.CREATE)
def add_mongodb_database(cluster, database_spec, **_):
    """
    Creates MongoDB database
    """
    pillar = get_cluster_pillar(cluster)

    pillar.add_database(database_spec)

    metadb.update_cluster_pillar(cluster['cid'], pillar)

    return create_operation(
        task_type=MongoDBTasks.database_create,
        operation_type=MongoDBOperations.database_add,
        metadata=CreateDatabaseMetadata(database_name=database_spec['name']),
        time_limit=get_user_database_task_timeout(pillar),
        cid=cluster['cid'],
        task_args={'target-database': database_spec['name']},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.DELETE)
def delete_mongodb_database(cluster, database_name, **_):
    """
    Deletes MongoDB database.
    """
    pillar = get_cluster_pillar(cluster)

    pillar.delete_database(database_name)

    metadb.update_cluster_pillar(cluster['cid'], pillar)

    return create_operation(
        task_type=MongoDBTasks.database_delete,
        operation_type=MongoDBOperations.database_delete,
        metadata=DeleteDatabaseMetadata(database_name=database_name),
        time_limit=get_user_database_task_timeout(pillar),
        cid=cluster['cid'],
        task_args={'target-database': database_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.INFO)
def get_mongodb_database(cluster, database_name, **_):
    """
    Returns MongoDB database.
    """
    pillar = get_cluster_pillar(cluster)

    return pillar.database(cluster['cid'], database_name)


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.LIST)
def list_mongodb_databases(cluster, **_):
    """
    Returns list of MongoDB databases.
    """
    pillar = get_cluster_pillar(cluster)

    return {'databases': pillar.databases(cluster['cid'])}
