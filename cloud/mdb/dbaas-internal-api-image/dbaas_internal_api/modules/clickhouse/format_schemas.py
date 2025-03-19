# -*- coding: utf-8 -*-
"""
API for managing ClickHouse format schemas.
"""
from ...core.types import Operation
from ...utils import metadb
from ...utils.feature_flags import ensure_no_feature_flag
from ...utils.metadata import Metadata
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .constants import MY_CLUSTER_TYPE
from .pillar import get_subcid_and_pillar
from .traits import ClickhouseOperations, ClickhouseTasks
from .utils import create_operation


class FormatSchemaMetadata(Metadata):
    """
    Metadata class for operations on ClickHouse format schemas.
    """

    def __init__(self, format_schema_name: str) -> None:
        self.format_schema_name = format_schema_name

    def _asdict(self) -> dict:
        return {'format_schema_name': self.format_schema_name}


@register_request_handler(MY_CLUSTER_TYPE, Resource.FORMAT_SCHEMA, DbaasOperation.CREATE)
def create_format_schema_handler(cluster: dict, format_schema_name: str, uri: str, **kwargs) -> Operation:
    """
    Handler for create ClickHouse format schema requests.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_FORMAT_SCHEMAS_API')

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    pillar.add_format_schema(format_schema_name, kwargs['type'], uri)
    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return create_operation(
        task_type=ClickhouseTasks.format_schema_create,
        operation_type=ClickhouseOperations.format_schema_create,
        metadata=FormatSchemaMetadata(format_schema_name),
        cid=cluster['cid'],
        task_args={'target-format-schema': format_schema_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.FORMAT_SCHEMA, DbaasOperation.MODIFY)
def modify_format_schema_handler(cluster: dict, format_schema_name: str, uri: str = None, **_) -> Operation:
    """
    Handler for create ClickHouse format schema requests.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_FORMAT_SCHEMAS_API')

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    pillar.update_format_schema(format_schema_name, uri)
    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return create_operation(
        task_type=ClickhouseTasks.format_schema_modify,
        operation_type=ClickhouseOperations.format_schema_modify,
        metadata=FormatSchemaMetadata(format_schema_name),
        cid=cluster['cid'],
        task_args={'target-format-schema': format_schema_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.FORMAT_SCHEMA, DbaasOperation.DELETE)
def delete_format_schema_handler(cluster, format_schema_name, **_):
    """
    Deletes ClickHouse format schema.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_FORMAT_SCHEMAS_API')

    subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    pillar.delete_format_schema(format_schema_name)

    metadb.update_subcluster_pillar(cluster['cid'], subcid, pillar)

    return create_operation(
        task_type=ClickhouseTasks.format_schema_delete,
        operation_type=ClickhouseOperations.format_schema_delete,
        metadata=FormatSchemaMetadata(format_schema_name),
        cid=cluster['cid'],
        task_args={'target-format-schema': format_schema_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.FORMAT_SCHEMA, DbaasOperation.INFO)
def get_format_schema_handler(cluster, format_schema_name, **_):
    """
    Returns ClickHouse format schema.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_FORMAT_SCHEMAS_API')

    _subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    return pillar.format_schema(cluster['cid'], format_schema_name)


@register_request_handler(MY_CLUSTER_TYPE, Resource.FORMAT_SCHEMA, DbaasOperation.LIST)
def list_format_schemas_handler(cluster, **_):
    """
    Returns ClickHouse format schemas.
    """
    ensure_no_feature_flag('MDB_CLICKHOUSE_DISABLE_LEGACY_FORMAT_SCHEMAS_API')

    _subcid, pillar = get_subcid_and_pillar(cluster['cid'])

    return {'format_schemas': pillar.format_schemas(cluster['cid'])}
