# -*- coding: utf-8 -*-
"""
Schemas for ClickHouse databases.
"""

from marshmallow.fields import Nested

from ....apis.schemas.common import ListResponseSchemaV1
from ....apis.schemas.fields import Str
from ....apis.schemas.objects import DatabaseSchemaV1
from ....utils.register import DbaasOperation, Resource, register_request_schema, register_response_schema
from ....utils.validation import Schema
from ..constants import MY_CLUSTER_TYPE
from ..traits import ClickhouseClusterTraits


@register_response_schema(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.INFO)
class ClickhouseDatabaseSchemaV1(DatabaseSchemaV1):
    """
    ClickHouse database schema.
    """

    name = Str(validate=ClickhouseClusterTraits.db_name.validate, required=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.LIST)
class ClickhouseListDatabasesResponseSchemaV1(ListResponseSchemaV1):
    """
    ClickHouse database list schema.
    """

    databases = Nested(ClickhouseDatabaseSchemaV1, many=True, required=True)


class ClickhouseDatabaseSpecSchemaV1(Schema):
    """
    ClickHouse database spec schema.
    """

    name = Str(validate=ClickhouseClusterTraits.db_name.validate, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.CREATE)
class ClickhouseCreateDatabaseRequestSchemaV1(Schema):
    """
    Schema for create ClickHouse database request.
    """

    databaseSpec = Nested(ClickhouseDatabaseSpecSchemaV1, attribute='database_spec', required=True)
