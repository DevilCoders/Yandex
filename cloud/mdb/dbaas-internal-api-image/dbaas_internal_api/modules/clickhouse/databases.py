# -*- coding: utf-8 -*-
"""
API for ClickHouse databases management.
"""

from ...core.exceptions import DatabaseAPIDisabledError
from ...utils import logs, metadb
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.metadata import CreateDatabaseMetadata, DeleteDatabaseMetadata
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .pillar import get_subcid_and_pillar
from .traits import ClickhouseOperations, ClickhouseTasks
from .utils import create_operation, validate_db_name


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.CREATE)
def add_clickhouse_database(cluster, database_spec, **kwargs):
    """
    Creates ClickHouse database
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_DATABASES_API')

    logs.log_debug_struct(kwargs)
    validate_db_name(database_spec['name'])

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    if pillar.sql_database_management:
        raise DatabaseAPIDisabledError()

    pillar.add_database(database_spec)

    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return create_operation(
        task_type=ClickhouseTasks.database_create,
        operation_type=ClickhouseOperations.database_add,
        metadata=CreateDatabaseMetadata(database_name=database_spec['name']),
        cid=cluster['cid'],
        task_args={'target-database': database_spec['name']},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.DELETE)
def delete_clickhouse_database(cluster, database_name, **_):
    """
    Deletes ClickHouse database.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_DATABASES_API')

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    if pillar.sql_database_management:
        raise DatabaseAPIDisabledError()

    pillar.delete_database(database_name)

    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return create_operation(
        task_type=ClickhouseTasks.database_delete,
        operation_type=ClickhouseOperations.database_delete,
        metadata=DeleteDatabaseMetadata(database_name=database_name),
        cid=cluster['cid'],
        task_args={'target-database': database_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.INFO)
def get_clickhouse_database(cluster, database_name, **_):
    """
    Returns ClickHouse database.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_DATABASES_API')

    _subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    return pillar.database(cluster['cid'], database_name)


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.LIST)
def list_clickhouse_databases(cluster, **_):
    """
    Returns list of ClickHouse databases.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_DATABASES_API')

    _subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    if pillar.sql_database_management:
        return {'databases': []}

    return {'databases': pillar.databases(cluster['cid'])}
